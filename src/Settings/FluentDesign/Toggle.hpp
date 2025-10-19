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

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <functional>
#include "Tools/Event.hpp"
#include "Theme.hpp"

namespace FluentDesign
{
    class Toggle
    {

    private:
        static const int Layout_ItemHeight = 20;
        static const int Layout_ItemWidth = 40;
        static const int Layout_TextWidth = 32;


        HWND m_hToggle;
        bool m_buttonPressed;
        bool m_buttonMouseOver;
        int  m_pressedPos;
        int  m_thumbShift;
        int  m_pressedPath;
        bool m_isChecked;

        static LRESULT CALLBACK ToggleSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void HandleMouse(HWND hWnd, UINT uMsg, LPARAM lParam);
        void DrawButton(HWND hWnd, HDC hdc, RECT rc);

        void UpdateLayout();

        Theme &m_theme;

    public:
        Toggle(Theme& theme);
        Toggle(
            Theme& theme,
            HWND hParent,
            int x, int y,
            int width, int height);

        HWND Create(HWND hParent, int x, int y, int width, int height);

        ~Toggle();

        void SetCheck(bool isChecked);
        bool GetCheck();
        Event OnChanged;
    };
}