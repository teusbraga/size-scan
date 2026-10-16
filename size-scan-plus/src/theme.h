#ifndef THEME_H
#define THEME_H

#include <windows.h>
#include <gdiplus.h>
#include <dwmapi.h>

enum AppThemeMode {
    THEME_DARK = 0,
    THEME_LIGHT = 1
};

struct ThemeColors {
    // Cores GDI+
    Gdiplus::Color col_bg;           // Fundo da janela
    Gdiplus::Color col_surface;      // Superfície base (header, tabs)
    Gdiplus::Color col_card;         // Cartões
    Gdiplus::Color col_card_hover;   // Hover suave
    Gdiplus::Color col_border;       // Bordas
    Gdiplus::Color col_border_light; // Divisores suaves
    Gdiplus::Color col_text;         // Texto principal
    Gdiplus::Color col_text_muted;   // Texto secundário suave
    Gdiplus::Color col_accent_mint;  // Destaque menta/teal
    Gdiplus::Color col_accent_blue;  // Destaque azul
    Gdiplus::Color col_accent_amber; // Destaque âmbar/amarelo
    Gdiplus::Color col_accent_red;   // Destaque vermelho/coral

    // Paleta de categorias equilibrada para o tema ativo
    Gdiplus::Color cat_colors[8];

    // Equivalentes em COLORREF para Win32 / GDI
    COLORREF cr_bg;
    COLORREF cr_surface;
    COLORREF cr_card;
    COLORREF cr_border;
    COLORREF cr_text;
    COLORREF cr_text_muted;
    COLORREF cr_accent_mint;

    HBRUSH hbr_bg;
    HBRUSH hbr_surface;
    HBRUSH hbr_card;
    HBRUSH hbr_border;
};

class ThemeManager {
public:
    ThemeManager();
    ~ThemeManager();

    void Init(HWND hwnd);
    const ThemeColors& GetColors() const { return m_colors; }
    void ApplyWindowTheme(HWND hwnd);

    AppThemeMode GetThemeMode() const { return m_mode; }
    void SetThemeMode(AppThemeMode mode);
    void ToggleTheme();
    bool IsDarkMode() const { return m_mode == THEME_DARK; }

    static void DrawRoundedRect(Gdiplus::Graphics &g, Gdiplus::Pen &pen, Gdiplus::Brush &brush, const Gdiplus::Rect &r, int radius);

private:
    void InitPalette();
    void FreeBrushes();

private:
    HWND m_hwnd;
    AppThemeMode m_mode;
    ThemeColors m_colors;
};

extern ThemeManager g_theme;

#endif // THEME_H
