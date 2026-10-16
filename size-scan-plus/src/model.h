#ifndef MODEL_H
#define MODEL_H

#include <windows.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

#include "../../types.h"
#include "scanner_fast.h"

enum DisplaySizeMode {
    SIZE_LOGICAL = 0,   // Tamanho Real (Bytes)
    SIZE_ON_DISK = 1    // Tamanho em Disco (Alocação em Clusters)
};

// Item de visualização de arquivos - LEVE (40 bytes, zero alocações dinâmicas de string)
struct FileViewItem {
    const wchar_t *name;
    const wchar_t *full_path;
    uint64_t size_logical;
    uint64_t size_on_disk;
    FileCategory category;
    FILETIME last_modified;
};

// Item de visualização de pastas - LEVE (40 bytes, zero alocações dinâmicas de string)
struct FolderViewItem {
    const wchar_t *folder_path;
    const wchar_t *display_name;
    uint64_t size_logical;
    uint64_t size_on_disk;
    uint32_t file_count;
    uint32_t subfolder_count;
    bool is_redundant_child;
};

// Snapshot para o cache persistente (apenas compartilha o ponteiro do FastScanResult com 0 bytes clonados)
struct ScanSnapshot {
    std::wstring target_path;
    std::shared_ptr<FastScanResult> scan_data;
    bool is_complete;
};

class ModelManager {
public:
    ModelManager();
    ~ModelManager();

    bool SelectOrStartScan(const wchar_t *path, bool force_rescan = false);
    bool StartScan(const wchar_t *path) { return SelectOrStartScan(path, false); }
    bool RescanCurrent() { return SelectOrStartScan(m_target_path, true); }

    void CancelScan();
    void Shutdown();
    bool IsScanning() const { return m_is_scanning.load(); }
    bool HasResult() const;
    bool IsCached(const wchar_t *path);

    const wchar_t* GetCurrentPath() const { return m_target_path; }
    std::wstring GetCurrentScannedFolderSafe() const;
    void SetCurrentScannedFolder(const wchar_t *folder);

    uint64_t GetDiskTotal() const;
    uint64_t GetDiskFree() const;
    uint64_t GetDiskUsed() const;
    uint64_t GetTotalScannedLogical() const;
    uint64_t GetTotalScannedOnDisk() const;
    uint64_t GetTotalFiles() const;
    uint64_t GetTotalFolders() const;
    uint32_t GetClusterSize() const;
    float GetScanProgressPct() const;

    // Retorna as pastas com filtro inteligente de não-redundância (apenas até max_count itens, ultra-rápido)
    std::vector<FolderViewItem> GetFolders(bool hide_redundant, DisplaySizeMode size_mode, int sort_col, bool sort_desc, const wchar_t *search_query = nullptr, size_t max_count = 100);

    // Retorna os arquivos filtrados de forma ultra-otimizada
    std::vector<FileViewItem> GetFiles(uint64_t min_size, int category_filter, DisplaySizeMode size_mode, int sort_col, bool sort_desc, const wchar_t *search_query = nullptr, size_t max_count = 0);

    uint64_t GetCategoryTotal(int cat, DisplaySizeMode size_mode) const;
    uint64_t GetCategoryFileCount(int cat) const;

    static std::wstring NormalizePath(const wchar_t *p);
    static std::wstring FormatBytes(uint64_t bytes);
    static uint64_t CalculateSizeOnDisk(uint64_t logical_size, uint32_t cluster_size);

private:
    void ResetActiveState();
    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    void ExecuteScanThread();

private:
    std::shared_ptr<FastScanResult> m_current_scan;
    std::atomic<bool> m_is_scanning;
    std::atomic<bool> m_cancel_flag;
    HANDLE m_hWorkerThread;
    mutable std::mutex m_data_mutex;
    mutable std::mutex m_folder_mutex;

    wchar_t m_target_path[MAX_PATH_EXTENDED];
    std::wstring m_current_folder;

    // Cache por partição / diretório escaneado (armazena shared_ptr sem duplicar memória!)
    std::map<std::wstring, std::shared_ptr<ScanSnapshot>> m_scan_cache;
};

extern ModelManager g_model;

#endif // MODEL_H
