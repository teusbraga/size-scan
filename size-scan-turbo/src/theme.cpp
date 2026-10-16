#include "theme.h"

using namespace Gdiplus;

ThemeManager g_theme;

ThemeManager::ThemeManager()
    : m_hwnd(NULL)
    , m_mode(THEME_DARK)
{
    memset(&m_colors, 0, sizeof(m_colors));
    InitPalette();
}

ThemeManager::~ThemeManager() {
    FreeBrushes();
}

void ThemeManager::FreeBrushes() {
    if (m_colors.hbr_bg)      { DeleteObject(m_colors.hbr_bg);      m_colors.hbr_bg = NULL; }
    if (m_colors.hbr_surface) { DeleteObject(m_colors.hbr_surface); m_colors.hbr_surface = NULL; }
    if (m_colors.hbr_card)    { DeleteObject(m_colors.hbr_card);    m_colors.hbr_card = NULL; }
    if (m_colors.hbr_border)  { DeleteObject(m_colors.hbr_border);  m_colors.hbr_border = NULL; }
}

void ThemeManager::InitPalette() {
    FreeBrushes();

    if (m_mode == THEME_DARK) {
        // Paleta Dark Mode (Grafite Profundo & Suave)
        m_colors.col_bg           = Color(255, 30, 32, 34);     // #1e2022
        m_colors.col_surface      = Color(255, 46, 49, 51);     // #2e3133
        m_colors.col_card         = Color(255, 38, 40, 42);     // #26282a
        m_colors.col_card_hover   = Color(255, 50, 53, 56);     // #323538
        m_colors.col_border       = Color(255, 61, 64, 67);     // #3d4043
        m_colors.col_border_light = Color(255, 48, 51, 54);     // #303336
        m_colors.col_text         = Color(255, 240, 242, 245);  // #f0f2f5
        m_colors.col_text_muted   = Color(255, 157, 163, 168);  // #9da3a8
        m_colors.col_accent_mint  = Color(255, 119, 201, 181);  // #77c9b5
        m_colors.col_accent_blue  = Color(255, 107, 163, 214);  // #6ba3d6
        m_colors.col_accent_amber = Color(255, 226, 169, 100);  // #e2a964
        m_colors.col_accent_red   = Color(255, 217, 107, 107);  // #d96b6b

        // Categorias para Dark Mode
        m_colors.cat_colors[0] = Color(255, 217, 107, 107); // Vídeos - Coral
        m_colors.cat_colors[1] = Color(255, 119, 201, 181); // Imagens - Menta
        m_colors.cat_colors[2] = Color(255, 163, 136, 204); // Áudios - Lavanda
        m_colors.cat_colors[3] = Color(255, 226, 169, 100); // Compactados - Âmbar
        m_colors.cat_colors[4] = Color(255, 107, 163, 214); // Documentos - Azul
        m_colors.cat_colors[5] = Color(255, 219, 126, 98);  // Executáveis - Terracota
        m_colors.cat_colors[6] = Color(255, 91, 191, 174);  // Código - Ciano
        m_colors.cat_colors[7] = Color(255, 138, 145, 153); // Outros - Cinza

        m_colors.cr_bg          = RGB(30, 32, 34);
        m_colors.cr_surface     = RGB(46, 49, 51);
        m_colors.cr_card        = RGB(38, 40, 42);
        m_colors.cr_border      = RGB(61, 64, 67);
        m_colors.cr_text        = RGB(240, 242, 245);
        m_colors.cr_text_muted  = RGB(157, 163, 168);
        m_colors.cr_accent_mint = RGB(119, 201, 181);
    } else {
        // Paleta Light Mode (Visual Moderno Windows 11 Fluent)
        m_colors.col_bg           = Color(255, 243, 244, 246);  // #F3F4F6
        m_colors.col_surface      = Color(255, 255, 255, 255);  // #FFFFFF
        m_colors.col_card         = Color(255, 255, 255, 255);  // #FFFFFF
        m_colors.col_card_hover   = Color(255, 241, 245, 249);  // #F1F5F9
        m_colors.col_border       = Color(255, 226, 232, 240);  // #E2E8F0
        m_colors.col_border_light = Color(255, 241, 245, 249);  // #F1F5F9
        m_colors.col_text         = Color(255, 15, 23, 42);     // #0F172A (Texto profundo de alto contraste)
        m_colors.col_text_muted   = Color(255, 100, 116, 139);  // #64748B
        m_colors.col_accent_mint  = Color(255, 13, 148, 136);   // #0D9488 (Teal nítido)
        m_colors.col_accent_blue  = Color(255, 37, 99, 235);    // #2563EB
        m_colors.col_accent_amber = Color(255, 217, 119, 6);    // #D97706
        m_colors.col_accent_red   = Color(255, 225, 29, 72);    // #E11D48

        // Categorias para Light Mode (Cores vívidas de excelente leitura no branco)
        m_colors.cat_colors[0] = Color(255, 225, 29, 72);   // Vídeos - Vermelho/Rosa
        m_colors.cat_colors[1] = Color(255, 13, 148, 136);  // Imagens - Teal
        m_colors.cat_colors[2] = Color(255, 124, 58, 237);  // Áudios - Roxo
        m_colors.cat_colors[3] = Color(255, 217, 119, 6);   // Compactados - Âmbar
        m_colors.cat_colors[4] = Color(255, 37, 99, 235);   // Documentos - Azul
        m_colors.cat_colors[5] = Color(255, 234, 88, 12);   // Executáveis - Laranja
        m_colors.cat_colors[6] = Color(255, 5, 150, 105);   // Código - Verde Esmeralda
        m_colors.cat_colors[7] = Color(255, 100, 116, 139); // Outros - Cinza Ardósia

        m_colors.cr_bg          = RGB(243, 244, 246);
        m_colors.cr_surface     = RGB(255, 255, 255);
        m_colors.cr_card        = RGB(255, 255, 255);
        m_colors.cr_border      = RGB(226, 232, 240);
        m_colors.cr_text        = RGB(15, 23, 42);
        m_colors.cr_text_muted  = RGB(100, 116, 139);
        m_colors.cr_accent_mint = RGB(13, 148, 136);
    }

    m_colors.hbr_bg      = CreateSolidBrush(m_colors.cr_bg);
    m_colors.hbr_surface = CreateSolidBrush(m_colors.cr_surface);
    m_colors.hbr_card    = CreateSolidBrush(m_colors.cr_card);
    m_colors.hbr_border  = CreateSolidBrush(m_colors.cr_border);
}

void ThemeManager::SetThemeMode(AppThemeMode mode) {
    if (m_mode == mode) return;
    m_mode = mode;
    InitPalette();
    if (m_hwnd) {
        ApplyWindowTheme(m_hwnd);
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

void ThemeManager::ToggleTheme() {
    SetThemeMode(m_mode == THEME_DARK ? THEME_LIGHT : THEME_DARK);
}

void ThemeManager::Init(HWND hwnd) {
    m_hwnd = hwnd;
    ApplyWindowTheme(hwnd);
}

void ThemeManager::ApplyWindowTheme(HWND hwnd) {
    BOOL is_dark = (m_mode == THEME_DARK);
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &is_dark, sizeof(is_dark));
    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corner, sizeof(corner));
}

void ThemeManager::DrawRoundedRect(Graphics &g, Pen &pen, Brush &brush, const Rect &r, int radius) {
    int dia = radius * 2;
    if (dia > r.Width) dia = r.Width;
    if (dia > r.Height) dia = r.Height;
    if (dia <= 0) {
        g.FillRectangle(&brush, r);
        g.DrawRectangle(&pen, r);
        return;
    }

    GraphicsPath path;
    path.AddArc(r.X, r.Y, dia, dia, 180, 90);
    path.AddArc(r.X + r.Width - dia, r.Y, dia, dia, 270, 90);
    path.AddArc(r.X + r.Width - dia, r.Y + r.Height - dia, dia, dia, 0, 90);
    path.AddArc(r.X, r.Y + r.Height - dia, dia, dia, 90, 90);
    path.CloseFigure();

    g.FillPath(&brush, &path);
    g.DrawPath(&pen, &path);
}
