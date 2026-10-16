#include "model.h"
#include <algorithm>
#include <cwctype>
#include <cstdio>

ModelManager g_model;

static bool CaseInsensitiveContains(const wchar_t *str, const std::wstring &query) {
    if (!str || query.empty()) return true;
    size_t sLen = wcslen(str);
    size_t qLen = query.length();
    if (qLen > sLen) return false;

    for (size_t i = 0; i <= sLen - qLen; i++) {
        bool match = true;
        for (size_t j = 0; j < qLen; j++) {
            if (towlower(str[i + j]) != towlower(query[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

std::wstring ModelManager::NormalizePath(const wchar_t *p) {
    if (!p) return L"";
    std::wstring s = p;
    if (!s.empty() && s.front() == L'"') s.erase(0, 1);
    if (!s.empty() && s.back() == L'"') s.pop_back();
    for (auto &ch : s) ch = towupper(ch);
    if (s.length() == 2 && s[1] == L':') {
        s += L"\\";
    }
    if (s.length() > 3 && (s.back() == L'\\' || s.back() == L'/')) {
        s.pop_back();
    }
    return s;
}

std::wstring ModelManager::FormatBytes(uint64_t bytes) {
    const double KB = 1024.0;
    const double MB = 1024.0 * KB;
    const double GB = 1024.0 * MB;
    const double TB = 1024.0 * GB;

    wchar_t buf[64];
    double d = (double)bytes;

    if (d >= TB) {
        swprintf_s(buf, 64, L"%.2f TB", d / TB);
    } else if (d >= GB) {
        swprintf_s(buf, 64, L"%.2f GB", d / GB);
    } else if (d >= MB) {
        swprintf_s(buf, 64, L"%.2f MB", d / MB);
    } else if (d >= KB) {
        swprintf_s(buf, 64, L"%.1f KB", d / KB);
    } else {
        swprintf_s(buf, 64, L"%llu B", bytes);
    }

    return std::wstring(buf);
}

uint64_t ModelManager::CalculateSizeOnDisk(uint64_t logical_size, uint32_t cluster_size) {
    return FastScanner::CalculateSizeOnDisk(logical_size, cluster_size);
}

ModelManager::ModelManager()
    : m_is_scanning(false)
    , m_cancel_flag(false)
    , m_hWorkerThread(NULL)
{
    m_target_path[0] = L'\0';
}

void ModelManager::Shutdown() {
    CancelScan();
    if (m_hWorkerThread) {
        WaitForSingleObject(m_hWorkerThread, 2000);
        CloseHandle(m_hWorkerThread);
        m_hWorkerThread = NULL;
    }
}

ModelManager::~ModelManager() {
    Shutdown();
    ResetActiveState();
}


void ModelManager::ResetActiveState() {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    m_current_scan.reset();
}

bool ModelManager::HasResult() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_scan && (m_current_scan->total_files > 0 || m_current_scan->total_folders > 0);
}

bool ModelManager::IsCached(const wchar_t *path) {
    std::wstring norm = NormalizePath(path);
    std::lock_guard<std::mutex> lock(m_data_mutex);
    auto it = m_scan_cache.find(norm);
    return (it != m_scan_cache.end() && it->second->scan_data && it->second->scan_data->total_files > 0);
}

std::wstring ModelManager::GetCurrentScannedFolderSafe() const {
    std::lock_guard<std::mutex> lock(m_folder_mutex);
    return m_current_folder;
}

void ModelManager::SetCurrentScannedFolder(const wchar_t *folder) {
    std::lock_guard<std::mutex> lock(m_folder_mutex);
    m_current_folder = (folder) ? folder : L"";
}

uint64_t ModelManager::GetDiskTotal() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_scan ? m_current_scan->disk_total_bytes : 0;
}

uint64_t ModelManager::GetDiskFree() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_scan ? m_current_scan->disk_free_bytes : 0;
}

uint64_t ModelManager::GetDiskUsed() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_scan ? m_current_scan->disk_used_bytes : 0;
}

uint64_t ModelManager::GetTotalScannedLogical() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_scan ? m_current_scan->total_scanned_bytes : 0;
}

uint64_t ModelManager::GetTotalScannedOnDisk() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_scan ? m_current_scan->total_scanned_on_disk : 0;
}

uint64_t ModelManager::GetTotalFiles() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_scan ? m_current_scan->total_files : 0;
}

uint64_t ModelManager::GetTotalFolders() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_scan ? m_current_scan->total_folders : 0;
}

uint32_t ModelManager::GetClusterSize() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_scan ? m_current_scan->cluster_size : 4096;
}

float ModelManager::GetScanProgressPct() const {
    if (!m_is_scanning.load()) {
        return m_current_scan ? 100.0f : 0.0f;
    }
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (m_current_scan) {
        float p = m_current_scan->scan_progress_pct;
        if (p < 0.0f) p = 0.0f;
        if (p > 99.0f) p = 99.0f;
        return p;
    }
    return 0.0f;
}

uint64_t ModelManager::GetCategoryTotal(int cat, DisplaySizeMode size_mode) const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (!m_current_scan || cat < 0 || cat >= CAT_COUNT) return 0;
    return (size_mode == SIZE_ON_DISK) ? m_current_scan->cat_size_on_disk[cat] : m_current_scan->cat_size[cat];
}

uint64_t ModelManager::GetCategoryFileCount(int cat) const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (!m_current_scan || cat < 0 || cat >= CAT_COUNT) return 0;
    return m_current_scan->cat_count[cat];
}

static void ScanProgressCallback(const wchar_t *current_folder, void *user_data) {
    ModelManager *m = reinterpret_cast<ModelManager*>(user_data);
    if (m) {
        m->SetCurrentScannedFolder(current_folder);
    }
}

DWORD WINAPI ModelManager::WorkerThreadProc(LPVOID lpParam) {
    ModelManager *pThis = reinterpret_cast<ModelManager*>(lpParam);
    if (pThis) {
        __try {
            pThis->ExecuteScanThread();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pThis->m_is_scanning.store(false);
            pThis->SetCurrentScannedFolder(L"");
        }
    }
    return 0;
}

void ModelManager::ExecuteScanThread() {
    std::wstring norm = m_target_path;
    auto temp_res = std::make_shared<FastScanResult>();
    bool ok = FastScanner::Run(
        m_target_path,
        *temp_res,
        (volatile bool*)&m_cancel_flag,
        (volatile bool*)&m_is_scanning,
        ScanProgressCallback,
        this
    );

    {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        m_current_scan = temp_res;

        auto snap = std::make_shared<ScanSnapshot>();
        snap->target_path = norm;
        snap->scan_data = temp_res;
        snap->is_complete = ok;
        m_scan_cache[norm] = snap;
    }

    m_is_scanning.store(false);
    SetCurrentScannedFolder(L"");
}

bool ModelManager::SelectOrStartScan(const wchar_t *path, bool force_rescan) {
    if (!path || path[0] == L'\0') return false;
    std::wstring norm = NormalizePath(path);

    // Se NÃO for force_rescan e já estiver em cache, restaura instantaneamente (0 ms, 0 bytes clonados!)
    if (!force_rescan) {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        auto it = m_scan_cache.find(norm);
        if (it != m_scan_cache.end() && it->second->scan_data) {
            wcsncpy_s(m_target_path, MAX_PATH_EXTENDED, norm.c_str(), _TRUNCATE);
            m_current_scan = it->second->scan_data;
            return false;
        }
    }

    if (m_is_scanning.load()) {
        CancelScan();
    }

    if (m_hWorkerThread) {
        WaitForSingleObject(m_hWorkerThread, 2000);
        CloseHandle(m_hWorkerThread);
        m_hWorkerThread = NULL;
    }

    wcsncpy_s(m_target_path, MAX_PATH_EXTENDED, norm.c_str(), _TRUNCATE);
    m_is_scanning.store(true);
    m_cancel_flag.store(false);
    SetCurrentScannedFolder(L"");

    m_hWorkerThread = CreateThread(
        NULL,
        8 * 1024 * 1024, // 8 MB stack para evitar qualquer risco de STATUS_STACK_OVERFLOW
        WorkerThreadProc,
        this,
        STACK_SIZE_PARAM_IS_A_RESERVATION,
        NULL
    );

    return (m_hWorkerThread != NULL);
}

void ModelManager::CancelScan() {
    m_cancel_flag.store(true);
}

std::vector<FolderViewItem> ModelManager::GetFolders(
    bool hide_redundant,
    DisplaySizeMode size_mode,
    int sort_col,
    bool sort_desc,
    const wchar_t *search_query,
    size_t max_count
) {
    (void)sort_col;
    (void)sort_desc;
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (!m_current_scan || m_current_scan->folders.empty()) return {};

    const auto &folders = m_current_scan->folders;
    uint32_t cluster = m_current_scan->cluster_size;

    std::vector<FolderViewItem> result;
    size_t cap = (max_count > 0 && max_count < 100) ? max_count : 100;
    result.reserve(min(cap, folders.size()));

    std::wstring q = (search_query) ? search_query : L"";

    if (hide_redundant) {
        for (const auto &fr : folders) {
            if (result.size() >= cap) break;
            if (!q.empty() && !CaseInsensitiveContains(fr.folder_path, q)) continue;

            bool is_sub = false;
            size_t path_len = wcslen(fr.folder_path);

            for (const auto &acc : result) {
                size_t acc_len = wcslen(acc.folder_path);
                if (path_len > acc_len) {
                    if (_wcsnicmp(fr.folder_path, acc.folder_path, acc_len) == 0) {
                        wchar_t next_ch = fr.folder_path[acc_len];
                        if (next_ch == L'\\' || next_ch == L'/') {
                            is_sub = true;
                            break;
                        }
                    }
                }
            }

            if (!is_sub) {
                FolderViewItem item;
                item.folder_path = fr.folder_path;
                item.display_name = fr.display_name;
                item.size_logical = fr.cumulative_size;
                item.size_on_disk = CalculateSizeOnDisk(fr.cumulative_size, cluster);
                item.file_count = fr.file_count;
                item.subfolder_count = fr.subfolder_count;
                item.is_redundant_child = false;
                result.push_back(item);
            }
        }
    } else {
        for (const auto &fr : folders) {
            if (result.size() >= cap) break;
            if (!q.empty() && !CaseInsensitiveContains(fr.folder_path, q)) continue;

            FolderViewItem item;
            item.folder_path = fr.folder_path;
            item.display_name = fr.display_name;
            item.size_logical = fr.cumulative_size;
            item.size_on_disk = CalculateSizeOnDisk(fr.cumulative_size, cluster);
            item.file_count = fr.file_count;
            item.subfolder_count = fr.subfolder_count;
            item.is_redundant_child = false;
            result.push_back(item);
        }
    }

    return result;
}

std::vector<FileViewItem> ModelManager::GetFiles(
    uint64_t min_size,
    int category_filter,
    DisplaySizeMode size_mode,
    int sort_col,
    bool sort_desc,
    const wchar_t *search_query,
    size_t max_count
) {
    (void)sort_col;
    (void)sort_desc;
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (!m_current_scan || m_current_scan->files.empty()) return {};

    const auto &files = m_current_scan->files;

    std::vector<FileViewItem> result;

    bool has_filter = (min_size > 0) || (category_filter >= 0) || (search_query && search_query[0] != L'\0');

    // Caso 1: Sem filtros ativos (Top Files)
    if (!has_filter) {
        size_t limit = (max_count > 0) ? min(max_count, files.size()) : min((size_t)100, files.size());
        result.reserve(limit);
        for (size_t i = 0; i < limit; i++) {
            const auto &f = files[i];
            FileViewItem item;
            item.name = f.file_name;
            item.full_path = f.full_path;
            item.size_logical = f.size_bytes;
            item.size_on_disk = f.size_on_disk;
            item.category = f.category;
            item.last_modified = f.last_modified;
            result.push_back(item);
        }
        return result;
    }

    // Caso 2: Com filtros ativos (busca ou filtros de tamanho/categoria)
    std::wstring q = (search_query) ? search_query : L"";
    result.reserve(256);

    for (const auto &f : files) {
        if (max_count > 0 && result.size() >= max_count) break;

        uint64_t sz = (size_mode == SIZE_ON_DISK) ? f.size_on_disk : f.size_bytes;
        if (sz < min_size) continue;
        if (category_filter >= 0 && (int)f.category != category_filter) continue;
        if (!q.empty() && !CaseInsensitiveContains(f.file_name, q) && !CaseInsensitiveContains(f.full_path, q)) continue;

        FileViewItem item;
        item.name = f.file_name;
        item.full_path = f.full_path;
        item.size_logical = f.size_bytes;
        item.size_on_disk = f.size_on_disk;
        item.category = f.category;
        item.last_modified = f.last_modified;
        result.push_back(item);
    }

    return result;
}
