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


#include <windows.h>
#include <string>
#include <functional>
#include <Uxtheme.h>
#include "Tools/Icon.hpp"
#include "Popup.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"
#include "Popup.hpp"

namespace FluentDesign
{
    void Popup::Show(HWND hParent, int x, int y, const std::vector<PopupItem>& items, int width, int flags, int selectedIndex)
    {
        if (m_popupVisible)
            return;

        if (items.empty())
            return;

        m_popupItems = items;
        m_selectedIndex = (selectedIndex >= 0 && selectedIndex < (int)items.size()) ? selectedIndex : -1;
        m_originalIndex = m_selectedIndex;
        m_hoveredIndex = -1;
        m_nPopupScrollPos = 0;
        m_nPopupViewHeight = min(m_theme.DpiScale(400), (int)items.size() * m_theme.DpiScale(Layout_ItemHeight));
        m_nPopupContentHeight = (int)items.size() * m_theme.DpiScale(Layout_ItemHeight);

        int popupWidth = width;
        int popupHeight = m_nPopupViewHeight;

        MONITORINFO mi{sizeof(MONITORINFO)};
        int monitorHeight = 2160;
        int monitorWidth = 3840;

        if (GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi))
        {
            monitorHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
            monitorWidth = mi.rcMonitor.right - mi.rcMonitor.left;
        }

        int dx = (flags & TPM_RIGHTALIGN || x + popupWidth > monitorWidth ) ? -popupWidth : (flags & TPM_CENTERALIGN) ? -popupWidth / 2 : 0;
        int dy = (flags & TPM_BOTTOMALIGN || y + popupHeight + m_theme.DpiScale(Layout_ItemHeight)  > monitorHeight ) ? -popupHeight : 0;


        // Create popup listbox window
        m_hWnd = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"LISTBOX",
            L"",
            WS_CHILD | WS_POPUP | LBS_NOINTEGRALHEIGHT | CS_DROPSHADOW | WS_TABSTOP,
            x + dx, y + dy,
            popupWidth,
            popupHeight,
            hParent,
            NULL,
            GetModuleHandle(NULL),
            NULL);

        // Set transparency for shadow
        SetLayeredWindowAttributes(m_hWnd, 0, 255, LWA_COLORKEY);

        for (size_t i = 0; i < m_popupItems.size(); ++i)
        {
            int lbIndex = (int)SendMessage(m_hWnd, LB_ADDSTRING, 0, (LPARAM)m_popupItems[i].name.c_str());
            SendMessage(m_hWnd, LB_SETITEMDATA, lbIndex, (LPARAM)&(m_popupItems[i]));
        }

        // Subclass the popup listbox
        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
        SetWindowSubclass(m_hWnd, PopupSubclassProc, 0, (DWORD_PTR)this);

        // Set item height
        SendMessage(m_hWnd, LB_SETITEMHEIGHT, 0, (LPARAM)m_theme.DpiScale(Layout_ItemHeight));

        ShowWindow(m_hWnd, SW_SHOW);
        m_popupVisible = true;

        // Capture mouse to close when clicking outside
        SetFocus(m_hWnd);
        SetCapture(m_hWnd);

        EnsureVisible(m_selectedIndex);
    }

    void Popup::Hide()
    {
        if (m_popupVisible && m_hWnd)
        {
            HWND hOwner = GetWindow(m_hWnd, GW_OWNER);
            ReleaseCapture();
            DestroyWindow(m_hWnd);
            m_hWnd = NULL;
            m_popupVisible = false;
            if (hOwner)
            {
                SetFocus(hOwner);
            }
        }
    }

    void Popup::DrawPopupBackground(HWND hWnd, HDC hdc, RECT rect)
    {
        using namespace Gdiplus;

        HBRUSH hNilBrush = CreateSolidBrush(RGB(0,0,0));
        FillRect(hdc, &rect, hNilBrush);

        RectF rectF = ToRectF(rect);
        rectF.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));
        Pen borderPen(m_theme.GetColor(Theme::Colors::ComboPopupBorder), m_theme.DpiScaleF(1));
        SolidBrush bgBrush(m_theme.GetColor(Theme::Colors::ComboPopup));

        RoundRect(Graphics(hdc), rectF, (float)m_theme.GetSize_Corner(), &bgBrush, borderPen);

        if (m_nPopupContentHeight > m_nPopupViewHeight)
        {
            rect.right -= m_theme.DpiScale(4);
            rect.left = rect.right - m_theme.DpiScale(2);
            rect.top += m_theme.DpiScale(2);

            int height = rect.bottom - rect.top;
            int thumbHeight = max((m_nPopupViewHeight * height / m_nPopupContentHeight), m_theme.DpiScale(20));
            rect.top = m_theme.DpiScale(2) + m_nPopupScrollPos * (height - thumbHeight) / (m_nPopupContentHeight - height);
            rect.bottom = rect.top + thumbHeight;

            RectF rectF = ToRectF(rect);
            DWORD color = m_theme.GetColor(false ? Theme::Colors::ScrollThumb : Theme::Colors::ScrollThumbStroke);
            SolidBrush thumbBrush(color);
            Pen thumbPen(color, 0.5);

            RoundRect(Graphics(hdc), rectF, rectF.Width, &thumbBrush, thumbPen);

        }
    }

    void Popup::DrawPopupItemBackground(HWND hWnd, HDC hdc, RECT itemRect, int itemId)
    {
        using namespace Gdiplus;

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        RectF backgroundRect = ToRectF(itemRect);
        backgroundRect.Inflate(m_theme.DpiScaleF(-Layout_LeftMargin/4), m_theme.DpiScaleF(-2));

        if ( itemId == m_hoveredIndex || itemId == m_selectedIndex)
        {
            Color color = m_theme.GetColor(
                itemId == m_hoveredIndex
                    ? Theme::Colors::ComboPopupHover
                    : Theme::Colors::ComboPopupSelected);
            SolidBrush hoverBrush(color);
            Pen borserPen(color, 1);
            Gdiplus::RoundRect(graphics, backgroundRect, (float)m_theme.GetSize_Corner(), &hoverBrush, borserPen);
        }
        if (itemId == m_selectedIndex)
        {
            SolidBrush gripBrush(m_theme.GetColor(Theme::Colors::ComboPopupSelectedMark));
            RectF gripRect = backgroundRect;
            gripRect.X += 2;
            gripRect.Width = m_theme.DpiScaleF(3);
            gripRect.Y += (gripRect.Height - m_theme.DpiScale(16)) / 2;
            gripRect.Height = m_theme.DpiScaleF(16);
            graphics.FillRectangle(&gripBrush, gripRect);
        }
    }
    void Popup::DrawPopupItem(HWND hWnd, HDC hdc, RECT itemRect, int itemId)
    {

        DrawPopupItemBackground(hWnd, hdc, itemRect, itemId);

        using namespace Gdiplus;

        Graphics graphics(hdc);

        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        RectF rectF = ToRectF(itemRect);
        rectF.X += m_theme.DpiScale(Layout_LeftMargin);
        rectF.Width -= m_theme.DpiScale(Layout_LeftMargin);
        PopupItem *pItem = (PopupItem*)SendMessage(m_hWnd, LB_GETITEMDATA, itemId, 0);
        if (!pItem)
        {
            return;
        }
        if (pItem->draw)
        {
            pItem->draw(hWnd, hdc, itemRect, itemId, itemId == m_selectedIndex, itemId == m_hoveredIndex);
            return;
        }

        SolidBrush textBrush(Color(m_theme.GetColor(Theme::Colors::Text)));
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);

        if (!pItem->icon.empty())
        {
            RectF iconRect = rectF;
            iconRect.Width = m_theme.DpiScaleF(Layout_ImageSize);
            iconRect.Y += m_theme.DpiScaleF(2);
            Font font(hdc, m_theme.GetFont_GlyphNormal());

            graphics.DrawString(pItem->icon.c_str(), -1, &font, iconRect, &format, &textBrush);
            rectF.X += m_theme.DpiScaleF(Layout_ImageSize + Layout_IconMargin);
            rectF.Width -= m_theme.DpiScaleF(Layout_ImageSize + Layout_IconMargin);
        }
        format.SetAlignment(StringAlignmentNear);

        Font font(hdc, m_theme.GetFont_Text());
        graphics.DrawString(pItem->name.c_str(), -1, &font, rectF, &format, &textBrush);
    }

    void Popup::OnPaint(HWND hWnd)
    {
        FluentDesign::DoubleBuferedPaint paint(hWnd);

        DrawPopupBackground(hWnd, paint.MemDC(), paint.ClientRect());

        int size = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0);
        for (int i = 0; i < size; i++)
        {
            RECT itemRect;
            SendMessage(hWnd, LB_GETITEMRECT, i, (LPARAM)&itemRect);
            OffsetRect(&itemRect, 0, -m_nPopupScrollPos);
            if (m_nPopupContentHeight > m_nPopupViewHeight)
            {
                itemRect.right -= m_theme.DpiScale(6);
            }
            DrawPopupItem(hWnd, paint.MemDC(), itemRect, i);
        }

        if (m_theme.IsKeyboardFocused() && m_selectedIndex >= 0)
        {
            RECT itemRect;
            SendMessage(hWnd, LB_GETITEMRECT, m_selectedIndex, (LPARAM)&itemRect);
            InflateRect(&itemRect, m_theme.DpiScale(-Layout_LeftMargin/4), m_theme.DpiScale(-2));
            OffsetRect(&itemRect, 0, -m_nPopupScrollPos);
            if (m_nPopupContentHeight > m_nPopupViewHeight)
            {
                itemRect.right -= m_theme.DpiScale(6);
            }
            m_theme.DrawFocusFrame(paint.MemDC(), itemRect, 0);
        }
    }

    void Popup::OnMouseMove(HWND hWnd, LPARAM lParam)
    {
        SetCursor(LoadCursor(NULL, IDC_ARROW));

        // Handle item hover
        POINT pt = {LOWORD(lParam), HIWORD(lParam)};
        pt.y += m_nPopupScrollPos;
        RECT rc;
        GetClientRect(hWnd, &rc);
        int hoverIndex = pt.y / m_theme.DpiScale(Layout_ItemHeight);
        m_hoveredIndex = (pt.x >= rc.left && pt.x <= rc.right && hoverIndex >= 0 && hoverIndex < (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0))
                             ? hoverIndex
                             : -1;
        InvalidateRect(hWnd, NULL, TRUE);
    }

    void Popup::OnLButtonDown(HWND hWnd, LPARAM lParam)
    {
        POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
        pt.y += m_nPopupScrollPos;
        RECT rc;
        GetClientRect(hWnd, &rc);
        int clickIndex = pt.y / m_theme.DpiScale(Layout_ItemHeight);
        if (pt.x >= rc.left && pt.x <=rc.right && clickIndex >= 0 && clickIndex < (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0))
        {
            HandleListClick(clickIndex);
        }
        else
        {
            m_selectedIndex = m_originalIndex;
            OnSelectionChanged.Notify();
        }
        Hide();
    }

    void Popup::OnCaptureChanged(HWND hWnd, LPARAM lParam)
    {
        // If we lose capture, hide the popup
        if ((HWND)lParam != hWnd)
        {
            m_selectedIndex = m_originalIndex;
            OnSelectionChanged.Notify();
            Hide();
        }
        else
        {
            SetCapture(hWnd);
        }
    }

    void Popup::OnKeyDown(HWND hWnd, WPARAM wParam)
    {
        switch (wParam)
        {
            case VK_ESCAPE:
            case VK_GAMEPAD_B:
                m_selectedIndex = m_originalIndex;
                OnSelectionChanged.Notify();
                Hide();
                return;

            case VK_RETURN:
            case VK_SPACE:
            case VK_GAMEPAD_A:
                HandleListClick(m_selectedIndex);
                Hide();
                return;
            case VK_UP:
            case VK_GAMEPAD_DPAD_UP:
                if (m_selectedIndex >0 )
                {
                    m_selectedIndex -= 1;
                    EnsureVisible(m_selectedIndex);
                    InvalidateRect(m_hWnd, NULL, TRUE);
                    OnSelectionChanged.Notify();
                }
                m_theme.SetKeyboardFocused(m_hWnd);
                return;
            case VK_DOWN:
            case VK_GAMEPAD_DPAD_DOWN:
                if (m_selectedIndex < SendMessage(hWnd, LB_GETCOUNT, 0, 0) - 1 )
                {
                    m_selectedIndex += 1;
                    EnsureVisible(m_selectedIndex);
                    InvalidateRect(m_hWnd, NULL, TRUE);
                    OnSelectionChanged.Notify();
                }
                m_theme.SetKeyboardFocused(m_hWnd);
                return;
        }
    }

    // Single popup listbox window procedure
    LRESULT CALLBACK Popup::PopupSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        Popup *This = reinterpret_cast<Popup *>(dwRefData);

        switch (uMsg)
        {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            This->OnPaint(hWnd);
            return 0;

        case WM_NCPAINT:
            return 0;

        case WM_MOUSEMOVE:
            This->OnMouseMove(hWnd, lParam);
            break;

        case WM_LBUTTONDOWN:
            This->OnLButtonDown(hWnd, lParam);
            return 0;

        case WM_CAPTURECHANGED:
            This->OnCaptureChanged(hWnd, lParam);
            break;

        case WM_KEYDOWN:
            This->OnKeyDown(hWnd, wParam);
            return 0;

        case WM_DESTROY:
            RemoveWindowSubclass(hWnd, PopupSubclassProc, uIdSubclass);
            break;

        case WM_MOUSEWHEEL:
        {
            // Typically, WHEEL_DELTA is 120, scroll 3 lines per wheel click
            if (This->m_nPopupContentHeight <= This->m_nPopupViewHeight)
            {
                return 0; // No scrolling needed if content fits
            }

            int scrollAmount = -GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * 16 * 3;
            This->ScrollTo(This->m_nPopupScrollPos + scrollAmount);

            return 0;
        }
        return 0;

        }

        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void Popup::ScrollTo(int newPos)
    {
        int maxPos = m_nPopupContentHeight - m_nPopupViewHeight;
        if (newPos < 0)
            newPos = 0;
        if (newPos > maxPos)
            newPos = maxPos;

        if (newPos != m_nPopupScrollPos)
        {
            int delta = m_nPopupScrollPos - newPos;
            m_nPopupScrollPos = newPos;
            // Scroll the window content
            ScrollWindowEx(m_hWnd, 0, delta, NULL, NULL, NULL, NULL, SW_INVALIDATE );
            RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE);
        }
    }

    void Popup::EnsureVisible(int index)
    {
        if (index < 0 || m_nPopupContentHeight <= m_nPopupViewHeight)
        {
            return;
        }

        RECT rcItem;
        SendMessage(m_hWnd, LB_GETITEMRECT, index, (LPARAM)&rcItem);

        int margin = m_theme.DpiScale(Layout_ItemHeight); // Optional margin around the item
        int itemTop = rcItem.top - margin;
        int itemBottom = rcItem.bottom + margin;
        int visibleTop = m_nPopupScrollPos;
        int visibleBottom = m_nPopupScrollPos + m_nPopupViewHeight;

        // Check if item is already fully visible
        if (itemTop >= visibleTop && itemBottom <= visibleBottom)
        {
            return; // Already fully visible
        }

        int newScrollPos = m_nPopupScrollPos;

        // If item is above visible area, scroll to show it at top
        if (itemTop < visibleTop)
        {
            newScrollPos = itemTop;
        }
        // If item is below visible area, scroll to show it at bottom
        else if (itemBottom > visibleBottom)
        {
            newScrollPos = itemBottom - m_nPopupViewHeight;
        }

        // If item is taller than viewport, at least show the top
        if (itemBottom - itemTop > m_nPopupViewHeight)
        {
            newScrollPos = itemTop;
        }

        ScrollTo(newScrollPos);
    }


    void Popup::HandleListClick(int index)
    {
        if (index >= 0 && index < (int)SendMessage(m_hWnd, LB_GETCOUNT, 0, 0))
        {
            m_selectedIndex = index;
            OnSelectionChanged.Notify();
            PopupItem *pItem = (PopupItem*)SendMessage(m_hWnd, LB_GETITEMDATA, index, 0);
            if (pItem && pItem->callback)
            {
                pItem->callback();
            }
        }
    }
}
