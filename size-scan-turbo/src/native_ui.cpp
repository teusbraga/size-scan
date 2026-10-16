#include "native_ui.h"
#include <shlobj.h>
#include <shellapi.h>
#include <windowsx.h>
#include <algorithm>
#include <stdio.h>

using namespace Gdiplus;

NativeUI g_ui;

static const wchar_t* WINDOW_CLASS_NAME = L"SizeScanPlusModernGDI";

static const wchar_t* CAT_NAMES[8] = {
    L"Vídeos", L"Imagens", L"Áudios", L"Compactados / ISO",
    L"Documentos", L"Programas / Exe", L"Código & Dev", L"Outros"
};

static const wchar_t* CAT_ICONS[8] = {
    L"🎬", L"🖼️", L"🎵", L"📦", L"📄", L"⚙️", L"💻", L"📁"
};

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCS = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCS->lpCreateParams));
    }
    NativeUI* pThis = reinterpret_cast<NativeUI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

NativeUI::NativeUI()
    : m_hInstance(NULL)
    , m_hwnd(NULL)
    , m_hSearchEdit(NULL)
    , m_active_tab(TAB_OVERVIEW)
    , m_size_mode(SIZE_LOGICAL)
    , m_hide_redundant_folders(true) // Padrão: Ocultar subpastas redundantes
    , m_filter_category(-1)          // Todas as categorias
    , m_filter_min_size(0)
    , m_scroll_y(0)
    , m_max_scroll_y(0)
    , m_hovered_region_id(-1)
    , m_timer_id(0)
    , m_pBackBuffer(nullptr)
    , m_backBufferWidth(0)
    , m_backBufferHeight(0)
{
    m_search_query[0] = L'\0';
    m_last_mouse_pos.x = -1;
    m_last_mouse_pos.y = -1;
}

void NativeUI::Destroy() {
    if (m_timer_id && m_hwnd) {
        KillTimer(m_hwnd, m_timer_id);
        m_timer_id = 0;
    }
    if (m_pBackBuffer) {
        delete m_pBackBuffer;
        m_pBackBuffer = nullptr;
    }
}

NativeUI::~NativeUI() {
    Destroy();
}


bool NativeUI::Create(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    HICON hAppIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    if (!hAppIcon) hAppIcon = LoadIcon(NULL, IDI_APPLICATION);
    HICON hAppIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (!hAppIconSm) hAppIconSm = hAppIcon;

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL; // Evita flicker, pintado no WM_PAINT
    wc.hIcon = hAppIcon;
    wc.hIconSm = hAppIconSm;

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    m_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"⚡ SIZE-SCAN TURBO - MFT Direct & x64 Assembly Engine",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1140, 780,
        NULL, NULL, hInstance, this
    );

    if (!m_hwnd) return false;

    // Define o ícone explicitamente na barra de título e na barra de tarefas
    SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hAppIcon);
    SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hAppIconSm);

    // Inicializa o tema do Windows e bordas arredondadas do DWM
    g_theme.Init(m_hwnd);

    InitControls();
    DetectDrives();

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    // Timer para atualização periódica suave da UI durante varredura (80ms = ~12fps)
    m_timer_id = SetTimer(m_hwnd, 1, 80, NULL);

    // Inicia a varredura automática no USERPROFILE por padrão (rápido e sem bloqueios de sistema)
    wchar_t user_dir[MAX_PATH_EXTENDED] = {0};
    GetEnvironmentVariableW(L"USERPROFILE", user_dir, MAX_PATH_EXTENDED);
    if (wcslen(user_dir) > 0) {
        g_model.SelectOrStartScan(user_dir, false);
    } else if (!m_drives.empty()) {
        g_model.SelectOrStartScan(m_drives[0].c_str(), false);
    }

    return true;
}

void NativeUI::DetectDrives() {
    m_drives.clear();
    wchar_t drive_strings[512];
    DWORD len = GetLogicalDriveStringsW(512, drive_strings);
    if (len > 0) {
        wchar_t* p = drive_strings;
        while (*p) {
            UINT type = GetDriveTypeW(p);
            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
                m_drives.push_back(p);
            }
            p += wcslen(p) + 1;
        }
    }
    if (m_drives.empty()) {
        m_drives.push_back(L"C:\\");
    }
}

void NativeUI::InitControls() {
    // Campo de busca nativo
    m_hSearchEdit = CreateWindowExW(
        0, L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        0, 0, 0, 0,
        m_hwnd, (HMENU)1001, m_hInstance, NULL
    );

    SendMessageW(m_hSearchEdit, 0x1501 /* EM_SETCUEBANNER */, TRUE,
        (LPARAM)TR(STR_SEARCH_CUE));

    HFONT hFont = CreateFontW(
        15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    SendMessageW(m_hSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void NativeUI::StartScan(const wchar_t *path) {
    if (!path || path[0] == L'\0') return;
    g_model.StartScan(path);
    m_scroll_y = 0;
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void NativeUI::OnChooseFolder() {
    IFileDialog *pfd = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }
        if (SUCCEEDED(pfd->Show(m_hwnd))) {
            IShellItem *psi = NULL;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath = NULL;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    StartScan(pszPath);
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
}

// -------------------------------------------------------------
// RENDERIZAÇÃO GDI+ COM DOUBLE-BUFFERING
// -------------------------------------------------------------

void NativeUI::OnPaint(HDC hdc, int width, int height) {
    if (width <= 0 || height <= 0) return;

    if (!m_pBackBuffer || m_backBufferWidth != width || m_backBufferHeight != height) {
        delete m_pBackBuffer;
        m_pBackBuffer = new Bitmap(width, height);
        m_backBufferWidth = width;
        m_backBufferHeight = height;
    }

    Graphics g(m_pBackBuffer);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // Limpa a lista de regiões clicáveis para o novo quadro
    m_click_regions.clear();

    // 1. Fundo Geral (#1e2022)
    SolidBrush brushBg(g_theme.GetColors().col_bg);
    g.FillRectangle(&brushBg, 0, 0, width, height);

    // 2. Cabeçalho (#2e3133)
    RenderHeader(g, width);

    // 3. Abas de Navegação (#2e3133)
    RenderTabs(g, width);

    // 4. Barra de Progresso quando escaneando
    int contentY = 162;
    if (g_model.IsScanning()) {
        RenderScanningProgress(g, width);
        contentY += 58;
    }

    // 5. Renderiza a Aba Ativa
    switch (m_active_tab) {
        case TAB_OVERVIEW:
            if (m_hSearchEdit && IsWindowVisible(m_hSearchEdit)) ShowWindow(m_hSearchEdit, SW_HIDE);
            RenderTabOverview(g, width, contentY, height);
            break;
        case TAB_TOP_FILES:
            if (m_hSearchEdit && IsWindowVisible(m_hSearchEdit)) ShowWindow(m_hSearchEdit, SW_HIDE);
            RenderTabTopFiles(g, width, contentY, height);
            break;
        case TAB_TOP_FOLDERS:
            if (m_hSearchEdit && IsWindowVisible(m_hSearchEdit)) ShowWindow(m_hSearchEdit, SW_HIDE);
            RenderTabTopFolders(g, width, contentY, height);
            break;
        case TAB_ALL_FILES:
            RenderTabAllFiles(g, width, contentY, height);
            break;
    }

    // Copia o backBuffer final para o HDC da janela
    Graphics screenG(hdc);
    screenG.DrawImage(m_pBackBuffer, 0, 0);
}

// -------------------------------------------------------------
// PIXEL ART DAS BANDEIRAS (Brasil, EUA/Inglês, Espanha)
// -------------------------------------------------------------
static void DrawPixelArtFlag(Graphics &g, int x, int y, AppLanguage lang) {
    SmoothingMode oldSm = g.GetSmoothingMode();
    g.SetSmoothingMode(SmoothingModeNone);

    switch (lang) {
        case LANG_PT: {
            // Bandeira do Brasil (18x12)
            SolidBrush greenBrush(Color(255, 0, 156, 59));
            g.FillRectangle(&greenBrush, x, y, 18, 12);

            SolidBrush yellowBrush(Color(255, 255, 223, 0));
            Point pts[4] = {
                Point(x + 9, y + 1),
                Point(x + 16, y + 6),
                Point(x + 9, y + 10),
                Point(x + 1, y + 6)
            };
            g.FillPolygon(&yellowBrush, pts, 4);

            SolidBrush blueBrush(Color(255, 0, 39, 118));
            g.FillEllipse(&blueBrush, x + 6, y + 3, 6, 6);

            SolidBrush whiteBrush(Color(255, 255, 255, 255));
            g.FillRectangle(&whiteBrush, x + 7, y + 6, 4, 1);
            g.FillRectangle(&whiteBrush, x + 8, y + 5, 2, 1);
            break;
        }

        case LANG_EN: {
            // Bandeira Americana (US) (18x12)
            SolidBrush whiteBrush(Color(255, 255, 255, 255));
            g.FillRectangle(&whiteBrush, x, y, 18, 12);

            SolidBrush redBrush(Color(255, 179, 25, 66));
            g.FillRectangle(&redBrush, x, y + 0, 18, 2);
            g.FillRectangle(&redBrush, x, y + 4, 18, 2);
            g.FillRectangle(&redBrush, x, y + 8, 18, 2);
            g.FillRectangle(&redBrush, x, y + 11, 18, 1);

            SolidBrush blueCanton(Color(255, 10, 49, 97));
            g.FillRectangle(&blueCanton, x, y, 8, 7);

            g.FillRectangle(&whiteBrush, x + 1, y + 1, 1, 1);
            g.FillRectangle(&whiteBrush, x + 4, y + 1, 1, 1);
            g.FillRectangle(&whiteBrush, x + 6, y + 1, 1, 1);
            g.FillRectangle(&whiteBrush, x + 2, y + 3, 1, 1);
            g.FillRectangle(&whiteBrush, x + 5, y + 3, 1, 1);
            g.FillRectangle(&whiteBrush, x + 1, y + 5, 1, 1);
            g.FillRectangle(&whiteBrush, x + 4, y + 5, 1, 1);
            g.FillRectangle(&whiteBrush, x + 6, y + 5, 1, 1);
            break;
        }

        case LANG_ES: {
            // Bandeira da Espanha (18x12)
            SolidBrush redBrush(Color(255, 170, 21, 27));
            SolidBrush yellowBrush(Color(255, 241, 191, 0));

            g.FillRectangle(&redBrush, x, y, 18, 3);
            g.FillRectangle(&yellowBrush, x, y + 3, 18, 6);
            g.FillRectangle(&redBrush, x, y + 9, 18, 3);

            // Brasão na faixa amarela
            SolidBrush crestRed(Color(255, 170, 21, 27));
            SolidBrush crestGold(Color(255, 255, 255, 255));
            g.FillRectangle(&crestRed, x + 3, y + 4, 3, 4);
            g.FillRectangle(&crestGold, x + 4, y + 5, 1, 2);
            g.FillRectangle(&crestRed, x + 3, y + 3, 3, 1);
            break;
        }
    }

    // Contorno fino sutil para acabamento visual perfeito
    Pen borderPen(Color(120, 0, 0, 0), 1.0f);
    g.DrawRectangle(&borderPen, x, y, 17, 11);

    g.SetSmoothingMode(oldSm);
}

void NativeUI::RenderHeader(Graphics &g, int width) {
    const ThemeColors &c = g_theme.GetColors();
    SolidBrush brushSurface(c.col_surface);
    Pen penBorder(c.col_border);

    // Fundo do topo (Altura: 110px)
    g.FillRectangle(&brushSurface, 0, 0, width, 110);
    g.DrawLine(&penBorder, 0, 110, width, 110);

    FontFamily fontFamily(L"Segoe UI");
    Font fontTitle(&fontFamily, 16, FontStyleBold, UnitPixel);
    Font fontSub(&fontFamily, 11, FontStyleRegular, UnitPixel);
    Font fontBtn(&fontFamily, 11, FontStyleBold, UnitPixel);
    Font fontBtnSmall(&fontFamily, 10, FontStyleRegular, UnitPixel);

    SolidBrush brushText(c.col_text);
    SolidBrush brushMuted(c.col_text_muted);
    SolidBrush brushMint(c.col_accent_mint);

    // Ícone da aplicação desenhado no topo esquerdo
    HICON hIcon = (HICON)GetClassLongPtr(m_hwnd, GCLP_HICONSM);
    if (!hIcon) hIcon = (HICON)SendMessage(m_hwnd, WM_GETICON, ICON_SMALL, 0);
    if (hIcon) {
        HDC hdc = g.GetHDC();
        DrawIconEx(hdc, 20, 14, hIcon, 24, 24, 0, NULL, DI_NORMAL);
        g.ReleaseHDC(hdc);
    }

    // Título Principal com Badge TURBO
    g.DrawString(L"SIZE-SCAN TURBO", -1, &fontTitle, PointF(52, 12), &brushText);

    // Badge TURBO MFT + ASM
    {
        Rect badgeRect(215, 12, 135, 20);
        SolidBrush badgeBg(Color(40, 0, 230, 160));
        Pen badgePen(c.col_accent_mint);
        ThemeManager::DrawRoundedRect(g, badgePen, badgeBg, badgeRect, 4);
        Font fontBadge(&fontFamily, 9, FontStyleBold, UnitPixel);
        StringFormat sfBadge;
        sfBadge.SetAlignment(StringAlignmentCenter);
        sfBadge.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(L"⚡ MFT + x64 ASM", -1, &fontBadge, RectF((REAL)badgeRect.X, (REAL)badgeRect.Y, (REAL)badgeRect.Width, (REAL)badgeRect.Height), &sfBadge, &brushMint);
    }

    // Subtítulo com Diretório Atual
    std::wstring targetStr = std::wstring(TR(STR_APP_SUBTITLE_PREFIX)) + std::wstring(g_model.GetCurrentPath());
    if (targetStr.length() > 65) {
        targetStr = targetStr.substr(0, 62) + L"...";
    }
    g.DrawString(targetStr.c_str(), -1, &fontSub, PointF(52, 36), &brushMuted);

    // Botão Alternar Modo Claro / Modo Escuro (Canto superior direito)
    int themeBtnW = 135;
    int themeBtnX = width - themeBtnW - 20;
    int themeBtnY = 14;
    {
        Rect themeBtnRect(themeBtnX, themeBtnY, themeBtnW, 30);

        SolidBrush btnBg(c.col_card);
        Pen btnPen(c.col_border);
        ThemeManager::DrawRoundedRect(g, btnPen, btnBg, themeBtnRect, 6);

        const wchar_t* themeLabel = (g_theme.GetThemeMode() == THEME_DARK) ? TR(STR_THEME_LIGHT) : TR(STR_THEME_DARK);
        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(themeLabel, -1, &fontBtnSmall, RectF((REAL)themeBtnRect.X, (REAL)themeBtnRect.Y, (REAL)themeBtnRect.Width, (REAL)themeBtnRect.Height), &sf, &brushText);

        ClickableRegion reg = { themeBtnRect, CLICK_TOGGLE_THEME, 104, L"" };
        m_click_regions.push_back(reg);
    }

    // Botão Alternar Idioma (PT / EN / ES) ao lado do botão de tema com Pixel Art
    {
        int langBtnW = 84;
        int langBtnX = themeBtnX - langBtnW - 10;
        int langBtnY = 14;
        Rect langBtnRect(langBtnX, langBtnY, langBtnW, 30);

        SolidBrush btnBg(c.col_card);
        Pen btnPen(c.col_border);
        ThemeManager::DrawRoundedRect(g, btnPen, btnBg, langBtnRect, 6);

        // Bandeirinha em Pixel Art (18x12)
        int flagX = langBtnRect.X + 11;
        int flagY = langBtnRect.Y + 9;
        DrawPixelArtFlag(g, flagX, flagY, g_i18n.GetLanguage());

        // Sigla do idioma (PT / EN / ES) com Segoe UI Negrito
        g.DrawString(g_i18n.GetLangCode(), -1, &fontBtn, PointF((REAL)(langBtnRect.X + 37), (REAL)(langBtnRect.Y + 7)), &brushText);

        ClickableRegion reg = { langBtnRect, CLICK_TOGGLE_LANG, 105, L"" };
        m_click_regions.push_back(reg);
    }

    // Botões de Ação e Discos (Linha 1 de botões)
    int btnX = 20;
    int btnY = 64;

    // Discos do Sistema
    for (size_t i = 0; i < m_drives.size() && i < 5; i++) {
        std::wstring driveLabel = std::wstring(TR(STR_DRIVE_PREFIX)) + m_drives[i];
        Rect btnRect(btnX, btnY, 84, 32);

        bool isCurrent = (_wcsicmp(m_drives[i].c_str(), g_model.GetCurrentPath()) == 0);
        SolidBrush btnBg(isCurrent ? c.col_card_hover : c.col_card);
        Pen btnPen(isCurrent ? c.col_accent_mint : c.col_border);
        ThemeManager::DrawRoundedRect(g, btnPen, btnBg, btnRect, 6);

        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(driveLabel.c_str(), -1, &fontBtn, RectF((REAL)btnRect.X, (REAL)btnRect.Y, (REAL)btnRect.Width, (REAL)btnRect.Height), &sf, isCurrent ? &brushMint : &brushText);

        ClickableRegion reg = { btnRect, CLICK_DRIVE, (int)i, m_drives[i] };
        m_click_regions.push_back(reg);

        btnX += 92;
    }

    // Botão Escolher Pasta...
    {
        Rect btnRect(btnX, btnY, 130, 32);
        SolidBrush btnBg(c.col_card);
        Pen btnPen(c.col_border);
        ThemeManager::DrawRoundedRect(g, btnPen, btnBg, btnRect, 6);

        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(TR(STR_CHOOSE_FOLDER), -1, &fontBtn, RectF((REAL)btnRect.X, (REAL)btnRect.Y, (REAL)btnRect.Width, (REAL)btnRect.Height), &sf, &brushText);

        ClickableRegion reg = { btnRect, CLICK_CHOOSE_FOLDER, 100, L"" };
        m_click_regions.push_back(reg);
        btnX += 138;
    }

    // Botão Re-escanear
    {
        Rect btnRect(btnX, btnY, 105, 32);
        SolidBrush btnBg(c.col_card);
        Pen btnPen(c.col_border);
        ThemeManager::DrawRoundedRect(g, btnPen, btnBg, btnRect, 6);

        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(TR(STR_RESCAN), -1, &fontBtn, RectF((REAL)btnRect.X, (REAL)btnRect.Y, (REAL)btnRect.Width, (REAL)btnRect.Height), &sf, &brushText);

        ClickableRegion reg = { btnRect, CLICK_RESCAN, 101, L"" };
        m_click_regions.push_back(reg);
        btnX += 113;
    }

    // Botões dos Recursos Solicitados (Lado Direito):
    // 1. Alternador de Tamanho Real vs. Em Disco (Clusters)
    int rightBtnX = width - 20;
    {
        int w = 210;
        rightBtnX -= w;
        Rect btnRect(rightBtnX, btnY, w, 32);
        bool isDiskMode = (m_size_mode == SIZE_ON_DISK);
        SolidBrush btnBg(isDiskMode ? c.col_card_hover : c.col_card);
        Pen btnPen(isDiskMode ? c.col_accent_mint : c.col_border);
        ThemeManager::DrawRoundedRect(g, btnPen, btnBg, btnRect, 6);

        const wchar_t* label = isDiskMode ? TR(STR_MODE_ON_DISK) : TR(STR_MODE_REAL_SIZE);
        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(label, -1, &fontBtnSmall, RectF((REAL)btnRect.X, (REAL)btnRect.Y, (REAL)btnRect.Width, (REAL)btnRect.Height), &sf, isDiskMode ? &brushMint : &brushText);

        ClickableRegion reg = { btnRect, CLICK_TOGGLE_SIZE_MODE, 102, L"" };
        m_click_regions.push_back(reg);
        rightBtnX -= 10;
    }

    // 2. Alternador de Filtro de Subpastas Redundantes
    {
        int w = 220;
        rightBtnX -= w;
        Rect btnRect(rightBtnX, btnY, w, 32);
        SolidBrush btnBg(m_hide_redundant_folders ? c.col_card_hover : c.col_card);
        Pen btnPen(m_hide_redundant_folders ? c.col_accent_mint : c.col_border);
        ThemeManager::DrawRoundedRect(g, btnPen, btnBg, btnRect, 6);

        const wchar_t* label = m_hide_redundant_folders ? TR(STR_SUBFOLDERS_HIDDEN) : TR(STR_SUBFOLDERS_VISIBLE);
        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(label, -1, &fontBtnSmall, RectF((REAL)btnRect.X, (REAL)btnRect.Y, (REAL)btnRect.Width, (REAL)btnRect.Height), &sf, m_hide_redundant_folders ? &brushMint : &brushText);

        ClickableRegion reg = { btnRect, CLICK_TOGGLE_REDUNDANT, 103, L"" };
        m_click_regions.push_back(reg);
    }
}

void NativeUI::RenderTabs(Graphics &g, int width) {
    const ThemeColors &c = g_theme.GetColors();
    int tabY = 111;
    SolidBrush brushTabBg(c.col_surface);
    g.FillRectangle(&brushTabBg, 0, tabY, width, 44);

    Pen penLine(c.col_border);
    g.DrawLine(&penLine, 0, tabY + 44, width, tabY + 44);

    FontFamily fontFamily(L"Segoe UI");
    Font fontTab(&fontFamily, 12, FontStyleBold, UnitPixel);
    SolidBrush brushActive(c.col_accent_mint);
    SolidBrush brushInactive(c.col_text_muted);

    const wchar_t *tabNames[] = {
        TR(STR_TAB_OVERVIEW),
        TR(STR_TAB_TOP_FILES),
        TR(STR_TAB_TOP_FOLDERS),
        TR(STR_TAB_ALL_FILES)
    };

    int tabX = 20;
    for (int i = 0; i < 4; i++) {
        int tabW = 200;
        Rect tabRect(tabX, tabY + 4, tabW, 36);

        bool isActive = (m_active_tab == (AppTab)i);
        if (isActive) {
            SolidBrush brushActiveBg(c.col_card);
            Pen penActiveBorder(c.col_border);
            ThemeManager::DrawRoundedRect(g, penActiveBorder, brushActiveBg, tabRect, 6);

            // Indicador inferior em verde-menta dessaturado (#77c9b5)
            Pen penIndicator(c.col_accent_mint, 3.0f);
            g.DrawLine(&penIndicator, tabX + 16, tabY + 42, tabX + tabW - 16, tabY + 42);
        }

        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(tabNames[i], -1, &fontTab, RectF((REAL)tabRect.X, (REAL)tabRect.Y, (REAL)tabRect.Width, (REAL)tabRect.Height), &sf, isActive ? &brushActive : &brushInactive);

        ClickableRegion reg = { tabRect, CLICK_TAB, i, L"" };
        m_click_regions.push_back(reg);

        tabX += tabW + 12;
    }
}

void NativeUI::RenderScanningProgress(Graphics &g, int width) {
    const ThemeColors &c = g_theme.GetColors();
    Rect barArea(20, 160, width - 40, 52);
    SolidBrush bgBrush(c.col_card);
    Pen borderPen(c.col_accent_mint);
    ThemeManager::DrawRoundedRect(g, borderPen, bgBrush, barArea, 6);

    FontFamily fontFamily(L"Segoe UI");
    Font fontText(&fontFamily, 12, FontStyleRegular, UnitPixel);
    Font fontBold(&fontFamily, 12, FontStyleBold, UnitPixel);
    SolidBrush textBrush(c.col_text);
    SolidBrush mintBrush(c.col_accent_mint);

    // Leitura 100% thread-safe do caminho atual escaneado (evita crash 0xc0000005)
    std::wstring folderStr = std::wstring(TR(STR_SCANNING_PREFIX)) + g_model.GetCurrentScannedFolderSafe();
    if (folderStr.length() > 58) {
        folderStr = folderStr.substr(0, 55) + L"...";
    }
    g.DrawString(folderStr.c_str(), -1, &fontText, PointF(35, 168), &textBrush);

    // Percentual exato de progresso calculado
    float pct = g_model.GetScanProgressPct();
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    wchar_t pctBuf[32];
    swprintf_s(pctBuf, 32, L"%.1f%%", pct);

    uint64_t totalBytes = (m_size_mode == SIZE_ON_DISK) ? 
        g_model.GetTotalScannedOnDisk() : g_model.GetTotalScannedLogical();
    std::wstring statsStr = std::wstring(pctBuf) + L"  |  " + std::to_wstring(g_model.GetTotalFiles()) + L" " + TR(STR_FILES_LABEL) + L" (" + ModelManager::FormatBytes(totalBytes) + L")";
    g.DrawString(statsStr.c_str(), -1, &fontBold, PointF((REAL)(width - 330), 168.0f), &mintBrush);

    // Botão Parar
    Rect cancelRect(width - 95, 165, 65, 26);
    SolidBrush cancelBg(c.col_card_hover);
    Pen cancelBorder(c.col_accent_red);
    ThemeManager::DrawRoundedRect(g, cancelBorder, cancelBg, cancelRect, 4);

    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    SolidBrush redBrush(c.col_accent_red);
    g.DrawString(TR(STR_STOP), -1, &fontBold, RectF((REAL)cancelRect.X, (REAL)cancelRect.Y, (REAL)cancelRect.Width, (REAL)cancelRect.Height), &sf, &redBrush);

    ClickableRegion reg = { cancelRect, CLICK_CANCEL_SCAN, 204, L"" };
    m_click_regions.push_back(reg);

    // Barra de progresso determinada com preenchimento em % e efeito shimmer suave
    int trackX = barArea.X + 16;
    int trackY = barArea.Y + 41;
    int trackW = barArea.Width - 32;
    int trackH = 5;

    SolidBrush trackBg(c.col_border);
    Rect trackRect(trackX, trackY, trackW, trackH);
    g.FillRectangle(&trackBg, trackRect);

    // Preenchimento proporcional real em percentual
    int fillW = (int)((trackW * (double)pct) / 100.0);
    if (fillW < 4 && pct > 0.0f) fillW = 4;
    if (fillW > trackW) fillW = trackW;

    if (fillW > 0) {
        Rect fillRect(trackX, trackY, fillW, trackH);
        SolidBrush fillBrush(c.col_accent_mint);
        g.FillRectangle(&fillBrush, fillRect);

        // Pulso animado derivado de GetTickCount64 dentro da barra preenchida
        int pulseW = 100;
        ULONGLONG ticks = GetTickCount64();
        int animOffset = (int)((ticks / 2) % (fillW + pulseW)) - pulseW;

        int indicatorX = trackX + animOffset;
        int drawX = (indicatorX < trackX) ? trackX : indicatorX;
        int drawRight = indicatorX + pulseW;
        if (drawRight > trackX + fillW) drawRight = trackX + fillW;

        if (drawRight > drawX) {
            Rect indicatorRect(drawX, trackY, drawRight - drawX, trackH);
            SolidBrush pulseSolid(Color(130, 255, 255, 255));
            g.FillRectangle(&pulseSolid, indicatorRect);
        }
    }
}

// -------------------------------------------------------------
// ABA 0: VISÃO GERAL (Donut Chart + Categorias + Capacidade)
// -------------------------------------------------------------

void NativeUI::RenderTabOverview(Graphics &g, int width, int startY, int height) {
    (void)height;
    const ThemeColors &c = g_theme.GetColors();
    FontFamily fontFamily(L"Segoe UI");
    Font fontCardTitle(&fontFamily, 12, FontStyleBold, UnitPixel);
    Font fontLabel(&fontFamily, 11, FontStyleRegular, UnitPixel);
    Font fontCatName(&fontFamily, 12, FontStyleBold, UnitPixel);

    SolidBrush brushText(c.col_text);
    SolidBrush brushMuted(c.col_text_muted);
    SolidBrush brushCard(c.col_card);
    Pen penBorder(c.col_border);

    int curY = startY;

    // 1. CARD DE CAPACIDADE DO DISCO
    Rect gaugeCard(20, curY, width - 40, 96);
    ThemeManager::DrawRoundedRect(g, penBorder, brushCard, gaugeCard, 8);

    g.DrawString(TR(STR_DISK_CAPACITY), -1, &fontCardTitle, PointF(35, (REAL)(curY + 12)), &brushText);

    uint64_t diskTotal = g_model.GetDiskTotal();
    uint64_t diskUsed = g_model.GetDiskUsed();
    uint64_t diskFree = g_model.GetDiskFree();
    double used_pct = (diskTotal > 0) ? ((double)diskUsed / (double)diskTotal) * 100.0 : 0.0;

    std::wstring usedStr = ModelManager::FormatBytes(diskUsed) + TR(STR_USED_OF) + std::to_wstring((int)used_pct) + L"%)";
    std::wstring freeStr = ModelManager::FormatBytes(diskFree) + TR(STR_FREE_OF) + ModelManager::FormatBytes(diskTotal) + 
        TR(STR_CLUSTER_LABEL) + std::to_wstring(g_model.GetClusterSize()) + L" B]";

    g.DrawString(usedStr.c_str(), -1, &fontLabel, PointF(35, (REAL)(curY + 34)), &brushMuted);
    g.DrawString(freeStr.c_str(), -1, &fontLabel, PointF((REAL)(width - 320), (REAL)(curY + 34)), &brushMuted);

    // Barra segmentada de capacidade
    int barX = 35;
    int barY = curY + 56;
    int barW = width - 70;
    int barH = 20;

    SolidBrush barBg(Color(255, 30, 32, 34));
    g.FillRectangle(&barBg, barX, barY, barW, barH);

    int currentSegX = barX;
    uint64_t totalScanned = (m_size_mode == SIZE_ON_DISK) ? 
        g_model.GetTotalScannedOnDisk() : g_model.GetTotalScannedLogical();

    if (totalScanned > 0 && diskTotal > 0) {
        for (int i = 0; i < CAT_COUNT; i++) {
            uint64_t catSz = g_model.GetCategoryTotal(i, m_size_mode);
            if (catSz > 0) {
                double ratio = (double)catSz / (double)diskTotal;
                int segW = (int)(ratio * barW);
                if (segW < 2) segW = 2;
                if (currentSegX + segW > barX + barW) segW = (barX + barW) - currentSegX;

                SolidBrush segBrush(c.cat_colors[i]);
                g.FillRectangle(&segBrush, currentSegX, barY, segW, barH);
                currentSegX += segW;
            }
        }
    }

    curY += 110;

    // 2. DONUT CHART (À Esquerda) & CARTÕES DE CATEGORIA (À Direita)
    int chartCardW = 340;
    int chartCardH = 346;
    Rect chartCard(20, curY, chartCardW, chartCardH);
    ThemeManager::DrawRoundedRect(g, penBorder, brushCard, chartCard, 8);

    g.DrawString(TR(STR_STORAGE_DISTRIBUTION), -1, &fontCardTitle, PointF(35, (REAL)(curY + 12)), &brushText);

    // Desenho do Donut Chart
    int donutDiameter = 200;
    int donutX = 20 + (chartCardW - donutDiameter) / 2;
    int donutY = curY + 45;

    REAL startAngle = 0.0f;
    if (totalScanned > 0) {
        for (int i = 0; i < CAT_COUNT; i++) {
            uint64_t catSz = g_model.GetCategoryTotal(i, m_size_mode);
            if (catSz > 0) {
                REAL sweepAngle = (REAL)(((double)catSz / (double)totalScanned) * 360.0);
                SolidBrush arcBrush(c.cat_colors[i]);
                g.FillPie(&arcBrush, donutX, donutY, donutDiameter, donutDiameter, startAngle, sweepAngle);
                startAngle += sweepAngle;
            }
        }
    } else {
        SolidBrush emptyBrush(Color(255, 46, 49, 51));
        g.FillPie(&emptyBrush, donutX, donutY, donutDiameter, donutDiameter, 0, 360);
    }

    // Furo central do Donut
    int holeDiameter = 124;
    int holeX = donutX + (donutDiameter - holeDiameter) / 2;
    int holeY = donutY + (donutDiameter - holeDiameter) / 2;
    g.FillEllipse(&brushCard, holeX, holeY, holeDiameter, holeDiameter);

    // Texto no Centro do Donut
    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);

    std::wstring totalStr = ModelManager::FormatBytes(totalScanned);
    g.DrawString(totalStr.c_str(), -1, &fontCardTitle, RectF((REAL)holeX, (REAL)holeY - 8, (REAL)holeDiameter, (REAL)holeDiameter / 2), &sfCenter, &brushText);
    g.DrawString(TR(STR_TOTAL_ANALYZED), -1, &fontLabel, RectF((REAL)holeX, (REAL)holeY + 14, (REAL)holeDiameter, (REAL)holeDiameter / 2), &sfCenter, &brushMuted);

    // Legenda no rodapé do cartão de gráfico
    g.DrawString(TR(STR_CLICK_FILTER_HINT), -1, &fontLabel, PointF(30, (REAL)(curY + chartCardH - 26)), &brushMuted);

    // GRID DE CARTÕES DE CATEGORIA (2 Colunas x 4 Linhas)
    int gridX = 20 + chartCardW + 16;
    int gridW = width - gridX - 20;
    int cardColW = (gridW - 14) / 2;
    int cardRowH = 76;

    for (int i = 0; i < CAT_COUNT; i++) {
        int col = i % 2;
        int row = i / 2;
        int cx = gridX + col * (cardColW + 14);
        int cy = curY + row * (cardRowH + 14);

        Rect catCard(cx, cy, cardColW, cardRowH);

        // Hover effect nos cartões
        bool isHovered = (m_hovered_region_id == 300 + i);
        SolidBrush cardBg(isHovered ? c.col_card_hover : c.col_card);
        Pen cardBorder(isHovered ? c.col_accent_mint : c.col_border);
        ThemeManager::DrawRoundedRect(g, cardBorder, cardBg, catCard, 6);

        // Barra de cor lateral da categoria
        SolidBrush catBrush(c.cat_colors[i]);
        g.FillRectangle(&catBrush, cx + 12, cy + 14, 6, 48);

        // Nome da categoria
        g.DrawString(g_i18n.GetCategoryName(i), -1, &fontCatName, PointF((REAL)(cx + 26), (REAL)(cy + 14)), &brushText);

        // Contagem de arquivos
        uint64_t count = g_model.GetCategoryFileCount(i);
        std::wstring countStr = std::to_wstring(count) + TR(STR_SUB_ARQ_LABEL);
        g.DrawString(countStr.c_str(), -1, &fontLabel, PointF((REAL)(cx + 26), (REAL)(cy + 34)), &brushMuted);

        // Tamanho da Categoria & Porcentagem
        uint64_t catSz = g_model.GetCategoryTotal(i, m_size_mode);
        std::wstring szStr = ModelManager::FormatBytes(catSz);
        double pct = (totalScanned > 0) ? (((double)catSz / (double)totalScanned) * 100.0) : 0.0;

        wchar_t pctBuf[32];
        swprintf_s(pctBuf, 32, L"%.1f%%", pct);

        StringFormat sfRight;
        sfRight.SetAlignment(StringAlignmentFar);
        g.DrawString(szStr.c_str(), -1, &fontCatName, RectF((REAL)(cx + cardColW - 140), (REAL)(cy + 14), 125, 20), &sfRight, &catBrush);
        g.DrawString(pctBuf, -1, &fontLabel, RectF((REAL)(cx + cardColW - 75), (REAL)(cy + 34), 60, 20), &sfRight, &brushMuted);

        // Mini barra de proporção
        int miniBarW = cardColW - 40;
        int miniFilled = (int)((pct / 100.0) * miniBarW);
        if (miniFilled > miniBarW) miniFilled = miniBarW;
        SolidBrush miniBg(Color(255, 30, 32, 34));
        g.FillRectangle(&miniBg, cx + 26, cy + 56, miniBarW, 4);
        g.FillRectangle(&catBrush, cx + 26, cy + 56, miniFilled, 4);

        // Registra como região clicável para abrir a lista já filtrada!
        ClickableRegion reg = { catCard, CLICK_CATEGORY_CARD, 300 + i, L"" };
        m_click_regions.push_back(reg);
    }
}

// -------------------------------------------------------------
// ABA 1: TOP ARQUIVOS GRANDES
// -------------------------------------------------------------

void NativeUI::RenderTabTopFiles(Graphics &g, int width, int startY, int height) {
    const ThemeColors &c = g_theme.GetColors();
    FontFamily fontFamily(L"Segoe UI");
    Font fontHeader(&fontFamily, 11, FontStyleBold, UnitPixel);
    Font fontRow(&fontFamily, 11, FontStyleRegular, UnitPixel);
    Font fontRowBold(&fontFamily, 11, FontStyleBold, UnitPixel);
    Font fontBtn(&fontFamily, 10, FontStyleBold, UnitPixel);

    SolidBrush brushText(c.col_text);
    SolidBrush brushMuted(c.col_text_muted);
    SolidBrush brushCard(c.col_card);
    SolidBrush brushMint(c.col_accent_mint);
    Pen penBorder(c.col_border);
    Pen penLine(c.col_border_light);

    // Cabeçalho discreto em caixa alta
    Rect thRect(20, startY, width - 40, 34);
    ThemeManager::DrawRoundedRect(g, penBorder, brushCard, thRect, 6);

    g.DrawString(TR(STR_COL_RANK), -1, &fontHeader, PointF(35, (REAL)(startY + 9)), &brushMuted);
    g.DrawString(TR(STR_COL_TYPE), -1, &fontHeader, PointF(70, (REAL)(startY + 9)), &brushMuted);
    g.DrawString(TR(STR_COL_FILENAME), -1, &fontHeader, PointF(125, (REAL)(startY + 9)), &brushMuted);
    g.DrawString(TR(STR_COL_SIZE), -1, &fontHeader, PointF((REAL)(width - 340), (REAL)(startY + 9)), &brushMuted);
    g.DrawString(TR(STR_COL_ACTIONS), -1, &fontHeader, PointF((REAL)(width - 120), (REAL)(startY + 9)), &brushMuted);

    int rowY = startY + 40 - m_scroll_y;
    int rowH = 40;

    std::vector<FileViewItem> files = g_model.GetFiles(0, -1, m_size_mode, 2 /* tamanho */, true /* desc */, nullptr, 100);
    size_t limit = (files.size() < 100) ? files.size() : 100;

    for (size_t i = 0; i < limit; i++) {
        if (rowY + rowH >= startY + 40 && rowY < height) {
            const auto &f = files[i];
            Rect rowRect(20, rowY, width - 40, rowH);

            bool isHovered = (m_hovered_region_id == 1000 + (int)i);
            SolidBrush rowBg(isHovered ? c.col_card_hover : c.col_card);
            ThemeManager::DrawRoundedRect(g, penBorder, rowBg, rowRect, 6);

            // Rank
            std::wstring rankStr = std::to_wstring(i + 1);
            SolidBrush rankBrush(i < 3 ? c.col_accent_amber : c.col_text_muted);
            g.DrawString(rankStr.c_str(), -1, &fontRowBold, PointF(35, (REAL)(rowY + 12)), &rankBrush);

            // Ícone da Categoria
            int catIdx = (int)f.category;
            const wchar_t *iconW = (catIdx >= 0 && catIdx < CAT_COUNT) ? CAT_ICONS[catIdx] : L"📁";
            SolidBrush catBrush((catIdx >= 0 && catIdx < CAT_COUNT) ? c.cat_colors[catIdx] : c.col_text_muted);
            g.DrawString(iconW, -1, &fontRowBold, PointF(70, (REAL)(rowY + 12)), &catBrush);

            // Nome e Caminho truncado
            std::wstring fname = f.name;
            if (fname.length() > 50) fname = fname.substr(0, 47) + L"...";
            g.DrawString(fname.c_str(), -1, &fontRowBold, PointF(125, (REAL)(rowY + 6)), &brushText);

            std::wstring fpath = f.full_path;
            if (fpath.length() > 70) fpath = fpath.substr(0, 67) + L"...";
            g.DrawString(fpath.c_str(), -1, &fontRow, PointF(125, (REAL)(rowY + 22)), &brushMuted);

            // Tamanho alinhado à direita
            uint64_t sz = (m_size_mode == SIZE_ON_DISK) ? f.size_on_disk : f.size_logical;
            std::wstring szStr = ModelManager::FormatBytes(sz);
            g.DrawString(szStr.c_str(), -1, &fontRowBold, PointF((REAL)(width - 340), (REAL)(rowY + 12)), &brushMint);

            // Botão Abrir Pasta
            Rect btnRect(width - 130, rowY + 8, 90, 24);
            SolidBrush btnBg(c.col_surface);
            ThemeManager::DrawRoundedRect(g, penBorder, btnBg, btnRect, 4);

            StringFormat sf;
            sf.SetAlignment(StringAlignmentCenter);
            sf.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(TR(STR_BTN_OPEN_FOLDER), -1, &fontBtn, RectF((REAL)btnRect.X, (REAL)btnRect.Y, (REAL)btnRect.Width, (REAL)btnRect.Height), &sf, &brushText);

            ClickableRegion reg = { btnRect, CLICK_OPEN_FILE_EXPLORER, 1000 + (int)i, f.full_path };
            m_click_regions.push_back(reg);
        }
        rowY += rowH + 6;
    }

    m_max_scroll_y = (int)(limit * (rowH + 6) - (height - startY - 60));
    if (m_max_scroll_y < 0) m_max_scroll_y = 0;
}

// -------------------------------------------------------------
// ABA 2: PASTAS MAIS PESADAS (Com filtro de não-redundância!)
// -------------------------------------------------------------

void NativeUI::RenderTabTopFolders(Graphics &g, int width, int startY, int height) {
    const ThemeColors &c = g_theme.GetColors();
    FontFamily fontFamily(L"Segoe UI");
    Font fontHeader(&fontFamily, 11, FontStyleBold, UnitPixel);
    Font fontRow(&fontFamily, 11, FontStyleRegular, UnitPixel);
    Font fontRowBold(&fontFamily, 11, FontStyleBold, UnitPixel);
    Font fontBtn(&fontFamily, 10, FontStyleBold, UnitPixel);

    SolidBrush brushText(c.col_text);
    SolidBrush brushMuted(c.col_text_muted);
    SolidBrush brushCard(c.col_card);
    SolidBrush brushMint(c.col_accent_mint);
    Pen penBorder(c.col_border);

    // Cabeçalho da tabela
    Rect thRect(20, startY, width - 40, 34);
    ThemeManager::DrawRoundedRect(g, penBorder, brushCard, thRect, 6);

    g.DrawString(TR(STR_COL_RANK), -1, &fontHeader, PointF(35, (REAL)(startY + 9)), &brushMuted);
    g.DrawString(TR(STR_COL_FOLDER_PATH), -1, &fontHeader, PointF(70, (REAL)(startY + 9)), &brushMuted);
    g.DrawString(TR(STR_COL_SIZE), -1, &fontHeader, PointF((REAL)(width - 380), (REAL)(startY + 9)), &brushMuted);
    g.DrawString(TR(STR_FILES_LABEL), -1, &fontHeader, PointF((REAL)(width - 260), (REAL)(startY + 9)), &brushMuted);
    g.DrawString(TR(STR_COL_ACTIONS), -1, &fontHeader, PointF((REAL)(width - 120), (REAL)(startY + 9)), &brushMuted);

    int rowY = startY + 40 - m_scroll_y;
    int rowH = 40;

    // Obtém a lista de pastas aplicando o filtro de não-redundância!
    std::vector<FolderViewItem> folders = g_model.GetFolders(
        m_hide_redundant_folders,
        m_size_mode,
        1 /* tamanho */,
        true /* desc */,
        nullptr,
        100
    );

    size_t limit = (folders.size() < 100) ? folders.size() : 100;

    for (size_t i = 0; i < limit; i++) {
        if (rowY + rowH >= startY + 40 && rowY < height) {
            const auto &f = folders[i];
            Rect rowRect(20, rowY, width - 40, rowH);

            bool isHovered = (m_hovered_region_id == 2000 + (int)i);
            SolidBrush rowBg(isHovered ? c.col_card_hover : c.col_card);
            ThemeManager::DrawRoundedRect(g, penBorder, rowBg, rowRect, 6);

            // Rank
            std::wstring rankStr = std::to_wstring(i + 1);
            SolidBrush rankBrush(i < 3 ? c.col_accent_amber : c.col_text_muted);
            g.DrawString(rankStr.c_str(), -1, &fontRowBold, PointF(35, (REAL)(rowY + 12)), &rankBrush);

            // Caminho da pasta
            std::wstring fpath = f.folder_path;
            if (fpath.length() > 68) fpath = fpath.substr(0, 65) + L"...";
            g.DrawString(fpath.c_str(), -1, &fontRowBold, PointF(70, (REAL)(rowY + 12)), &brushText);

            // Tamanho
            uint64_t sz = (m_size_mode == SIZE_ON_DISK) ? f.size_on_disk : f.size_logical;
            std::wstring szStr = ModelManager::FormatBytes(sz);
            g.DrawString(szStr.c_str(), -1, &fontRowBold, PointF((REAL)(width - 380), (REAL)(rowY + 12)), &brushMint);

            // Arquivos e Subpastas
            std::wstring counts = std::to_wstring(f.file_count) + TR(STR_COL_COUNT_FILES) + std::to_wstring(f.subfolder_count) + TR(STR_COL_COUNT_SUBS);
            g.DrawString(counts.c_str(), -1, &fontRow, PointF((REAL)(width - 260), (REAL)(rowY + 12)), &brushMuted);

            // Botão Abrir
            Rect btnRect(width - 130, rowY + 8, 90, 24);
            SolidBrush btnBg(c.col_surface);
            ThemeManager::DrawRoundedRect(g, penBorder, btnBg, btnRect, 4);

            StringFormat sf;
            sf.SetAlignment(StringAlignmentCenter);
            sf.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(TR(STR_BTN_OPEN_FOLDER), -1, &fontBtn, RectF((REAL)btnRect.X, (REAL)btnRect.Y, (REAL)btnRect.Width, (REAL)btnRect.Height), &sf, &brushText);

            ClickableRegion reg = { btnRect, CLICK_OPEN_FOLDER_EXPLORER, 2000 + (int)i, f.folder_path };
            m_click_regions.push_back(reg);
        }
        rowY += rowH + 6;
    }

    m_max_scroll_y = (int)(limit * (rowH + 6) - (height - startY - 60));
    if (m_max_scroll_y < 0) m_max_scroll_y = 0;
}

// -------------------------------------------------------------
// ABA 3: TODOS OS ARQUIVOS & FILTROS (Com pills e busca)
// -------------------------------------------------------------

void NativeUI::RenderTabAllFiles(Graphics &g, int width, int startY, int height) {
    const ThemeColors &c = g_theme.GetColors();
    FontFamily fontFamily(L"Segoe UI");
    Font fontHeader(&fontFamily, 11, FontStyleBold, UnitPixel);
    Font fontPill(&fontFamily, 11, FontStyleBold, UnitPixel);
    Font fontRow(&fontFamily, 11, FontStyleRegular, UnitPixel);
    Font fontRowBold(&fontFamily, 11, FontStyleBold, UnitPixel);
    Font fontBtn(&fontFamily, 10, FontStyleBold, UnitPixel);

    SolidBrush brushText(c.col_text);
    SolidBrush brushMuted(c.col_text_muted);
    SolidBrush brushCard(c.col_card);
    SolidBrush brushMint(c.col_accent_mint);
    Pen penBorder(c.col_border);

    // 1. Painel de Filtros Superiores
    int filterCardH = 76;
    Rect filterCard(20, startY, width - 40, filterCardH);
    ThemeManager::DrawRoundedRect(g, penBorder, brushCard, filterCard, 8);

    g.DrawString(TR(STR_FILTER_SIZE_TITLE), -1, &fontHeader, PointF(35, (REAL)(startY + 12)), &brushMuted);

    // Pills de Tamanho
    struct SizePill { const wchar_t *label; uint64_t bytes; int id; };
    SizePill sizePills[6] = {
        { TR(STR_FILTER_ALL), 0, 501 },
        { L"> 10 MB", 10ULL * 1024 * 1024, 502 },
        { L"> 50 MB", 50ULL * 1024 * 1024, 503 },
        { L"> 100 MB", 100ULL * 1024 * 1024, 504 },
        { L"> 500 MB", 500ULL * 1024 * 1024, 505 },
        { L"> 1 GB", 1024ULL * 1024 * 1024, 506 }
    };

    int pillX = 35;
    int pillY = startY + 34;
    for (int i = 0; i < 6; i++) {
        Rect pRect(pillX, pillY, 78, 28);
        bool isSel = (m_filter_min_size == sizePills[i].bytes);

        SolidBrush pBg(isSel ? c.col_card_hover : c.col_surface);
        Pen pBorder(isSel ? c.col_accent_mint : c.col_border);
        ThemeManager::DrawRoundedRect(g, pBorder, pBg, pRect, 6);

        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(sizePills[i].label, -1, &fontPill, RectF((REAL)pRect.X, (REAL)pRect.Y, (REAL)pRect.Width, (REAL)pRect.Height), &sf, isSel ? &brushMint : &brushText);

        ClickableRegion reg = { pRect, CLICK_SIZE_PILL, sizePills[i].id, L"" };
        m_click_regions.push_back(reg);

        pillX += 86;
    }

    // Indicador da Categoria Ativa
    g.DrawString(TR(STR_FILTER_CAT_TITLE), -1, &fontHeader, PointF((REAL)(width - 340), (REAL)(startY + 12)), &brushMuted);

    Rect catFilterRect(width - 340, startY + 34, 150, 28);
    ThemeManager::DrawRoundedRect(g, penBorder, brushCard, catFilterRect, 6);

    std::wstring curCatLabel = TR(STR_FILTER_ALL_CATS);
    if (m_filter_category >= 0 && m_filter_category < CAT_COUNT) {
        curCatLabel = g_i18n.GetCategoryName(m_filter_category);
    }

    StringFormat sfCat;
    sfCat.SetAlignment(StringAlignmentCenter);
    sfCat.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(curCatLabel.c_str(), -1, &fontPill, RectF((REAL)catFilterRect.X, (REAL)catFilterRect.Y, (REAL)catFilterRect.Width, (REAL)catFilterRect.Height), &sfCat, &brushMint);

    // Botão Limpar Filtro de Categoria
    Rect clearRect(width - 180, startY + 34, 140, 28);
    SolidBrush clearBg(c.col_surface);
    ThemeManager::DrawRoundedRect(g, penBorder, clearBg, clearRect, 6);
    g.DrawString(TR(STR_BTN_CLEAR_FILTERS), -1, &fontPill, RectF((REAL)clearRect.X, (REAL)clearRect.Y, (REAL)clearRect.Width, (REAL)clearRect.Height), &sfCat, &brushText);

    ClickableRegion regClear = { clearRect, CLICK_CLEAR_CATEGORY_FILTER, 510, L"" };
    m_click_regions.push_back(regClear);

    // Posiciona o campo de busca nativo
    if (m_hSearchEdit) {
        SetWindowPos(m_hSearchEdit, NULL, 20, startY + filterCardH + 10, width - 40, 30, SWP_SHOWWINDOW);
    }

    // Tabela de Resultados
    int tableStartY = startY + filterCardH + 48;
    Rect thRect(20, tableStartY, width - 40, 34);
    ThemeManager::DrawRoundedRect(g, penBorder, brushCard, thRect, 6);

    const wchar_t *searchPtr = (m_search_query[0] != L'\0') ? m_search_query : nullptr;
    std::vector<FileViewItem> files = g_model.GetFiles(
        m_filter_min_size,
        m_filter_category,
        m_size_mode,
        2 /* tamanho */,
        true /* desc */,
        searchPtr
    );

    std::wstring countHeader = std::wstring(TR(STR_RESULTS_TITLE)) + std::to_wstring(files.size()) + L" " + TR(STR_FILES_LABEL) + L")";
    g.DrawString(countHeader.c_str(), -1, &fontHeader, PointF(35, (REAL)(tableStartY + 9)), &brushMuted);
    g.DrawString(TR(STR_COL_SIZE), -1, &fontHeader, PointF((REAL)(width - 240), (REAL)(tableStartY + 9)), &brushMuted);
    g.DrawString(TR(STR_COL_ACTIONS), -1, &fontHeader, PointF((REAL)(width - 120), (REAL)(tableStartY + 9)), &brushMuted);

    int rowY = tableStartY + 40 - m_scroll_y;
    int rowH = 40;

    // Estado Vazio se a busca não tiver resultados
    if (files.empty()) {
        Rect emptyRect(20, rowY, width - 40, 80);
        ThemeManager::DrawRoundedRect(g, penBorder, brushCard, emptyRect, 6);
        StringFormat sfEmpty;
        sfEmpty.SetAlignment(StringAlignmentCenter);
        sfEmpty.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(TR(STR_EMPTY_SEARCH), -1, &fontRowBold,
            RectF((REAL)emptyRect.X, (REAL)emptyRect.Y, (REAL)emptyRect.Width, (REAL)emptyRect.Height), &sfEmpty, &brushMuted);
        return;
    }

    size_t limit = (files.size() < 100) ? files.size() : 100;

    for (size_t i = 0; i < limit; i++) {
        if (rowY + rowH >= tableStartY + 40 && rowY < height) {
            const auto &f = files[i];
            Rect rowRect(20, rowY, width - 40, rowH);

            bool isHovered = (m_hovered_region_id == 3000 + (int)i);
            SolidBrush rowBg(isHovered ? c.col_card_hover : c.col_card);
            ThemeManager::DrawRoundedRect(g, penBorder, rowBg, rowRect, 6);

            // Ícone
            int catIdx = (int)f.category;
            const wchar_t *iconW = (catIdx >= 0 && catIdx < CAT_COUNT) ? CAT_ICONS[catIdx] : L"📁";
            SolidBrush catBrush((catIdx >= 0 && catIdx < CAT_COUNT) ? c.cat_colors[catIdx] : c.col_text_muted);
            g.DrawString(iconW, -1, &fontRowBold, PointF(35, (REAL)(rowY + 12)), &catBrush);

            // Nome e Caminho
            std::wstring fname = f.name;
            if (fname.length() > 55) fname = fname.substr(0, 52) + L"...";
            g.DrawString(fname.c_str(), -1, &fontRowBold, PointF(70, (REAL)(rowY + 6)), &brushText);

            std::wstring fpath = f.full_path;
            if (fpath.length() > 75) fpath = fpath.substr(0, 72) + L"...";
            g.DrawString(fpath.c_str(), -1, &fontRow, PointF(70, (REAL)(rowY + 22)), &brushMuted);

            // Tamanho
            uint64_t sz = (m_size_mode == SIZE_ON_DISK) ? f.size_on_disk : f.size_logical;
            std::wstring szStr = ModelManager::FormatBytes(sz);
            g.DrawString(szStr.c_str(), -1, &fontRowBold, PointF((REAL)(width - 240), (REAL)(rowY + 12)), &brushMint);

            // Ação Abrir Pasta
            Rect btnRect(width - 130, rowY + 8, 90, 24);
            SolidBrush btnBg(c.col_surface);
            ThemeManager::DrawRoundedRect(g, penBorder, btnBg, btnRect, 4);

            StringFormat sf;
            sf.SetAlignment(StringAlignmentCenter);
            sf.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(TR(STR_BTN_OPEN_FOLDER), -1, &fontBtn, RectF((REAL)btnRect.X, (REAL)btnRect.Y, (REAL)btnRect.Width, (REAL)btnRect.Height), &sf, &brushText);

            ClickableRegion reg = { btnRect, CLICK_OPEN_FILE_EXPLORER, 3000 + (int)i, f.full_path };
            m_click_regions.push_back(reg);
        }
        rowY += rowH + 6;
    }

    m_max_scroll_y = (int)(limit * (rowH + 6) - (height - tableStartY - 80));
    if (m_max_scroll_y < 0) m_max_scroll_y = 0;
}

// -------------------------------------------------------------
// EVENTOS DO MOUSE E CLIQUES
// -------------------------------------------------------------

void NativeUI::OnLButtonDown(int x, int y) {
    for (const auto &reg : m_click_regions) {
        if (reg.rect.Contains(x, y)) {
            switch (reg.type) {
                case CLICK_DRIVE:
                    g_model.SelectOrStartScan(reg.extra_data.c_str(), false);
                    m_scroll_y = 0;
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_CHOOSE_FOLDER:
                    OnChooseFolder();
                    break;
                case CLICK_RESCAN:
                    g_model.RescanCurrent();
                    m_scroll_y = 0;
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_CANCEL_SCAN:
                    g_model.CancelScan();
                    break;
                case CLICK_TOGGLE_SIZE_MODE:
                    m_size_mode = (m_size_mode == SIZE_LOGICAL) ? SIZE_ON_DISK : SIZE_LOGICAL;
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_TOGGLE_REDUNDANT:
                    m_hide_redundant_folders = !m_hide_redundant_folders;
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_TOGGLE_THEME:
                    g_theme.ToggleTheme();
                    if (m_hSearchEdit) InvalidateRect(m_hSearchEdit, NULL, TRUE);
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_TOGGLE_LANG:
                    g_i18n.NextLanguage();
                    if (m_hSearchEdit) {
                        SendMessageW(m_hSearchEdit, 0x1501 /* EM_SETCUEBANNER */, TRUE, (LPARAM)TR(STR_SEARCH_CUE));
                        InvalidateRect(m_hSearchEdit, NULL, TRUE);
                    }
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_TAB:
                    m_active_tab = (AppTab)reg.id;
                    m_scroll_y = 0;
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_CATEGORY_CARD:
                    // Clicar em qualquer cartão abre a lista já filtrada!
                    m_filter_category = reg.id - 300;
                    m_active_tab = TAB_ALL_FILES;
                    m_scroll_y = 0;
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_SIZE_PILL:
                    if (reg.id == 501) m_filter_min_size = 0;
                    else if (reg.id == 502) m_filter_min_size = 10ULL * 1024 * 1024;
                    else if (reg.id == 503) m_filter_min_size = 50ULL * 1024 * 1024;
                    else if (reg.id == 504) m_filter_min_size = 100ULL * 1024 * 1024;
                    else if (reg.id == 505) m_filter_min_size = 500ULL * 1024 * 1024;
                    else if (reg.id == 506) m_filter_min_size = 1024ULL * 1024 * 1024;
                    m_scroll_y = 0;
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_CLEAR_CATEGORY_FILTER:
                    m_filter_category = -1;
                    m_filter_min_size = 0;
                    if (m_hSearchEdit) SetWindowTextW(m_hSearchEdit, L"");
                    m_search_query[0] = L'\0';
                    m_scroll_y = 0;
                    InvalidateRect(m_hwnd, NULL, FALSE);
                    break;
                case CLICK_OPEN_FILE_EXPLORER:
                    if (!reg.extra_data.empty()) {
                        std::wstring clean = reg.extra_data;
                        size_t pos = 0;
                        while ((pos = clean.find(L"\\\\", pos)) != std::wstring::npos) {
                            if (pos > 0 && clean[pos - 1] == L':') {
                                clean.replace(pos, 2, L"\\");
                            } else {
                                pos += 2;
                            }
                        }

                        PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(clean.c_str());
                        if (pidl) {
                            SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
                            ILFree(pidl);
                        } else {
                            std::wstring param = L"/select,\"" + clean + L"\"";
                            ShellExecuteW(NULL, L"open", L"explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
                        }
                    }
                    break;
                case CLICK_OPEN_FOLDER_EXPLORER:
                    if (!reg.extra_data.empty()) {
                        std::wstring clean = reg.extra_data;
                        size_t pos = 0;
                        while ((pos = clean.find(L"\\\\", pos)) != std::wstring::npos) {
                            if (pos > 0 && clean[pos - 1] == L':') {
                                clean.replace(pos, 2, L"\\");
                            } else {
                                pos += 2;
                            }
                        }
                        ShellExecuteW(NULL, L"open", clean.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    }
                    break;
                case CLICK_SCAN_SUBFOLDER:
                    if (!reg.extra_data.empty()) {
                        StartScan(reg.extra_data.c_str());
                    }
                    break;
                case CLICK_COPY_PATH:
                    if (!reg.extra_data.empty() && OpenClipboard(m_hwnd)) {
                        EmptyClipboard();
                        size_t bytes = (reg.extra_data.length() + 1) * sizeof(wchar_t);
                        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                        if (hMem) {
                            memcpy(GlobalLock(hMem), reg.extra_data.c_str(), bytes);
                            GlobalUnlock(hMem);
                            SetClipboardData(CF_UNICODETEXT, hMem);
                        }
                        CloseClipboard();
                    }
                    break;
                default:
                    break;
            }
            return;
        }
    }
}

void NativeUI::OnMouseMove(int x, int y) {
    int old_hover = m_hovered_region_id;
    m_hovered_region_id = -1;

    for (const auto &reg : m_click_regions) {
        if (reg.rect.Contains(x, y)) {
            m_hovered_region_id = reg.id;
            break;
        }
    }

    if (old_hover != m_hovered_region_id) {
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void NativeUI::OnMouseWheel(int delta) {
    m_scroll_y -= (delta / 120) * 45;
    if (m_scroll_y < 0) m_scroll_y = 0;
    if (m_scroll_y > m_max_scroll_y) m_scroll_y = m_max_scroll_y;
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// -------------------------------------------------------------
// PROCESSAMENTO PRINCIPAL DE MENSAGENS WIN32
// -------------------------------------------------------------

LRESULT NativeUI::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_ERASEBKGND:
            return 1; // Evita flicker durante double buffering

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            OnPaint(hdc, rc.right - rc.left, rc.bottom - rc.top);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_TIMER:
            if (wParam == 1 && g_model.IsScanning()) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;

        case WM_MOUSEWHEEL:
            OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            return 0;

        case WM_MOUSEMOVE: {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }

        case WM_MOUSELEAVE:
            m_hovered_region_id = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_LBUTTONDOWN:
            OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == 1001 && HIWORD(wParam) == EN_CHANGE) {
                GetWindowTextW(m_hSearchEdit, m_search_query, 128);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;

        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, g_theme.GetColors().cr_text);
            SetBkColor(hdcEdit, g_theme.GetColors().cr_card);
            return (LRESULT)g_theme.GetColors().hbr_card;
        }

        case WM_DESTROY:
            Destroy();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
