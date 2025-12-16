
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
#include "Tools/Window.hpp"

namespace FluentDesign
{
    LRESULT ScrollView::ScrollViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        ScrollView *This = reinterpret_cast<ScrollView *>(dwRefData);
        switch (uMsg)
        {
            case WM_SETFOCUS:
                SetFocus(GetWindow(hWnd, GW_CHILD));
                break;

            case WM_SIZE:
                This->OnResize(LOWORD(lParam), HIWORD(lParam));
                break;

            case WM_NCHITTEST:
                return HTCLIENT;

            case WM_VSCROLL:
                return This->OnVScroll(LOWORD(wParam), HIWORD(wParam));

            case WM_MOUSEWHEEL:
                return This->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));

            case WM_MOUSEMOVE:
                return This->OnMouseMove();

            case WM_MOUSELEAVE:
                return This->OnMouseLeave();

            case WM_LBUTTONDOWN:
                return This->OnLButtonDown();

            case WM_LBUTTONUP:
                return This->OnLButtonUp();

            case WM_PAINT:
                return This->OnPaint(hWnd);

            case WM_ERASEBKGND:
                return This->OnEraseBkgnd((HDC)wParam, (HWND)lParam);

            case WM_NCDESTROY:
                RemoveWindowSubclass(hWnd, ScrollViewSubclassProc, uIdSubclass);
                break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT ScrollView::OnEraseBkgnd(HDC hdc, HWND child)
    {
        if(child)
        {
            HWND parent = GetParent(m_hWnd);
            if (parent)
            {
                SendMessage(parent, WM_ERASEBKGND, (WPARAM)hdc, (LPARAM)m_hWnd);
            }
        }
        return 1;
    }

    LRESULT ScrollView::OnMouseMove()
    {
        if (m_dragging)
        {
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            MapWindowPoints(NULL, m_hWnd, &cursorPos, 1);

            int deltaMouseY = cursorPos.y - m_dragStartMousePos;
            float scale = m_viewHeight ? (float) m_contentHeight / m_viewHeight : 0;
            ScrollTo((int)(deltaMouseY * scale + m_dragStartScrollPos));
        }
        else if (Window::MouseInClientRect(m_hWnd, &m_scrollBarRect))
        {
            if (!m_hovered)
            {
                m_hovered = true;
                UpdateScrollBar();
            }

            TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd};
            TrackMouseEvent(&tme);
            SetCapture(m_hWnd);
        }
        else if (m_hovered)
        {
            m_hovered = false;
            ReleaseCapture();
            UpdateScrollBar();
        }

        return 0;
    };

    LRESULT ScrollView::OnMouseLeave()
    {
        return 0;
    };

    LRESULT ScrollView::OnLButtonDown()
    {
        if (!Window::MouseInClientRect(m_hWnd, &m_scrollBarRect))
        {
            return 0;
        }

        POINT cursorPos;
        GetCursorPos(&cursorPos);
        MapWindowPoints(NULL, m_hWnd, &cursorPos, 1);

        if ( cursorPos.y > m_thumbRect.top && cursorPos.y < m_thumbRect.bottom)
        {
            m_dragStartMousePos = cursorPos.y;
            m_dragStartScrollPos = m_scrollPos;
            SetCapture(m_hWnd);
            m_dragging = true;
        }
        else if (cursorPos.y < m_thumbRect.top)
        {
            ScrollBy(-m_viewHeight);
        }
        else if (cursorPos.y > m_thumbRect.bottom)
        {
            ScrollBy(m_viewHeight);
        }

        return 0;
    };

    LRESULT ScrollView::OnLButtonUp()
    {
        m_dragging = false;
        if (!Window::MouseInClientRect(m_hWnd, &m_scrollBarRect))
        {
            m_hovered = false;
            UpdateScrollBar();
            ReleaseCapture();
        }
        return 0;
    };

    LRESULT ScrollView::OnPaint(HWND hWnd)
    {
        using namespace Gdiplus;
        FluentDesign::DoubleBuferedPaint paint(hWnd);
        Graphics graphics(paint.MemDC());
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        RectF rect = ToRectF(m_scrollBarRect);

        if (rect.Width > 0 && m_hovered)
        {
            //rect.Inflate(m_theme.DpiScaleF(-2), 0);
            SolidBrush backBrush(m_theme.GetColor(Theme::Colors::ScrollTrack));
            Pen thumbPen(m_theme.GetColor(Theme::Colors::ScrollTrack), 0.5);
            RoundRect(graphics, rect, rect.Width, &backBrush, thumbPen);
        }

        rect = ToRectF(m_thumbRect);

        if (rect.Width > 0)
        {
            DWORD color = m_theme.GetColor(m_hovered ? Theme::Colors::ScrollThumb : Theme::Colors::ScrollThumbStroke);
            SolidBrush thumbBrush(color);
            Pen thumbPen(color, 0.5);

            RoundRect(graphics, rect, rect.Width, &thumbBrush, thumbPen);
        }
        return 0;
    }

    void ScrollView::SetOffset(int newOffset)
    {
    }

    void ScrollView::CalculateRects()
    {
        RECT rcWindow;
        GetClientRect(m_hWnd, &rcWindow);

        int height = rcWindow.bottom - rcWindow.top;

        // Calculate scrollbar rectangle (right side)
        m_scrollBarRect =
        {
            rcWindow.right - m_theme.DpiScale(10),
            0,
            rcWindow.right,
            rcWindow.bottom
        };

        int thumbWidth = 0;
        int thumbHeight = 0;

        if (m_contentHeight && m_contentHeight > height)
        {
            thumbWidth = m_theme.DpiScale(m_hovered ? 6 : 2);

            m_thumbRect.left = m_scrollBarRect.right - thumbWidth;
            m_thumbRect.right = m_thumbRect.left + thumbWidth;

            thumbHeight = max((m_viewHeight * height / m_contentHeight), m_theme.DpiScale(20));
            m_thumbRect.top = m_scrollPos * (height - thumbHeight) / (m_contentHeight - height);
            m_thumbRect.bottom = m_thumbRect.top + thumbHeight;
        }
        else
        {
            SetRectEmpty(&m_scrollBarRect);
            SetRectEmpty(&m_thumbRect);
        }
    }

    ScrollView::ScrollView(Theme &theme)
        : FluentControl(theme)
        , m_hovered(false)
        , m_dragging(false)
        , m_thumbRect{0}
        , m_scrollBarRect{0}
        , m_contentHeight(0)
        , m_scrollPos(0)
        , m_dragStartMousePos(0)
        , m_dragStartScrollPos(0)
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
        m_hWnd = CreateWindowEx(
            WS_EX_CONTROLPARENT,
            L"STATIC",
            L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP,
            x, y, width, height,
            hParent, NULL, GetModuleHandle(NULL), NULL);

        m_viewHeight = height;
        UpdateScrollBar();
        SetWindowSubclass(m_hWnd, ScrollViewSubclassProc, 0, (DWORD_PTR)this);
        return m_hWnd;
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
        CalculateRects();
        RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
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
            ScrollWindowEx(m_hWnd, 0, delta, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN );
            UpdateScrollBar();
            RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
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
        HWND hChild = GetWindow(m_hWnd, GW_CHILD);
        while (hChild)
        {
            RECT rc;
            GetClientRect(hChild, &rc);
            SendMessage(hChild, WM_SIZE, 0, MAKELONG(rc.right - rc.left, rc.bottom - rc.top));
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
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