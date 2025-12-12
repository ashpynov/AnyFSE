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
    class ScrollView
    {
        static LRESULT CALLBACK ScrollViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                                       UINT_PTR uIdSubclass, DWORD_PTR dwRefData);


        Theme &m_theme;

        HWND m_hScrollView;

        int m_contentHeight;
        int m_scrollPos;
        int m_viewHeight = 0;

        RECT m_scrollBarRect;
        RECT m_thumbRect;

        bool m_hovered;
        bool m_dragging;
        int  m_dragStartScrollPos;
        int  m_dragStartMousePos;

        void SetOffset(int newOffset);

        void CalculateRects();
        LRESULT OnPaint(HWND hWnd);

    public:
        HWND GetHwnd() const { return m_hScrollView; }
        ScrollView(Theme& theme);
        ScrollView(Theme& theme, HWND hParent, int x, int y, int width, int height);
        HWND Create(HWND hParent, int x, int y, int width, int height);

        void SetContentHeight(int newHeight);
        void UpdateScrollBar();
        void ScrollTo(int newPos);
        void EnsureVisible(const RECT &rcItem);
        void ScrollBy(int delta);
        int GetScrollPos() const;

        ~ScrollView();

        void OnResize(int newWidth, int newHeight);

        LRESULT OnVScroll(int nScrollCode, int nPos);
        LRESULT OnMouseWheel(int delta);
        LRESULT OnMouseMove();
        LRESULT OnMouseLeave();
        LRESULT OnLButtonDown();
        LRESULT OnLButtonUp();
        LRESULT OnEraseBkgnd(HDC hdc, HWND child);
    };
}