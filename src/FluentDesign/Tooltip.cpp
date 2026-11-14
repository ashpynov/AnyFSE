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


#include <sstream>
#include "Tooltip.hpp"
#include "App/Constants.hpp"
#include "Tools/DoubleBufferedPaint.hpp"

namespace FluentDesign
{
    Tooltip::~Tooltip()
    {
        Hide(false);
    }

    void Tooltip::Show(HWND hParent, const std::wstring& text, int x, int y)
    {
        Hide(false);
        if (text.empty() || !IsWindow(hParent))
            return;

        MONITORINFO monitor {sizeof(monitor)};
        if (!GetMonitorInfo(MonitorFromPoint(POINT{x, y}, MONITOR_DEFAULTTONEAREST), &monitor))
            return;

        const int padding = m_theme.DpiScale(Padding);
        const int margin = m_theme.DpiScale(MonitorMargin);
        const RECT& work = monitor.rcWork;
        const int availableWidth = max(1, work.right - work.left - 2 * margin);
        const int textWidth = max(1, min(m_theme.DpiScale(MaxTextWidth), availableWidth - 2 * padding));

        HDC hdc = GetDC(hParent);
        if (!hdc)
            return;
        const SIZE textSize = LayoutText(hdc, text, RECT{0, 0, textWidth, 0}, false);
        ReleaseDC(hParent, hdc);

        const int width = min(availableWidth, textSize.cx + 2 * padding);
        const int height = min(max(1, work.bottom - work.top - 2 * margin), textSize.cy + 2 * padding);
        const int left = max(work.left + margin, min(x - width, work.right - margin - width));
        const int top = max(work.top + margin, min(y + height > work.bottom - margin ? y - height : y, work.bottom - margin - height));

        m_text = text;
        m_previousFocus = GetFocus();
        m_hWnd = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            Constants::TooltipWindowClass, m_text.c_str(), WS_POPUP,
            left, top, width, height, hParent, nullptr, GetModuleHandle(nullptr), nullptr);
        if (!m_hWnd)
            return;
        if (!SetWindowSubclass(m_hWnd, WndProc, 0, reinterpret_cast<DWORD_PTR>(this)))
        {
            Hide(false);
            return;
        }

        SetLayeredWindowAttributes(m_hWnd, RGB(0, 0, 0), 255, LWA_COLORKEY);
        ShowWindow(m_hWnd, SW_SHOW);
        m_theme.SwapFocus(m_hWnd);
        if (m_hWnd)
        {
            GetCursorPos(&m_mousePosition);
            SetCapture(m_hWnd);
        }
    }

    void Tooltip::Hide(bool restoreFocus)
    {
        if (!m_hWnd)
            return;

        HWND hWnd = m_hWnd;
        HWND previousFocus = m_previousFocus;
        const bool focused = GetFocus() == hWnd;
        m_hWnd = nullptr;
        m_previousFocus = nullptr;
        if (GetCapture() == hWnd)
            ReleaseCapture();
        DestroyWindow(hWnd);
        if (restoreFocus && focused && IsWindow(previousFocus) && IsWindowVisible(previousFocus) && IsWindowEnabled(previousFocus))
            m_theme.SwapFocus(previousFocus);
    }

    SIZE Tooltip::LayoutText(HDC hdc, const std::wstring& text, const RECT& bounds, bool draw)
    {
        HGDIOBJ oldFont = SelectObject(hdc, m_theme.GetFont_Text());
        TEXTMETRIC metrics {};
        GetTextMetrics(hdc, &metrics);
        const int lineHeight = max(metrics.tmHeight, m_theme.GetSize_GlyphNormal());
        const int iconWidth = m_theme.DpiScale(IconWidth);
        const int iconIndent = m_theme.DpiScale(IconWidth + IconSpacing);
        SIZE size {};
        std::wistringstream lines(text);
        std::wstring line;
        while (std::getline(lines, line))
        {
            if (!line.empty() && line.back() == L'\r')
                line.pop_back();
            const bool hasIcon = !line.empty() && line.front() >= FirstIcon && line.front() <= LastIcon;
            wchar_t icon = 0;
            if (hasIcon)
            {
                icon = line.front();
                line.erase(0, 1);
                if (!line.empty() && line.front() == L' ')
                    line.erase(0, 1);
            }
            const int indent = hasIcon ? iconIndent : 0;
            RECT measured {bounds.left + indent, 0, bounds.right, 0};
            if (line.empty())
                measured.right = measured.left;
            else
                ::DrawText(hdc, line.c_str(), -1, &measured, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
            const int height = max(lineHeight, measured.bottom);
            if (draw)
            {
                RECT row {bounds.left + indent, bounds.top + size.cy, bounds.right, bounds.top + size.cy + height};
                ::DrawText(hdc, line.c_str(), -1, &row, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
                if (hasIcon)
                {
                    RECT iconRect {bounds.left, row.top, bounds.left + iconWidth, row.top + lineHeight};
                    SelectObject(hdc, m_theme.GetFont_GlyphNormal());
                    ::DrawText(hdc, &icon, 1, &iconRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                    SelectObject(hdc, m_theme.GetFont_Text());
                }
            }
            size.cx = max(size.cx, measured.right - bounds.left);
            size.cy += height;
        }
        SelectObject(hdc, oldFont);
        return size;
    }

    void Tooltip::OnPaint(HDC targetDC)
    {
        DoubleBuferedPaint paint(m_hWnd, targetDC);
        HDC hdc = paint.MemDC();
        RECT rect = paint.ClientRect();
        FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        {
            Gdiplus::Graphics graphics(hdc);
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            Gdiplus::RectF bounds = Gdiplus::ToRectF(rect);
            bounds.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));
            Gdiplus::Pen border(m_theme.GetColor(Theme::Colors::ComboPopupBorder), m_theme.DpiScaleF(1));
            Gdiplus::SolidBrush background(m_theme.GetColor(Theme::Colors::ComboPopup));
            Gdiplus::RoundRect(graphics, bounds, static_cast<float>(m_theme.GetSize_Corner()), &background, border);
        }

        const int padding = m_theme.DpiScale(Padding);
        InflateRect(&rect, -padding, -padding);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, m_theme.GetColorRef(Theme::Colors::Text));
        LayoutText(hdc, m_text, rect, true);
    }

    LRESULT CALLBACK Tooltip::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        Tooltip* tooltip = reinterpret_cast<Tooltip*>(data);
        switch (message)
        {
        case WM_PAINT:
            tooltip->OnPaint();
            return 0;
        case WM_PRINTCLIENT:
            tooltip->OnPaint(reinterpret_cast<HDC>(wParam));
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
        {
            POINT position;
            if (GetCursorPos(&position) &&
                (position.x != tooltip->m_mousePosition.x || position.y != tooltip->m_mousePosition.y))
                tooltip->Hide();
            return 0;
        }
        case WM_CAPTURECHANGED:
        case WM_CANCELMODE:
            tooltip->Hide(false);
            return 0;
        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_CHAR:
        case WM_SYSCHAR:
            tooltip->Hide();
            return 0;
        case WM_KILLFOCUS:
            tooltip->Hide(false);
            return 0;
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
                tooltip->Hide(false);
            return 0;
        case WM_NCDESTROY:
            if (tooltip->m_hWnd == hWnd)
            {
                tooltip->m_hWnd = nullptr;
                tooltip->m_previousFocus = nullptr;
            }
            RemoveWindowSubclass(hWnd, WndProc, id);
            break;
        }
        return DefSubclassProc(hWnd, message, wParam, lParam);
    }
}
