#pragma once
#include "windows.h"

namespace FluentDesign
{
    class Theme
    {
        ULONG m_dpi;
        ULONG_PTR gdiplusToken;

        HWND m_hParentWnd;
        HFONT m_hPrimaryFont;
        HFONT m_hSecondaryFont;
        HFONT m_hGlyphFont;

        // Foreground
        COLORREF m_primaryColor;
        COLORREF m_secondaryColor;
        COLORREF m_disabledColor;

        // Background
        COLORREF m_baseColorBg;
        COLORREF m_baseSecondaryColorBg;
        COLORREF m_controlColorBg;
        COLORREF m_pressedColorBg;
        COLORREF m_hoveredColorBg;
        COLORREF m_popupColorBg;

        static const int m_primarySize = 14;
        static const int m_secondarySize = 11;
        static const int m_glyphSize = 10;

        void LoadColors(bool isDark);
        void CreateFonts();
        void FreeFonts();

    public:
        Theme(HWND hParentWnd = NULL);
        void Attach(HWND hParentWnd);
        ~Theme();

        const int DpiScale(int designSize);
        const float DpiScaleF(int designSize);
        const float DpiScaleF(float designSize);
        const HFONT PrimaryFont() { return m_hPrimaryFont; }
        const HFONT SecondaryFont() { return m_hSecondaryFont; }
        const HFONT GlyphFont() { return m_hGlyphFont; }

        // Foreground
        const COLORREF PrimaryColor() { return m_primaryColor; }
        const COLORREF SecondaryColor() { return m_secondaryColor; }
        const COLORREF DisabledColor() { return m_disabledColor; }

        const COLORREF ThemeColor(BYTE lumen) { return RGB(lumen, lumen, lumen); }

        // Background
        const COLORREF BaseColorBg() { return m_baseColorBg; }
        const COLORREF BaseSecondaryColorBg() { return m_baseSecondaryColorBg; }
        const COLORREF ControlColorBg() { return m_controlColorBg; }
        const COLORREF PressedColorBg() { return m_pressedColorBg; }
        const COLORREF HoveredColorBg() { return m_hoveredColorBg; }
        const COLORREF PopupColorBg() { return m_popupColorBg; }

        // Constants
        const int PrimarySize() { return DpiScale(m_primarySize); }
        const int SecondarySize() { return DpiScale(m_secondarySize); }
        const int GlyphSize() { return DpiScale(m_glyphSize); }
    };
}