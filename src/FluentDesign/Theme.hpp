// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#pragma once

#include <map>
#include <windows.h>
#include "Tools/Event.hpp"
#include "Tools/GdiPlus.hpp"

namespace FluentDesign
{
    class Theme
    {
    public:
        enum Colors
        {
            Text = 0 ,
            TextSecondary,
            TextDisabled,
            TextAccented,

            FocusFrame,

            Panel,
            PanelHover,
            PanelBorder,

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
            ComboBorder,
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
            EditBorder,
            EditBorderFocus,

            Button,
            ButtonHover,
            ButtonPressed,
            ButtonBorder,
            ButtonBorderHover,
            ButtonBorderPressed,

            ScrollThumb,
            ScrollThumbStroke,
            ScrollTrack,

            Breadcrumb,
            BreadcrumbLink,
            BreadcrumbLinkHover,
            BreadcrumbLinkPressed,

            Max
        };

    private:
        ULONG m_dpi;
        ULONG m_baseDpi;
        ULONG_PTR m_gdiplusToken;

        bool m_isDark;
        COLORREF m_accentColor;

        HWND m_hParentWnd;
        HFONT m_hPrimaryFont;
        HFONT m_hPrimaryBoldFont;
        HFONT m_hLargeFont;
        HFONT m_hSecondaryFont;
        HFONT m_hGlyphFont;
        HFONT m_hIconFont;
        HFONT m_hTitleFont;
        HFONT m_hGlyphNormalFont;

        COLORREF m_colors[Colors::Max + 1];

        static const int m_primarySize = 14;
        static const int m_largeSize = 20;
        static const int m_secondarySize = 11;
        static const int m_glyphSize = 10;
        static const int m_iconSize = 20;
        static const int m_titleSize = 28;
        static const int m_cornerSize = 16;
        static const int m_focusWidth = 2;
        static const int m_focusMargins = 1;
        static const int m_focusToggleMargins = 8;
        static const int m_focusCornerSize = 10;

        bool m_isKeyboardFocus;
        HWND m_lastFocused;
        std::list<HWND> m_childsList;
        DWORD GetGrey(BYTE lumen);

        DWORD GetAccent(BYTE lumen);

        void LoadColors();
        void CreateFonts();
        void FreeFonts();

        static LRESULT CALLBACK DialogSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        static LRESULT ControlParentSublassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        static LRESULT ControlSublassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void SendNotifyFocus(HWND hWnd);

    public:
        Theme(HWND hParentWnd = NULL);
        void Attach(HWND hParentWnd);

        void RegisterChild(HWND hChild);

        bool IsKeyboardFocused()
        {
            return m_isKeyboardFocus;
        }
        bool SetKeyboardFocused(HWND hwnd);

        ~Theme();

        float FocusOffsetByWndClass(HWND hwnd);

        void DrawFocusFrame(HDC hdc, const RECT &clientRect, float offset);

        void DrawChildFocus(HDC hdc, HWND parent, HWND child);

        Event OnDPIChanged;

        const bool IsDark() { return m_isDark; }

        const int DpiUnscale(int scaledSize);
        const float DpiUnscaleF(int scaledSize);
        const float DpiUnscaleF(float scaledSize);

        const int DpiScale(int designSize);
        const float DpiScaleF(int designSize);
        const float DpiScaleF(float designSize);

        const Gdiplus::RectF DpiUnscaleF(const RECT &actualRect);
        const RECT DpiScale(const Gdiplus::RectF &designRectF);

        const void DpiUnscaleChilds(HWND parent, std::map<HWND,Gdiplus::RectF>& storage);
        const void DpiScaleChilds(HWND parent, const std::map<HWND,Gdiplus::RectF>& storage);

        bool IsDarkThemeEnabled();
        COLORREF GetAccentColor();

        const HFONT GetFont_Text() { return m_hPrimaryFont; }
        const HFONT GetFont_TextBold() { return m_hPrimaryBoldFont; }
        const HFONT GetFont_TextSecondary() { return m_hSecondaryFont; }
        const HFONT GetFont_TextLarge() { return m_hLargeFont; }
        const HFONT GetFont_Glyph() { return m_hGlyphFont; }
        const HFONT GetFont_GlyphNormal() { return m_hGlyphNormalFont; }
        const HFONT GetFont_Title() { return m_hTitleFont; }
        const HFONT GetFont_Icon() { return m_hIconFont; }

        // colors
        const DWORD ReverseRGB(DWORD rgb) { return RGB(rgb >> 16, rgb >> 8, rgb); }
        const DWORD GetColor(Colors code) { return ReverseRGB(m_colors[code]) | 0xFF000000; }
        const COLORREF GetColorRef(Colors code) { return m_colors[code]; }

        // Constants
        const int GetSize_Text() { return DpiScale(m_primarySize); }
        const int GetSize_TextLarge() { return DpiScale(m_largeSize); }
        const int GetSize_TextSecondary() { return DpiScale(m_secondarySize); }
        const int GetSize_Glyph() { return DpiScale(m_glyphSize); }
        const int GetSize_Icon() { return DpiScale(m_iconSize); }
        const int GetSize_Title() { return DpiScale(m_titleSize); }
        const int GetSize_Corner() { return DpiScale(m_cornerSize); }

        const int GetSize_FocusWidth() { return DpiScale(m_focusWidth); }
        const int GetSize_FocusCornerSize() { return DpiScale(m_focusCornerSize); }
        const int GetSize_FocusMargins() { return DpiScale(m_focusMargins); }
    };

}