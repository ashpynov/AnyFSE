#pragma once
#include "windows.h"

namespace FluentDesign
{
    class Theme
    {
    public:
        enum Colors
        {
            Text = 255,
            TextSecondary = 200,
            TextDisabled = 128,

            Panel = 43,
            PanelHover = 43,

            Dialog = 32,

            ToggleBorderOn = 89,
            ToggleBorderOff = 207,
            ToggleTrackOn = 89,             // accented color
            ToggleTrackOnPressed = 84,      // accented color darker
            ToggleTrackOff = 39,
            ToggleTrackOffHover = 52,
            ToggleThumbOn = 0,
            ToggleThumbOff = 207,

            Combo = 55,
            ComboDisabled = 50,
            ComboHover = 61,
            ComboPressed = 50,
            ComboPopup = 38,
            ComboPopupSelected = 61,
            ComboPopupSelectedMark = 89,  // accented color
            ComboPopupHover = 61,

            Edit = 55,
            EditHover = 61,
            EditAccent = 154,
            EditFocus = 31,
            EditAccentFocus = 64,   // accented color
            EditBorderFocus = 58,
        };

    private:
        ULONG m_dpi;
        ULONG_PTR gdiplusToken;

        HWND m_hParentWnd;
        HFONT m_hPrimaryFont;
        HFONT m_hSecondaryFont;
        HFONT m_hGlyphFont;

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

        const HFONT GetFont_Text() { return m_hPrimaryFont; }
        const HFONT GetFont_TextSecondary() { return m_hSecondaryFont; }
        const HFONT GetFont_Glyph() { return m_hGlyphFont; }

        const DWORD GetColor(Colors code) { return RGB((BYTE)code, (BYTE)code, (BYTE)code) | 0xFF000000; }
        const COLORREF GetColorRef(Colors code) { return RGB((BYTE)code, (BYTE)code, (BYTE)code); }

        // Constants
        const int GetSize_Text() { return DpiScale(m_primarySize); }
        const int GetSize_TextSecondary() { return DpiScale(m_secondarySize); }
        const int GetSize_Glyph() { return DpiScale(m_glyphSize); }
    };
}