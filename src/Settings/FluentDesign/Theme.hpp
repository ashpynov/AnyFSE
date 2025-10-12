#pragma once
#include "windows.h"
#include "Tools/Event.hpp"


namespace FluentDesign
{
    class Theme
    {
    public:
        enum Colors
        {
            Text,
            TextSecondary,
            TextDisabled,

            Panel,
            PanelHover,

            Dialog,

            ToggleBorderOn,
            ToggleBorderOff,
            ToggleTrackOn,        // accented color
            ToggleTrackOnPressed, // accented color darker
            ToggleTrackOff,
            ToggleTrackOffHover,
            ToggleThumbOn,
            ToggleThumbOff,
            ToggleBorderOnDisabled,
            ToggleTrackOnDisabled,
            ToggleThumbOnDisabled,
            ToggleBorderOffDisabled,
            ToggleTrackOffDisabled,
            ToggleThumbOffDisabled,

            Combo,
            ComboDisabled,
            ComboHover,
            ComboPressed,
            ComboPopup,
            ComboPopupBorder,
            ComboPopupSelected,
            ComboPopupSelectedMark, // accented color
            ComboPopupHover,

            Edit,
            EditHover,
            EditAccent,
            EditFocus,
            EditAccentFocus, // accented color
            EditBorderFocus,

            Button,
            ButtonHover,
            ButtonPressed,
            ButtonBorder,
            ButtonBorderHover,
            ButtonBorderPressed,
            Max
        };

    private:
        ULONG m_dpi;
        ULONG m_baseDpi;
        ULONG_PTR gdiplusToken;

        bool m_isDark;

        HWND m_hParentWnd;
        HFONT m_hPrimaryFont;
        HFONT m_hSecondaryFont;
        HFONT m_hGlyphFont;
        HFONT m_hTitleFont;
        HFONT m_hGlyphNormalFont;

        COLORREF m_colors[Colors::Max];

        static const int m_primarySize = 14;
        static const int m_secondarySize = 11;
        static const int m_glyphSize = 10;
        static const int m_titleSize = 28;
        static const int m_cornerSize = 8;

        DWORD GetGrey(BYTE lumen);

        void LoadColors();
        void CreateFonts();
        void FreeFonts();

        static  LRESULT CALLBACK DialogSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    public:
        Theme(HWND hParentWnd = NULL);
        void Attach(HWND hParentWnd);
        ~Theme();

        Event OnDPIChanged;

        const bool IsDark() { return m_isDark; }

        const int DpiUnscale(int scaledSize);
        const int DpiScale(int designSize);
        const float DpiScaleF(int designSize);
        const float DpiScaleF(float designSize);

        const HFONT GetFont_Text() { return m_hPrimaryFont; }
        const HFONT GetFont_TextSecondary() { return m_hSecondaryFont; }
        const HFONT GetFont_Glyph() { return m_hGlyphFont; }
        const HFONT GetFont_GlyphNormal() { return m_hGlyphNormalFont; }
        const HFONT GetFont_Title() { return m_hTitleFont; }

        // colors
        const DWORD GetColor(Colors code) { return m_colors[code] | 0xFF000000; }
        const COLORREF GetColorRef(Colors code) { return m_colors[code]; }

        // Constants
        const int GetSize_Text() { return DpiScale(m_primarySize); }
        const int GetSize_TextSecondary() { return DpiScale(m_secondarySize); }
        const int GetSize_Glyph() { return DpiScale(m_glyphSize); }
        const int GetSize_Title() { return DpiScale(m_titleSize); }
        const int GetSize_Corner() { return DpiScale(m_cornerSize); }
    };
}