#include "Theme.hpp"
#include "GdiPlus.hpp"
#include "Logging/LogManager.hpp"
#include <dwmapi.h>
#include <commctrl.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Gdiplus.lib")

namespace FluentDesign
{
    DWORD Theme::GetGrey(BYTE lumen)
    {
        return RGB(lumen, lumen, lumen);
    }

    DWORD Theme::GetAccent(BYTE lumen)
    {
        return RGB(123, 20, 139);
    }



    void Theme::LoadColors()
    {
        if (m_isDark)
        {
            m_colors[Text] = GetGrey(255);
            m_colors[TextSecondary] = GetGrey(200);
            m_colors[TextDisabled] = GetGrey(128);

            m_colors[FocusFrame] = GetGrey(200);

            m_colors[Panel] = GetGrey(43);
            m_colors[PanelHover] = GetGrey(50);
            m_colors[PanelBorder] = GetGrey(43);

            m_colors[Dialog] = GetGrey(32);

            m_colors[ToggleBorderOn] = GetGrey(89);
            m_colors[ToggleBorderOff] = GetGrey(207);
            m_colors[ToggleTrackOn] = GetAccent(89);           // accented color
            m_colors[ToggleTrackOnPressed] = GetAccent(84);    // accented color darker
            m_colors[ToggleTrackOnDisabled] = GetAccent(89);           // accented color
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
            m_colors[ComboPopupSelectedMark] = GetAccent(89);  // accented color
            m_colors[ComboPopupHover] = GetGrey(61);

            m_colors[Edit] = GetGrey(55);
            m_colors[EditHover] = GetGrey(61);
            m_colors[EditAccent] = GetGrey(154);
            m_colors[EditFocus] = GetGrey(31);
            m_colors[EditAccentFocus] = GetAccent(64);         // accented color
            m_colors[EditBorder] = GetGrey(55);
            m_colors[EditBorderFocus] = GetGrey(58);

            m_colors[Button] = GetGrey(55);
            m_colors[ButtonHover] = GetGrey(60);
            m_colors[ButtonPressed] = GetGrey(50);
            m_colors[ButtonBorder] = GetGrey(63);
            m_colors[ButtonBorderHover] = GetGrey(63);
            m_colors[ButtonBorderPressed] = GetGrey(58);
        }
        else
        {
            m_colors[Text] = GetGrey(27);
            m_colors[TextSecondary] = GetGrey(96);
            m_colors[TextDisabled] = GetGrey(160);

            m_colors[FocusFrame] = GetGrey(27);

            m_colors[Panel] = GetGrey(251);
            m_colors[PanelHover] = GetGrey(246);
            m_colors[PanelBorder] = GetGrey(229);

            m_colors[Dialog] = GetGrey(243);

            m_colors[ToggleBorderOn] = GetAccent(89);             // accented color
            m_colors[ToggleBorderOff] = GetGrey(135);
            m_colors[ToggleTrackOn] = GetAccent(89);              // accented color
            m_colors[ToggleTrackOnPressed] = GetAccent(84);       // accented color darker
            m_colors[ToggleTrackOnDisabled] = GetAccent(89);      // accented color
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
            m_colors[ComboPopupSelectedMark] = GetAccent(89);  // accented color
            m_colors[ComboPopupHover] = GetGrey(240);

            m_colors[Edit] = GetGrey(251);
            m_colors[EditHover] = GetGrey(246);
            m_colors[EditAccent] = GetGrey(154);
            m_colors[EditFocus] = GetGrey(255);
            m_colors[EditAccentFocus] = GetAccent(64);         // accented color
            m_colors[EditBorder] = GetGrey(227);
            m_colors[EditBorderFocus] = GetGrey(227);

            m_colors[Button] = GetGrey(254);
            m_colors[ButtonHover] = GetGrey(250);
            m_colors[ButtonPressed] = GetGrey(249);
            m_colors[ButtonBorder] = GetGrey(211);
            m_colors[ButtonBorderHover] = GetGrey(211);
            m_colors[ButtonBorderPressed] = GetGrey(236);
        }
    }
}