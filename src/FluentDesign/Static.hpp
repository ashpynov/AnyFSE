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
#include "Tools/GdiPlus.hpp"
#include "Tools/Event.hpp"
#include "Theme.hpp"

namespace FluentDesign
{
    class Static
    {

    private:
        int m_designWidth;
        int m_designHeight;

        bool m_large;

        HWND m_hStatic;

        std::wstring m_text;
        Theme::Colors m_colorId;
        Gdiplus::StringFormat m_format;
        Gdiplus::Image * m_pImage;

        static LRESULT CALLBACK StaticSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void DrawStatic(HWND hWnd, HDC hdc, RECT rc);

        void UpdateLayout();

        Theme &m_theme;

    public:
        Static(Theme& theme);
        Static(
            Theme& theme,
            HWND hParent,
            int x, int y,
            int width, int height);

        HWND Create(HWND hParent, int x, int y, int width, int height);
        HWND GetHwnd() const { return m_hStatic; }

        void SetText(const std::wstring& text);

        void SetColor(Theme::Colors colorId);
        void SetLarge(bool large);
        void Invalidate();
        Gdiplus::StringFormat &Format() { return m_format; };
        void LoadIcon(const std::wstring &iconPath, int iconSize);

        ~Static();
    };
}