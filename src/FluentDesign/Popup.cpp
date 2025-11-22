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
#include "Tools/Tools.hpp"
#include "Popup.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"
#include "Popup.hpp"

namespace FluentDesign
{
    void Popup::Show(HWND hParent, int x, int y, const std::vector<PopupItem>& items, int width)
    {
        if (m_popupVisible)
            return;

        m_popupItems = items;
        m_selectedIndex = -1;
        m_hoveredIndex = -1;

        int popupWidth = width;
        int popupHeight = min(m_theme.DpiScale(400), (int)items.size() * m_theme.DpiScale(Layout_ItemHeight));

        // Create popup listbox window
        m_hPopupList = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"LISTBOX",
            L"",
            WS_CHILD | WS_POPUP | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | CS_DROPSHADOW | WS_TABSTOP,
            x - popupWidth, y,
            popupWidth,
            popupHeight,
            hParent,
            NULL,
            GetModuleHandle(NULL),
            NULL);

        // Set transparency for shadow
        SetLayeredWindowAttributes(m_hPopupList, 0, 255, LWA_COLORKEY);

        for (size_t i = 0; i < m_popupItems.size(); ++i)
        {
            int lbIndex = (int)SendMessage(m_hPopupList, LB_ADDSTRING, 0, (LPARAM)m_popupItems[i].name.c_str());
            SendMessage(m_hPopupList, LB_SETITEMDATA, lbIndex, (LPARAM)&(m_popupItems[i]));
        }

        // Subclass the popup listbox
        SetWindowSubclass(m_hPopupList, PopupSubclassProc, 0, (DWORD_PTR)this);

        // Set item height
        SendMessage(m_hPopupList, LB_SETITEMHEIGHT, 0, (LPARAM)m_theme.DpiScale(Layout_ItemHeight));

        ShowWindow(m_hPopupList, SW_SHOW);
        m_popupVisible = true;

        // Capture mouse to close when clicking outside
        SetFocus(m_hPopupList);
        SetCapture(m_hPopupList);
    }

    void Popup::Hide()
    {
        if (m_popupVisible && m_hPopupList)
        {
            ReleaseCapture();
            DestroyWindow(m_hPopupList);
            m_hPopupList = NULL;
            m_popupVisible = false;
        }
    }

    void Popup::DrawPopupBackground(HWND hWnd, HDC hdc, RECT rect)
    {
        using namespace Gdiplus;

        HBRUSH hNilBrush = CreateSolidBrush(RGB(0,0,0));
        FillRect(hdc, &rect, hNilBrush);

        RectF rectF = ToRectF(rect);
        rectF.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));
        Pen borderPen(m_theme.GetColor(Theme::Colors::PanelBorder), m_theme.DpiScaleF(1));
        SolidBrush bgBrush(m_theme.GetColor(Theme::Colors::Panel));

        Gdiplus::RoundRect(Gdiplus::Graphics(hdc), rectF, (float)m_theme.GetSize_Corner(), &bgBrush, borderPen);
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
                    ? Theme::Colors::PanelHover
                    : Theme::Colors::PanelHover);
            SolidBrush hoverBrush(color);
            Pen borserPen(color, 1);
            Gdiplus::RoundRect(graphics, backgroundRect, (float)m_theme.GetSize_Corner(), &hoverBrush, borserPen);
        }
        /*
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
        */
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
        PopupItem *pItem = (PopupItem*)SendMessage(m_hPopupList, LB_GETITEMDATA, itemId, 0);

        SolidBrush textBrush(Color(m_theme.GetColor(Theme::Colors::Text)));
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);

        if (!pItem->icon.empty())
        {
            RectF iconRect = rectF;
            iconRect.Width = m_theme.DpiScaleF(Layout_ImageSize);
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
            DrawPopupItem(hWnd, paint.MemDC(), itemRect, i);
        }
        if (m_theme.IsKeyboardFocused())
        {
            RECT itemRect;
            SendMessage(hWnd, LB_GETITEMRECT, m_selectedIndex, (LPARAM)&itemRect);
            InflateRect(&itemRect, m_theme.DpiScale(-Layout_LeftMargin/4), m_theme.DpiScale(-2));
            m_theme.DrawFocusFrame(paint.MemDC(), itemRect, 0);
        }
    }

    void Popup::OnMouseMove(HWND hWnd, LPARAM lParam)
    {
        int hoverIndex = (int)SendMessage(hWnd, LB_ITEMFROMPOINT, 0, lParam);
        if (hoverIndex >= 0 && hoverIndex < (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0))
        {
            m_hoveredIndex = hoverIndex;
            InvalidateRect(hWnd, NULL, TRUE);
        }
    }

    void Popup::OnLButtonDown(HWND hWnd, LPARAM lParam)
    {
        POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
        int clickIndex = (int)SendMessage(hWnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
        if (clickIndex >= 0 && clickIndex < (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0))
        {
            HandleListClick(clickIndex);
        }
        Hide();
        m_theme.SetKeyboardFocused(NULL);
    }

    void Popup::OnCaptureChanged(HWND hWnd, LPARAM lParam)
    {
        // If we lose capture, hide the popup
        if ((HWND)lParam != hWnd)
        {
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
                    InvalidateRect(m_hPopupList, NULL, TRUE);
                }
                m_theme.SetKeyboardFocused(m_hPopupList);
                return;
            case VK_DOWN:
            case VK_GAMEPAD_DPAD_DOWN:
                if (m_selectedIndex < SendMessage(hWnd, LB_GETCOUNT, 0, 0) - 1 )
                {
                    m_selectedIndex += 1;
                    InvalidateRect(m_hPopupList, NULL, TRUE);
                }
                m_theme.SetKeyboardFocused(m_hPopupList);
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
        }

        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }


    void Popup::HandleListClick(int index)
    {
        if (index >= 0 && index < (int)SendMessage(m_hPopupList, LB_GETCOUNT, 0, 0))
        {
            PopupItem *pItem = (PopupItem*)SendMessage(m_hPopupList, LB_GETITEMDATA, index, 0);
            pItem->callback();
        }
    }
}