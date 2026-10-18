#ifndef NATIVE_UI_H
#define NATIVE_UI_H

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include "model.h"
#include "theme.h"
#include "i18n.h"

enum ClickRegionType {
    CLICK_NONE = 0,
    CLICK_DRIVE,
    CLICK_CHOOSE_FOLDER,
    CLICK_RESCAN,
    CLICK_CANCEL_SCAN,
    CLICK_TOGGLE_SIZE_MODE,
    CLICK_TOGGLE_REDUNDANT,
    CLICK_TOGGLE_THEME,
    CLICK_TOGGLE_LANG,
    CLICK_TAB,
    CLICK_CATEGORY_CARD,
    CLICK_SIZE_PILL,
    CLICK_CLEAR_CATEGORY_FILTER,
    CLICK_OPEN_FILE_EXPLORER,
    CLICK_OPEN_FOLDER_EXPLORER,
    CLICK_SCAN_SUBFOLDER,
    CLICK_COPY_PATH
};

struct ClickableRegion {
    Gdiplus::Rect rect;
    ClickRegionType type;
    int id;
    std::wstring extra_data;
};

enum AppTab {
    TAB_OVERVIEW = 0,
    TAB_TOP_FILES = 1,
    TAB_TOP_FOLDERS = 2,
    TAB_ALL_FILES = 3
};

class NativeUI {
public:
    NativeUI();
    ~NativeUI();

    bool Create(HINSTANCE hInstance, int nCmdShow);
    void Destroy();
    HWND GetHWND() const { return m_hwnd; }

    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void InitControls();
    void DetectDrives();
    void StartScan(const wchar_t *path);

    // Renderização GDI+
    void OnPaint(HDC hdc, int width, int height);
    void RenderHeader(Gdiplus::Graphics &g, int width);
    void RenderTabs(Gdiplus::Graphics &g, int width);
    void RenderScanningProgress(Gdiplus::Graphics &g, int width);

    void RenderTabOverview(Gdiplus::Graphics &g, int width, int startY, int height);
    void RenderTabTopFiles(Gdiplus::Graphics &g, int width, int startY, int height);
    void RenderTabTopFolders(Gdiplus::Graphics &g, int width, int startY, int height);
    void RenderTabAllFiles(Gdiplus::Graphics &g, int width, int startY, int height);

    // Eventos do Mouse
    void OnLButtonDown(int x, int y);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(int delta);
    void OnChooseFolder();

private:
    HINSTANCE m_hInstance;
    HWND m_hwnd;
    HWND m_hSearchEdit;

    // Estado de Navegação e Filtros
    AppTab m_active_tab;
    DisplaySizeMode m_size_mode;        // Tamanho Real (Bytes) vs Tamanho em Disco (Clusters)
    bool m_hide_redundant_folders;      // Ocultar subpastas subsequentes de pastas pai listadas
    int m_filter_category;              // -1: todas, 0 a 7: categoria específica
    uint64_t m_filter_min_size;         // Filtro por tamanho mínimo em bytes
    wchar_t m_search_query[128];

    // Scroll
    int m_scroll_y;
    int m_max_scroll_y;

    // Regiões Clicáveis e Hover
    std::vector<ClickableRegion> m_click_regions;
    int m_hovered_region_id;
    POINT m_last_mouse_pos;

    // Discos Detectados
    std::vector<std::wstring> m_drives;

    // Timer de Varredura
    UINT_PTR m_timer_id;
    bool m_was_scanning;

    // Persistent GDI+ Backbuffer (elimina 46 MB/s de alocações)
    Gdiplus::Bitmap *m_pBackBuffer;
    int m_backBufferWidth;
    int m_backBufferHeight;
};

extern NativeUI g_ui;

#endif // NATIVE_UI_H
