
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

#include "ScrollView.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"

namespace FluentDesign
{
    LRESULT ScrollView::ScrollViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        ScrollView *This = reinterpret_cast<ScrollView *>(dwRefData);
        switch (uMsg)
        {
            case WM_SETFOCUS:
            {
                SetFocus(GetWindow(hWnd, GW_CHILD));
            }
            break;

            case WM_SIZE:
            {
                HWND hChild = GetWindow(hWnd, GW_CHILD);
                while (hChild)
                {
                    RECT rc;
                    GetClientRect(hChild, &rc);
                    SendMessage(hChild, WM_SIZE, 0, MAKELONG(rc.right - rc.left, rc.bottom - rc.top));
                    hChild = GetWindow(hChild, GW_HWNDNEXT);
                }
                This->OnResize(LOWORD(lParam), HIWORD(lParam));
            }
            break;

            case WM_VSCROLL:
                return This->OnVScroll(LOWORD(wParam), HIWORD(wParam));

            case WM_MOUSEWHEEL:
                return This->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));


            case WM_NCPAINT:
                return This->OnNcPaint(hWnd);

            case WM_PAINT:
                {
                    FluentDesign::DoubleBuferedPaint paint(hWnd);
                }
                return 0;

            case WM_ERASEBKGND:
                {
                    if(lParam)
                    {
                        HWND parent = GetParent(hWnd);
                        if (parent)
                        {
                            SendMessage(parent, WM_ERASEBKGND, (WPARAM)wParam, (LPARAM)hWnd);
                        }
                    }
                }
                return 1;
            case WM_NCDESTROY:
                RemoveWindowSubclass(hWnd, ScrollViewSubclassProc, uIdSubclass);
                break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT ScrollView::OnNcPaint(HWND hWnd)
    {
        {
            HDC hdc = GetWindowDC(hWnd);
            using namespace Gdiplus;
            Graphics graphics(hdc);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);

            RECT rcWindow;
            GetWindowRect(hWnd, &rcWindow);

            // Convert to client-relative coordinates for non-client area
            int width = rcWindow.right - rcWindow.left;
            int height = rcWindow.bottom - rcWindow.top;

            // Calculate scrollbar rectangle (right side)
            RECT rcScrollbar =
            {
                width - GetSystemMetrics(SM_CXVSCROLL),
                0,
                width,
                height
            };

            RectF rect = ToRectF(rcScrollbar);
            rect.Inflate(0.5, 0.5);
            SolidBrush backBrush(m_theme.GetColor(Theme::Colors::Dialog));
            graphics.FillRectangle(&backBrush, rect);

            if (m_contentHeight > height)
            {
                SolidBrush thumbBrush(m_theme.GetColor(Theme::Colors::ScrollThumb));
                Pen thumbPen(m_theme.GetColor(Theme::Colors::ScrollThumb), 0.5);

                rect.X = rect.GetRight() - 13.0f;
                rect.Width = 3;

                rect.Height = max(((FLOAT)m_viewHeight * (height / (FLOAT)m_contentHeight)), 20.0f);
                rect.Y = (FLOAT)m_scrollPos * (height - rect.Height) / (m_contentHeight - height);

                RoundRect(graphics, rect, rect.Width, &thumbBrush, thumbPen);
            }
            ReleaseDC(hWnd, hdc);
            return 0;
        }
    }

    void ScrollView::SetOffset(int newOffset)
    {
    }

    ScrollView::ScrollView(Theme &theme)
        : m_theme(theme)
    {}

    ScrollView::ScrollView(Theme &theme, HWND hParent, int x, int y, int width, int height)
        : ScrollView(theme)
    {
        m_viewHeight = height;
        m_contentHeight = m_viewHeight;
        m_scrollPos = 0;

        Create(hParent, x, y, width, height);
    }

    HWND ScrollView::Create(HWND hParent, int x, int y, int width, int height)
    {
        m_hScrollView = CreateWindowEx(
            WS_EX_CONTROLPARENT,
            L"STATIC",
            L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VSCROLL,
            x, y, width, height,
            hParent, NULL, GetModuleHandle(NULL), NULL);

        m_viewHeight = height;
        UpdateScrollBar();
        SetWindowSubclass(m_hScrollView, ScrollViewSubclassProc, 0, (DWORD_PTR)this);
        return m_hScrollView;
    }

    void ScrollView::SetContentHeight(int newHeight)
    {
        m_contentHeight = newHeight;
        if (m_contentHeight <= m_viewHeight)
        {
            m_contentHeight = m_viewHeight;
            ScrollTo(0);
        }
        else if (m_scrollPos + m_viewHeight > m_contentHeight)
        {
            ScrollTo(m_contentHeight - m_viewHeight);
        }
        UpdateScrollBar();

    }

    void ScrollView::UpdateScrollBar()
    {
        RedrawWindow(m_hScrollView, NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
    }

    void ScrollView::ScrollTo(int newPos)
    {
        int maxPos = m_contentHeight - m_viewHeight;
        if (newPos < 0)
            newPos = 0;
        if (newPos > maxPos)
            newPos = maxPos;

        if (newPos != m_scrollPos)
        {
            int delta = m_scrollPos - newPos;
            m_scrollPos = newPos;

            // Scroll the window content
            ScrollWindowEx(m_hScrollView, 0, delta, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN );
            UpdateScrollBar();
            RedrawWindow(m_hScrollView, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
    }

    void ScrollView::EnsureVisible(const RECT &rcItem)
    {
        if (m_contentHeight <= m_viewHeight)
        {
            return; // No scrolling needed if content fits
        }

        int margin = m_theme.DpiScale(10); // Optional margin around the item
        int itemTop = rcItem.top - margin;
        int itemBottom = rcItem.bottom + margin;
        int visibleTop = m_scrollPos;
        int visibleBottom = m_scrollPos + m_viewHeight;

        // Check if item is already fully visible
        if (itemTop >= visibleTop && itemBottom <= visibleBottom)
        {
            return; // Already fully visible
        }

        int newScrollPos = m_scrollPos;

        // If item is above visible area, scroll to show it at top
        if (itemTop < visibleTop)
        {
            newScrollPos = itemTop;
        }
        // If item is below visible area, scroll to show it at bottom
        else if (itemBottom > visibleBottom)
        {
            newScrollPos = itemBottom - m_viewHeight;
        }

        // If item is taller than viewport, at least show the top
        if (itemBottom - itemTop > m_viewHeight)
        {
            newScrollPos = itemTop;
        }

        ScrollTo(newScrollPos);
    }

    void ScrollView::ScrollBy(int delta)
    {
        ScrollTo(m_scrollPos + delta);
    }

    int ScrollView::GetScrollPos() const { return m_scrollPos; }

    // Call this when the window is resized
    void ScrollView::OnResize(int newWidth, int newHeight)
    {
        m_viewHeight = newHeight;
        UpdateScrollBar();
    }

    ScrollView::~ScrollView()
    {
    }

    LRESULT ScrollView::OnVScroll(int nScrollCode, int nPos)
    {
        int pageSize = m_viewHeight;
        int lineSize = 16; // You can adjust this

        switch (nScrollCode)
        {
        case SB_LINEUP: // Scroll up one line
            ScrollBy(-lineSize);
            break;

        case SB_LINEDOWN: // Scroll down one line
            ScrollBy(lineSize);
            break;

        case SB_PAGEUP: // Scroll up one page
            ScrollBy(-pageSize);
            break;

        case SB_PAGEDOWN: // Scroll down one page
            ScrollBy(pageSize);
            break;

        case SB_THUMBTRACK:    // Drag scroll thumb
        case SB_THUMBPOSITION: // Scroll to absolute position
            ScrollTo(nPos);
            break;

        case SB_TOP: // Scroll to top
            ScrollTo(0);
            break;

        case SB_BOTTOM: // Scroll to bottom
            ScrollTo(m_contentHeight);
            break;
        }

        return 0;
    }

    LRESULT ScrollView::OnMouseWheel(int delta)
    {
        // Typically, WHEEL_DELTA is 120, scroll 3 lines per wheel click
        if (m_contentHeight <= m_viewHeight || GetCapture())
        {
            return 0; // No scrolling needed if content fits
        }

        int scrollAmount = -delta / WHEEL_DELTA * 16 * 3;
        ScrollBy(scrollAmount);
        return 0;
    }
}