// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


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

        HWND GetHwnd() const { return m_hToggle; }
        void SetCheck(bool isChecked);
        bool GetCheck();
        Event OnChanged;
    };
}