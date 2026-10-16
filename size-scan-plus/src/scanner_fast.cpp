#include "scanner_fast.h"
#include <cwctype>
#include <cstdio>
#include <cstring>

static const wchar_t *g_ext_video[] = {
    L".mp4", L".mkv", L".avi", L".mov", L".wmv", L".flv", L".webm", L".m4v", L".3gp", L".ts", L".vob", L".mpg", L".mpeg", nullptr
};
static const wchar_t *g_ext_image[] = {
    L".jpg", L".jpeg", L".png", L".gif", L".webp", L".bmp", L".tiff", L".tif", L".svg", L".ico", L".raw", L".psd", L".ai", nullptr
};
static const wchar_t *g_ext_audio[] = {
    L".mp3", L".wav", L".flac", L".aac", L".ogg", L".m4a", L".wma", L".opus", L".mid", L".midi", nullptr
};
static const wchar_t *g_ext_archive[] = {
    L".zip", L".rar", L".7z", L".tar", L".gz", L".bz2", L".xz", L".iso", L".img", L".vmdk", L".vdi", L".cab", L".dmg", nullptr
};
static const wchar_t *g_ext_doc[] = {
    L".pdf", L".docx", L".doc", L".xlsx", L".xls", L".pptx", L".ppt", L".txt", L".csv", L".md", L".rtf", L".epub", L".odt", L".ods", nullptr
};
static const wchar_t *g_ext_exec[] = {
    L".exe", L".msi", L".dll", L".sys", L".bat", L".cmd", L".ps1", L".apk", L".jar", L".bin", L".appx", nullptr
};
static const wchar_t *g_ext_code[] = {
    L".c", L".h", L".cpp", L".hpp", L".cs", L".py", L".js", L".ts", L".jsx", L".tsx", L".html", L".css", L".json", L".xml", L".sql", L".java", L".go", L".rs", L".php", L".yml", L".yaml", L".ini", nullptr
};

static bool ExtInList(const wchar_t *ext, const wchar_t **list) {
    if (!ext || !list) return false;
    for (int i = 0; list[i] != nullptr; i++) {
        if (_wcsicmp(ext, list[i]) == 0) return true;
    }
    return false;
}

FileCategory FastScanner::ClassifyFile(const wchar_t *filename) {
    if (!filename) return CAT_OTHER;
    const wchar_t *dot = wcsrchr(filename, L'.');
    if (!dot) return CAT_OTHER;

    if (ExtInList(dot, g_ext_video)) return CAT_VIDEO;
    if (ExtInList(dot, g_ext_image)) return CAT_IMAGE;
    if (ExtInList(dot, g_ext_audio)) return CAT_AUDIO;
    if (ExtInList(dot, g_ext_archive)) return CAT_ARCHIVE;
    if (ExtInList(dot, g_ext_doc)) return CAT_DOC;
    if (ExtInList(dot, g_ext_exec)) return CAT_EXEC;
    if (ExtInList(dot, g_ext_code)) return CAT_CODE;

    return CAT_OTHER;
}

uint64_t FastScanner::CalculateSizeOnDisk(uint64_t logical_size, uint32_t cluster_size) {
    if (logical_size == 0) return 0;
    if (cluster_size == 0) cluster_size = 4096;
    return ((logical_size + cluster_size - 1) / cluster_size) * (uint64_t)cluster_size;
}

static uint64_t GetRealSizeOnDisk(const wchar_t *path, DWORD attributes, uint64_t logical_size, uint32_t cluster_size) {
    if (attributes & (FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_COMPRESSED)) {
        DWORD high = 0;
        DWORD low = GetCompressedFileSizeW(path, &high);
        if (low != INVALID_FILE_SIZE || GetLastError() == NO_ERROR) {
            ULARGE_INTEGER u;
            u.LowPart = low;
            u.HighPart = high;
            return u.QuadPart;
        }
    }
    return FastScanner::CalculateSizeOnDisk(logical_size, cluster_size);
}

uint32_t FastScanner::DetectClusterSize(const wchar_t *root_path) {
    if (!root_path || wcslen(root_path) < 2) return 4096;
    wchar_t root[4] = { root_path[0], L':', L'\\', L'\0' };
    DWORD sectorsPerCluster = 0, bytesPerSector = 0, numberOfFreeClusters = 0, totalNumberOfClusters = 0;
    if (GetDiskFreeSpaceW(root, &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters)) {
        if (sectorsPerCluster > 0 && bytesPerSector > 0) {
            return sectorsPerCluster * bytesPerSector;
        }
    }
    return 4096;
}

#define SCAN_PATH_BUF 2048
#define SCAN_MAX_DEPTH 64

static uint64_t ScanDirectoryInternal(
    FastScanResult &res,
    const wchar_t *current_dir,
    size_t current_dir_len,
    uint32_t *out_files,
    uint32_t *out_folders,
    volatile bool *cancel_flag,
    ScanProgressFn progress_fn,
    void *user_data,
    int depth,
    DWORD &last_progress_tick,
    bool is_partition_root,
    uint32_t top_dir_count,
    uint32_t &current_top_index
) {
    if (cancel_flag && *cancel_flag) return 0;
    if (depth > SCAN_MAX_DEPTH) return 0;

    // Atualiza progresso da UI suavemente a cada 80ms via callback seguro
    DWORD now = GetTickCount();
    if (now - last_progress_tick > 80) {
        last_progress_tick = now;
        if (is_partition_root && res.disk_used_bytes > 0) {
            double p = ((double)res.total_scanned_bytes / (double)res.disk_used_bytes) * 100.0;
            if (p > 99.0) p = 99.0;
            if (p < 0.0) p = 0.0;
            res.scan_progress_pct = (float)p;
        } else if (top_dir_count > 0) {
            float base = ((float)current_top_index / (float)top_dir_count) * 100.0f;
            if (base > 98.0f) base = 98.0f;
            if (base < 0.0f) base = 0.0f;
            res.scan_progress_pct = base;
        }
        if (progress_fn) {
            progress_fn(current_dir, user_data);
        }
    }

    bool has_trailing_slash = (current_dir_len > 0 && 
        (current_dir[current_dir_len - 1] == L'\\' || current_dir[current_dir_len - 1] == L'/'));

    wchar_t search_pattern[SCAN_PATH_BUF];
    if (has_trailing_slash) {
        if (current_dir_len + 2 >= SCAN_PATH_BUF) return 0;
        memcpy(search_pattern, current_dir, current_dir_len * sizeof(wchar_t));
        search_pattern[current_dir_len] = L'*';
        search_pattern[current_dir_len + 1] = L'\0';
    } else {
        if (current_dir_len + 3 >= SCAN_PATH_BUF) return 0;
        memcpy(search_pattern, current_dir, current_dir_len * sizeof(wchar_t));
        search_pattern[current_dir_len] = L'\\';
        search_pattern[current_dir_len + 1] = L'*';
        search_pattern[current_dir_len + 2] = L'\0';
    }

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search_pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }

    uint64_t folder_total_size = 0;
    uint32_t folder_file_count = 0;
    uint32_t folder_sub_count = 0;

    wchar_t item_path[SCAN_PATH_BUF];
    memcpy(item_path, current_dir, current_dir_len * sizeof(wchar_t));
    size_t prefix_len = current_dir_len;
    if (!has_trailing_slash) {
        item_path[current_dir_len] = L'\\';
        prefix_len = current_dir_len + 1;
    }

    do {
        if (cancel_flag && *cancel_flag) break;

        // Skip . and ..
        if (ffd.cFileName[0] == L'.' && 
            (ffd.cFileName[1] == L'\0' || (ffd.cFileName[1] == L'.' && ffd.cFileName[2] == L'\0'))) {
            continue;
        }

        // Avoid junctions, symlinks, mounts
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            continue;
        }

        size_t name_len = wcslen(ffd.cFileName);
        size_t item_len = prefix_len + name_len;
        if (item_len >= SCAN_PATH_BUF) continue;

        memcpy(item_path + prefix_len, ffd.cFileName, (name_len + 1) * sizeof(wchar_t));

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            folder_sub_count++;
            if (depth == 0) {
                current_top_index++;
            }
            uint32_t sub_files = 0, sub_dirs = 0;
            uint64_t sub_size = ScanDirectoryInternal(
                res, item_path, item_len, &sub_files, &sub_dirs,
                cancel_flag, progress_fn, user_data, depth + 1, last_progress_tick,
                is_partition_root, top_dir_count, current_top_index
            );
            folder_total_size += sub_size;
            folder_file_count += sub_files;
            folder_sub_count += sub_dirs;
        } else {
            // File
            ULARGE_INTEGER fsz;
            fsz.LowPart = ffd.nFileSizeLow;
            fsz.HighPart = ffd.nFileSizeHigh;
            uint64_t size = fsz.QuadPart;
            uint64_t size_on_disk = GetRealSizeOnDisk(item_path, ffd.dwFileAttributes, size, res.cluster_size);

            // Aloca o caminho no StringPool
            const wchar_t *saved_path = res.pool.AddString(item_path, item_len);
            const wchar_t *saved_name = saved_path + prefix_len;

            FileCategory cat = FastScanner::ClassifyFile(ffd.cFileName);

            FastFileRecord rec;
            rec.full_path = saved_path;
            rec.file_name = saved_name;
            rec.size_bytes = size;
            rec.size_on_disk = size_on_disk;
            rec.last_modified = ffd.ftLastWriteTime;
            rec.category = cat;

            res.files.push_back(rec);

            res.cat_size[cat] += size;
            res.cat_size_on_disk[cat] += size_on_disk;
            res.cat_count[cat]++;
            res.total_scanned_bytes += size;
            res.total_scanned_on_disk += size_on_disk;
            res.total_files++;

            folder_total_size += size;
            folder_file_count++;
        }
    } while (FindNextFileW(hFind, &ffd) != 0);

    FindClose(hFind);

    if (cancel_flag && *cancel_flag) return 0;

    // Registra a pasta com caminho salvo no pool
    const wchar_t *saved_folder = res.pool.AddString(current_dir, current_dir_len);
    const wchar_t *slash = wcsrchr(saved_folder, L'\\');
    const wchar_t *disp_name = slash ? (slash + 1) : saved_folder;

    FastFolderRecord fRec;
    fRec.folder_path = saved_folder;
    fRec.display_name = disp_name;
    fRec.cumulative_size = folder_total_size;
    fRec.file_count = folder_file_count;
    fRec.subfolder_count = folder_sub_count;
    fRec.is_redundant_child = false;

    res.folders.push_back(fRec);
    res.total_folders++;

    if (out_files) *out_files = folder_file_count;
    if (out_folders) *out_folders = folder_sub_count;

    return folder_total_size;
}

bool FastScanner::Run(
    const wchar_t *root_path,
    FastScanResult &out_result,
    volatile bool *cancel_flag,
    volatile bool *is_running,
    ScanProgressFn progress_fn,
    void *user_data
) {
    if (!root_path || root_path[0] == L'\0') return false;
    if (is_running) *is_running = true;

    out_result = FastScanResult();

    // Normaliza caminho absoluto
    DWORD len = GetFullPathNameW(root_path, MAX_PATH_EXTENDED, out_result.root_path, nullptr);
    if (len == 0 || len >= MAX_PATH_EXTENDED) {
        wcscpy_s(out_result.root_path, MAX_PATH_EXTENDED, root_path);
    }
    size_t root_len = wcslen(out_result.root_path);
    if (root_len > 3 && (out_result.root_path[root_len - 1] == L'\\' || out_result.root_path[root_len - 1] == L'/')) {
        out_result.root_path[root_len - 1] = L'\0';
        root_len--;
    }

    out_result.cluster_size = DetectClusterSize(out_result.root_path);

    // Informações de espaço do disco
    ULARGE_INTEGER free_bytes_avail, total_bytes, total_free_bytes;
    if (GetDiskFreeSpaceExW(out_result.root_path, &free_bytes_avail, &total_bytes, &total_free_bytes)) {
        out_result.disk_total_bytes = total_bytes.QuadPart;
        out_result.disk_free_bytes = free_bytes_avail.QuadPart;
        out_result.disk_used_bytes = (total_bytes.QuadPart >= free_bytes_avail.QuadPart) 
            ? (total_bytes.QuadPart - free_bytes_avail.QuadPart) : 0;
    }

    out_result.files.reserve(65536);
    out_result.folders.reserve(8192);

    DWORD last_progress_tick = 0;
    uint32_t total_files = 0, total_dirs = 0;

    bool is_partition_root = (root_len <= 3);
    uint32_t top_dir_count = 0;
    uint32_t current_top_index = 0;

    if (!is_partition_root) {
        wchar_t top_pattern[SCAN_PATH_BUF];
        bool has_slash = (root_len > 0 && (out_result.root_path[root_len - 1] == L'\\' || out_result.root_path[root_len - 1] == L'/'));
        if (has_slash) {
            swprintf_s(top_pattern, SCAN_PATH_BUF, L"%s*", out_result.root_path);
        } else {
            swprintf_s(top_pattern, SCAN_PATH_BUF, L"%s\\*", out_result.root_path);
        }
        WIN32_FIND_DATAW tfd;
        HANDLE hTop = FindFirstFileW(top_pattern, &tfd);
        if (hTop != INVALID_HANDLE_VALUE) {
            do {
                if (tfd.cFileName[0] == L'.' && 
                   (tfd.cFileName[1] == L'\0' || (tfd.cFileName[1] == L'.' && tfd.cFileName[2] == L'\0')))
                    continue;
                if (tfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    top_dir_count++;
                }
            } while (FindNextFileW(hTop, &tfd));
            FindClose(hTop);
        }
    }

    ScanDirectoryInternal(
        out_result,
        out_result.root_path,
        root_len,
        &total_files,
        &total_dirs,
        cancel_flag,
        progress_fn,
        user_data,
        0,
        last_progress_tick,
        is_partition_root,
        top_dir_count,
        current_top_index
    );

    out_result.scan_progress_pct = 100.0f;

    if (is_running) *is_running = false;

    // Ordenação no vetor plano
    if (out_result.files.size() > 1) {
        std::sort(out_result.files.begin(), out_result.files.end(), [](const FastFileRecord &a, const FastFileRecord &b) {
            return a.size_bytes > b.size_bytes;
        });
    }

    if (out_result.folders.size() > 1) {
        std::sort(out_result.folders.begin(), out_result.folders.end(), [](const FastFolderRecord &a, const FastFolderRecord &b) {
            return a.cumulative_size > b.cumulative_size;
        });
    }

    if (cancel_flag && *cancel_flag) {
        return false;
    }

    return true;
}
