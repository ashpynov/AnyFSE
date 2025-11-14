// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include "Theme.hpp"
#include "Tools/GdiPlus.hpp"
#include "Logging/LogManager.hpp"
#include <dwmapi.h>
#include <commctrl.h>
#include <algorithm>
#include "Tools/Registry.hpp"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Gdiplus.lib")

namespace FluentDesign
{
    bool Theme::IsDarkThemeEnabled()
    {
        std::wstring root = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
        DWORD appLight = Registry::ReadDWORD(root, L"AppsUseLightTheme", Registry::ReadDWORD(root, L"SystemUsesLightTheme", 1));
        return (appLight == 0);
    }

    COLORREF Theme::GetAccentColor()
    {
        BOOL dwmEnabled;
        DWORD dwmColor;

        if (SUCCEEDED(DwmIsCompositionEnabled(&dwmEnabled)) && dwmEnabled)
        {
            if (SUCCEEDED(DwmGetColorizationColor(&dwmColor, &dwmEnabled)))
            {
                return ReverseRGB(dwmColor) & 0xFFFFFF;
            }
        }
        return RGB(215, 170, 170);
    }

    DWORD Theme::GetGrey(BYTE lumen)
    {
        return RGB(lumen, lumen, lumen);
    }

    DWORD Theme::GetAccent(BYTE lumen)
    {
        // cast m_accent to HSV
        double R = ((double)GetRValue(m_accentColor)) / 255;
        double G = ((double)GetGValue(m_accentColor)) / 255;
        double B = ((double)GetBValue(m_accentColor)) / 255;

        double Cmax = max(R, max(G, B));
        double Cmin = min(R, min(G, B));
        double delta = (Cmax - Cmin);

        double V = Cmax;
        double S = Cmax == 0 ? 0 : delta / Cmax;

        double H = 0;
        if (delta != 0)
        {
            H =   Cmax == R ? (G - B) / delta + (G < B ? 6.0 : 0.0)
                : Cmax == G ? (B - R) / delta + 2.0
                            : (R - G) / delta + 4.0;
        }

        // change V
        double Vmix = ((double)lumen / 255.0);

        V = (Vmix <= 0.5)
            ? 2.0 * Vmix * V
            : 2.0 * (Vmix - 0.5) * (1.0 - V) + V;

        S = S * 0.75;
        // cast HSV to RGB:
        if (S == 0)
        {
            R = G = B = V;
        }
        else
        {
            int i = (int)H;
            double f = H - i;
            double p = V * (1 - S);
            double q = V * (1 - S * f);
            double t = V * (1 - S * (1 - f));

            switch (i)
            {
                case 0: R = V; G = t; B = p; break;
                case 1: R = q; G = V; B = p; break;
                case 2: R = p; G = V; B = t; break;
                case 3: R = p; G = q; B = V; break;
                case 4: R = t; G = p; B = V; break;
                default: R = V; G = p; B = q; break;
            }
        }
        return RGB(R * 255, G * 255, B * 255);
    }

    void Theme::LoadColors()
    {
        m_isDark = IsDarkThemeEnabled();
        m_accentColor = GetAccentColor();

        BOOL useDarkMode = IsDark() ? TRUE : FALSE;
        DwmSetWindowAttribute(m_hParentWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

        COLORREF captionColor = GetColorRef(FluentDesign::Theme::Colors::Dialog);
        DwmSetWindowAttribute(m_hParentWnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));

        if (m_isDark)
        {
            m_colors[Text] = GetGrey(255);
            m_colors[TextSecondary] = GetGrey(200);
            m_colors[TextDisabled] = GetGrey(128);
            m_colors[TextAccented] = GetAccent(204);

            m_colors[FocusFrame] = GetGrey(200);

            m_colors[Panel] = GetGrey(43);
            m_colors[PanelHover] = GetGrey(50);
            m_colors[PanelBorder] = GetGrey(32);

            m_colors[Dialog] = GetGrey(32);
            m_colors[Footer] = GetGrey(38);

            m_colors[ToggleBorderOn] = GetAccent(204);          // accented color
            m_colors[ToggleBorderOff] = GetGrey(207);
            m_colors[ToggleTrackOn] = GetAccent(204);           // accented color
            m_colors[ToggleTrackOnPressed] = GetAccent(196);    // accented color darker
            m_colors[ToggleTrackOnDisabled] = GetAccent(204);   // accented color
            m_colors[ToggleTrackOff] = GetGrey(39);
            m_colors[ToggleTrackOffHover] = GetGrey(52);
            m_colors[ToggleThumbOn] = GetGrey(0);
            m_colors[ToggleThumbOff] = GetGrey(207);

            m_colors[ToggleBorderOnDisabled] = GetGrey(60);
            m_colors[ToggleTrackOnDisabled] = GetGrey(60);
            m_colors[ToggleThumbOnDisabled] = GetGrey(0);
            m_colors[ToggleBorderOffDisabled] = GetGrey(60);
            m_colors[ToggleTrackOffDisabled] = GetGrey(39);
            m_colors[ToggleThumbOffDisabled] = GetGrey(60);


            m_colors[Combo] = GetGrey(55);
            m_colors[ComboDisabled] = GetGrey(50);
            m_colors[ComboHover] = GetGrey(61);
            m_colors[ComboPressed] = GetGrey(50);
            m_colors[ComboBorder] = GetGrey(211);
            m_colors[ComboPopup] = GetGrey(38);
            m_colors[ComboPopupBorder] = GetGrey(1);
            m_colors[ComboPopupSelected] = GetGrey(61);
            m_colors[ComboPopupSelectedMark] = GetAccent(204);  // accented color
            m_colors[ComboPopupHover] = GetGrey(61);

            m_colors[Edit] = GetGrey(55);
            m_colors[EditHover] = GetGrey(61);
            m_colors[EditAccent] = GetGrey(154);
            m_colors[EditFocus] = GetGrey(31);
            m_colors[EditAccentFocus] = GetAccent(204);         // accented color
            m_colors[EditBorder] = GetGrey(55);
            m_colors[EditBorderFocus] = GetGrey(58);

            m_colors[Button] = GetGrey(55);
            m_colors[ButtonHover] = GetGrey(60);
            m_colors[ButtonPressed] = GetGrey(50);
            m_colors[ButtonBorder] = GetGrey(63);
            m_colors[ButtonBorderHover] = GetGrey(63);
            m_colors[ButtonBorderPressed] = GetGrey(58);
            m_colors[ScrollThumb] = GetGrey(157);
            m_colors[ScrollThumbStroke] = GetGrey(100);
            m_colors[ScrollTrack] = GetGrey(51);

            m_colors[Breadcrumb] = GetGrey(255);
            m_colors[BreadcrumbLink] = GetGrey(204);
            m_colors[BreadcrumbLinkHover] = GetGrey(255);
            m_colors[BreadcrumbLinkPressed] = GetGrey(150);
        }
        else
        {
            m_colors[Text] = GetGrey(27);
            m_colors[TextSecondary] = GetGrey(96);
            m_colors[TextDisabled] = GetGrey(160);
            m_colors[TextAccented] = GetAccent(128);

            m_colors[FocusFrame] = GetGrey(27);

            m_colors[Panel] = GetGrey(251);
            m_colors[PanelHover] = GetGrey(246);
            m_colors[PanelBorder] = GetGrey(229);

            m_colors[Dialog] = GetGrey(243);
            m_colors[Footer] = GetGrey(230);

            m_colors[ToggleBorderOn] = GetAccent(128);             // accented color
            m_colors[ToggleBorderOff] = GetGrey(135);
            m_colors[ToggleTrackOn] = GetAccent(128);              // accented color
            m_colors[ToggleTrackOnPressed] = GetAccent(135);       // accented color darker
            m_colors[ToggleTrackOnDisabled] = GetAccent(128);      // accented color
            m_colors[ToggleTrackOff] = GetGrey(245);
            m_colors[ToggleTrackOffHover] = GetGrey(236);
            m_colors[ToggleThumbOn] = GetGrey(255);
            m_colors[ToggleThumbOff] = GetGrey(93);

            m_colors[ToggleBorderOnDisabled] = GetGrey(160);
            m_colors[ToggleTrackOnDisabled] = GetGrey(160);
            m_colors[ToggleThumbOnDisabled] = GetGrey(255);
            m_colors[ToggleBorderOffDisabled] = GetGrey(197);
            m_colors[ToggleTrackOffDisabled] = GetGrey(251);
            m_colors[ToggleThumbOffDisabled] = GetGrey(160);


            m_colors[Combo] = GetGrey(254);
            m_colors[ComboDisabled] = GetGrey(250);
            m_colors[ComboHover] = GetGrey(248);
            m_colors[ComboPressed] = GetGrey(246);
            m_colors[ComboBorder] = GetGrey(211);
            m_colors[ComboPopup] = GetGrey(249);
            m_colors[ComboPopupBorder] = GetGrey(222);
            m_colors[ComboPopupSelected] = GetGrey(239);
            m_colors[ComboPopupSelectedMark] = GetAccent(128);  // accented color
            m_colors[ComboPopupHover] = GetGrey(240);

            m_colors[Edit] = GetGrey(251);
            m_colors[EditHover] = GetGrey(246);
            m_colors[EditAccent] = GetGrey(154);
            m_colors[EditFocus] = GetGrey(255);
            m_colors[EditAccentFocus] = GetAccent(128);         // accented color
            m_colors[EditBorder] = GetGrey(227);
            m_colors[EditBorderFocus] = GetGrey(227);

            m_colors[Button] = GetGrey(254);
            m_colors[ButtonHover] = GetGrey(250);
            m_colors[ButtonPressed] = GetGrey(249);
            m_colors[ButtonBorder] = GetGrey(211);
            m_colors[ButtonBorderHover] = GetGrey(211);
            m_colors[ButtonBorderPressed] = GetGrey(236);
            m_colors[ScrollThumb] = GetGrey(135);
            m_colors[ScrollThumbStroke] = GetGrey(200);
            m_colors[ScrollTrack] = GetGrey(251);

            m_colors[Breadcrumb] = GetGrey(26);
            m_colors[BreadcrumbLink] = GetGrey(92);
            m_colors[BreadcrumbLinkHover] = GetGrey(26);
            m_colors[BreadcrumbLinkPressed] = GetGrey(134);
        }
    }
}