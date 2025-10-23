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
    class ScrollView
    {
        static LRESULT CALLBACK ScrollViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        Theme &m_theme;

        HWND m_hScrollView;

        int m_contentHeight;
        int m_currectOffset;

        void SetOffset(int newOffset);

    public:
        ScrollView(Theme& theme);
        ScrollView(Theme& theme, HWND hParent, int x, int y, int width, int height);
        HWND Create(HWND hParent, int x, int y, int width, int height);

        void SetContentHeight(int newHeight);

        ~ScrollView();
    };
}