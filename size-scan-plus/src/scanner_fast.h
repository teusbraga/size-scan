#ifndef SCANNER_FAST_H
#define SCANNER_FAST_H

#include <windows.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <algorithm>
#include "../../types.h"

// Callback para progresso de varredura seguro entre threads
typedef void (*ScanProgressFn)(const wchar_t *current_folder, void *user_data);

// StringPool de altíssima performance (alocação em blocos contíguos de 2 MB)
class StringPool {
public:
    struct Chunk {
        Chunk *next;
        size_t capacity;
        size_t used;
        wchar_t data[1];
    };

    StringPool(size_t chunkSize = 1024 * 1024)
        : m_head(nullptr), m_current(nullptr), m_chunkSize(chunkSize) {}

    ~StringPool() {
        Clear();
    }

    StringPool(const StringPool&) = delete;
    StringPool& operator=(const StringPool&) = delete;

    StringPool(StringPool&& other) noexcept
        : m_head(other.m_head), m_current(other.m_current), m_chunkSize(other.m_chunkSize) {
        other.m_head = nullptr;
        other.m_current = nullptr;
    }

    StringPool& operator=(StringPool&& other) noexcept {
        if (this != &other) {
            Clear();
            m_head = other.m_head;
            m_current = other.m_current;
            m_chunkSize = other.m_chunkSize;
            other.m_head = nullptr;
            other.m_current = nullptr;
        }
        return *this;
    }

    void Clear() {
        Chunk *c = m_head;
        while (c) {
            Chunk *nxt = c->next;
            free(c);
            c = nxt;
        }
        m_head = nullptr;
        m_current = nullptr;
    }

    const wchar_t* AddString(const wchar_t *str, size_t len) {
        if (!str || len == 0) return L"";
        size_t needed = len + 1;
        if (!m_current || (m_current->used + needed > m_current->capacity)) {
            size_t cap = (needed > m_chunkSize) ? needed : m_chunkSize;
            size_t totalBytes = sizeof(Chunk) + (cap - 1) * sizeof(wchar_t);
            Chunk *c = (Chunk*)malloc(totalBytes);
            if (!c) return L"";
            c->next = nullptr;
            c->capacity = cap;
            c->used = 0;
            if (!m_head) {
                m_head = c;
            } else {
                m_current->next = c;
            }
            m_current = c;
        }

        wchar_t *dest = m_current->data + m_current->used;
        memcpy(dest, str, len * sizeof(wchar_t));
        dest[len] = L'\0';
        m_current->used += needed;
        return dest;
    }

    const wchar_t* AddString(const wchar_t *str) {
        if (!str) return L"";
        return AddString(str, wcslen(str));
    }

private:
    Chunk *m_head;
    Chunk *m_current;
    size_t m_chunkSize;
};

// Registro ultra-compacto de arquivo
struct FastFileRecord {
    const wchar_t *full_path;
    const wchar_t *file_name;
    uint64_t size_bytes;
    uint64_t size_on_disk;
    FILETIME last_modified;
    FileCategory category;
};

// Registro ultra-compacto de pasta
struct FastFolderRecord {
    const wchar_t *folder_path;
    const wchar_t *display_name;
    uint64_t cumulative_size;
    uint32_t file_count;
    uint32_t subfolder_count;
    bool is_redundant_child;
};

// Resultado completo da varredura ultraleve
struct FastScanResult {
    wchar_t root_path[MAX_PATH_EXTENDED];
    uint32_t cluster_size;
    uint64_t disk_total_bytes;
    uint64_t disk_free_bytes;
    uint64_t disk_used_bytes;
    uint64_t total_scanned_bytes;
    uint64_t total_scanned_on_disk;
    uint64_t total_files;
    uint64_t total_folders;
    volatile float scan_progress_pct;

    uint64_t cat_size[CAT_COUNT];
    uint64_t cat_size_on_disk[CAT_COUNT];
    uint64_t cat_count[CAT_COUNT];

    StringPool pool;
    std::vector<FastFileRecord> files;
    std::vector<FastFolderRecord> folders;

    FastScanResult() {
        root_path[0] = L'\0';
        cluster_size = 4096;
        disk_total_bytes = 0;
        disk_free_bytes = 0;
        disk_used_bytes = 0;
        total_scanned_bytes = 0;
        total_scanned_on_disk = 0;
        total_files = 0;
        total_folders = 0;
        scan_progress_pct = 0.0f;
        memset(cat_size, 0, sizeof(cat_size));
        memset(cat_size_on_disk, 0, sizeof(cat_size_on_disk));
        memset(cat_count, 0, sizeof(cat_count));
    }
};

class FastScanner {
public:
    static FileCategory ClassifyFile(const wchar_t *filename);
    static uint64_t CalculateSizeOnDisk(uint64_t logical_size, uint32_t cluster_size);
    static uint32_t DetectClusterSize(const wchar_t *root_path);

    static bool Run(
        const wchar_t *root_path,
        FastScanResult &out_result,
        volatile bool *cancel_flag,
        volatile bool *is_running,
        ScanProgressFn progress_fn = nullptr,
        void *user_data = nullptr
    );
};

#endif // SCANNER_FAST_H
