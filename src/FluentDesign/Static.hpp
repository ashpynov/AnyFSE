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
#include "Tools/GdiPlus.hpp"
#include "Tools/Event.hpp"
#include "Theme.hpp"

namespace FluentDesign
{
    class Static
    {

    private:
        int m_designWidth = 0;
        int m_designHeight = 0;

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

        HWND Create(HWND hParent, const std::wstring &text, int x, int y, int width, int height);

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