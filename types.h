#ifndef TYPES_H
#define TYPES_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_PATH_EXTENDED 4096
#define SIZESCAN_VERSION "1.0.0"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAT_VIDEO = 0,
    CAT_IMAGE,
    CAT_AUDIO,
    CAT_ARCHIVE,
    CAT_DOC,
    CAT_EXEC,
    CAT_CODE,
    CAT_OTHER,
    CAT_COUNT
} FileCategory;

typedef struct {
    const char *name;
    const char *icon;
    const char *color_ansi;
    const char *html_color;
} CategoryMeta;

typedef struct {
    wchar_t *full_path;
    wchar_t *file_name;
    uint64_t size_bytes;
    FILETIME last_modified;
    FileCategory category;
} FileRecord;

typedef struct {
    wchar_t *folder_path;
    uint64_t cumulative_size;
    uint32_t file_count;
    uint32_t subfolder_count;
} FolderRecord;

typedef struct {
    uint64_t total_size;
    uint64_t file_count;
} CategoryStats;

typedef struct {
    wchar_t root_path[MAX_PATH_EXTENDED];
    
    // Disk info
    uint64_t disk_total_bytes;
    uint64_t disk_free_bytes;
    uint64_t disk_used_bytes;
    
    // Scanned info
    uint64_t total_scanned_bytes;
    uint64_t total_files;
    uint64_t total_folders;
    
    // Arrays
    FileRecord *files;
    size_t files_count;
    size_t files_capacity;
    
    FolderRecord *folders;
    size_t folders_count;
    size_t folders_capacity;
    
    CategoryStats categories[CAT_COUNT];
} ScanResult;

typedef struct {
    uint64_t min_size_bytes;
    uint64_t max_size_bytes;
    int category_filter; // -1 for any
    wchar_t ext_filter[32]; // empty if none
    wchar_t search_query[128]; // empty if none
    int sort_mode; // 0: Size Desc, 1: Size Asc, 2: Name Asc, 3: Date Desc
} FilterCriteria;

extern const CategoryMeta g_categories[CAT_COUNT];

#ifdef __cplusplus
}
#endif

#endif // TYPES_H
