#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <Uxtheme.h>
#include "Tools/Tools.hpp"
#include "ComboBox.hpp"
#include "DoubleBufferedPaint.hpp"
#include "GdiPlus.hpp"

namespace FluentDesign
{
    ComboBox::ComboBox(FluentDesign::Theme &theme)
        : m_theme(theme)
        , buttonMouseOver(false)
        , buttonPressed(false)
        , m_popupVisible(false)
        , m_selectedIndex(-1)
        , m_hoveredIndex(-1)
        , m_hImageList(NULL)
    {
        theme.OnDPIChanged += [This = this]() { This->UpdateLayout(); };
    }

    ComboBox::ComboBox(
        FluentDesign::Theme& theme,
        HWND hParent,
        int x, int y,
        int width, int height)
            : ComboBox(theme)
    {
        Create(hParent, x, y, width, height);
    }

    HWND ComboBox::Create(
        HWND hParent,
        int x, int y,
        int width, int height
    )
    {
        m_designWidth = m_theme.DpiUnscale(width);

        hCombo = CreateWindow(
            L"BUTTON",
            L"",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, width, m_theme.DpiScale(itemHeight),
            hParent, NULL, GetModuleHandle(NULL), NULL);

        m_hImageList = ImageList_Create(
            m_theme.DpiScale(imageSize), m_theme.DpiScale(imageSize),
            ILC_COLOR32 | ILC_MASK, 3, 1);


        m_theme.RegisterChild(hCombo);
        SetWindowSubclass(hCombo, ComboBoxSubclassProc, 0, (DWORD_PTR)this);
        return hCombo;
    }

    ComboBox::~ComboBox()
    {
        ImageList_RemoveAll(m_hImageList);
        ImageList_Destroy(m_hImageList);
    }

    int ComboBox::AddItem(const std::wstring &name, const std::wstring &icon, const std::wstring &value, int pos)
    {
        ComboItem &cb = *(comboItems.insert(pos != -1 ? comboItems.begin() + pos : comboItems.end(), ComboItem{name, icon, value, -1}));

        HICON hIcon = Tools::LoadIconW(cb.icon);
        cb.iconIndex = hIcon ? ImageList_AddIcon(m_hImageList, hIcon) : -1;

        InvalidateRect(hCombo, NULL, FALSE);
        return pos == -1 ? (int)comboItems.size() : pos;
    }

    int ComboBox::Reset()
    {
        comboItems.clear();
        ImageList_RemoveAll(m_hImageList);
        InvalidateRect(hCombo, NULL, FALSE);
        return 0;
    }

    void ComboBox::SelectItem(int index)
    {
        if (index >=0 && index < comboItems.size())
        {
            m_selectedIndex = index;
        }
    }

    std::wstring ComboBox::GetCurentValue()
    {
        return comboItems[m_selectedIndex].value;
    }

    LRESULT ComboBox::ComboBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {

        ComboBox *This = reinterpret_cast<ComboBox *>(dwRefData);
        switch (uMsg)
        {
            case WM_MOUSEMOVE:
            case WM_MOUSELEAVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
                This->HandleMouse(hWnd, uMsg);
                break;

            case WM_GETDLGCODE:
                // if (This->m_theme.IsKeyboardFocused() && wParam == VK_UP || wParam == VK_DOWN)
                // {
                //     return DLGC_WANTARROWS;
                // }
                break;

            case WM_KEYDOWN:
                if (This->m_theme.IsKeyboardFocused())
                {
                    if (wParam == VK_SPACE || wParam == VK_GAMEPAD_A)
                    {
                        This->buttonPressed = true;
                        InvalidateRect(hWnd, NULL, TRUE);
                        This->ShowPopup();
                        return 0;
                    }
                    // else if (wParam == VK_UP || wParam == VK_GAMEPAD_DPAD_UP)
                    // {
                    //     if (This->m_selectedIndex >0 )
                    //     {
                    //         This->m_selectedIndex -= 1;
                    //         InvalidateRect(This->hCombo, NULL, TRUE);
                    //         This->m_theme.SetKeyboardFocused(This->hCombo);
                    //         This->OnChanged.Notify();
                    //         return 0;
                    //     }
                    // }
                    // else if (wParam == VK_DOWN || wParam == VK_GAMEPAD_DPAD_DOWN)
                    // {
                    //     if (This->m_selectedIndex < This->comboItems.size() - 1 )
                    //     {
                    //         This->m_selectedIndex += 1;
                    //         InvalidateRect(This->hCombo, NULL, TRUE);
                    //         This->m_theme.SetKeyboardFocused(This->hCombo);
                    //         This->OnChanged.Notify();
                    //         return 0;
                    //     }
                    // }
                }
                break;
            case WM_SETFOCUS:
                if ( This->m_popupVisible )
                {
                    SetFocus(This->m_hPopupList);
                }
                break;


            case WM_PAINT:
                {
                    FluentDesign::DoubleBuferedPaint paint(hWnd);

                    RECT rect = paint.ClientRect();
                    This->DrawComboBackground(hWnd, paint.MemDC(), rect);
                    rect.right -= This->m_theme.DpiScale(chevronMargin);
                    This->DrawComboItem(hWnd, paint.MemDC(), rect, This->m_selectedIndex);
                    rect.right += This->m_theme.DpiScale(chevronMargin);
                    This->DrawComboChevron(hWnd, paint.MemDC(), rect);
                }
                return 0;

            case WM_ERASEBKGND:
                return 1;
            case WM_NCDESTROY:
                RemoveWindowSubclass(hWnd, ComboBoxSubclassProc, uIdSubclass);
                break;
            }

        // Call original window procedure for default handling
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void ComboBox::HandleMouse(HWND hWnd, UINT uMsg)
    {
        switch (uMsg)
        {
        case WM_MOUSEMOVE:
            if (!buttonMouseOver)
            {
                buttonMouseOver = true;
                InvalidateRect(hWnd, NULL, TRUE);

                // Track mouse leave
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
            }
            break;

        case WM_MOUSELEAVE:
            buttonMouseOver = false;
            buttonPressed = false;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case WM_LBUTTONDOWN:
            buttonPressed = true;
            InvalidateRect(hWnd, NULL, TRUE);
            ShowPopup();
            break;

        case WM_LBUTTONUP:
            buttonPressed = false;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        return;
    }
    void ComboBox::DrawComboBackground(HWND hWnd, HDC hdc, RECT rect)
    {
            // Get button state
            bool enabled = IsWindowEnabled(hWnd);
            bool focused = (GetFocus() == hWnd);

            using namespace Gdiplus;
            Graphics graphics(hdc);

            // Enable anti-aliasing for smooth edges
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);

            // Draw button background
            Color color = m_theme.GetColor(
                  !enabled          ? Theme::Colors::ComboDisabled                       // TODO Disabled Color
                : buttonPressed     ? Theme::Colors::ComboPressed
                : buttonMouseOver   ? Theme::Colors::ComboHover
                : Theme::Colors::Combo
            );

            RectF clientRect = FromRECT(rect);
            clientRect.Height -= 1;
            clientRect.Width -= 1;
            Pen pen(m_theme.GetColor(Theme::Colors::ComboBorder), m_theme.DpiScaleF(1));
            SolidBrush brush(color);
            Gdiplus::RoundRect(graphics, clientRect, (REAL)m_theme.GetSize_FocusCornerSize(), &brush, pen);
            return;
    }
    void ComboBox::DrawComboItem(HWND hWnd, HDC hdc, RECT rect, int itemId)
    {
        if (itemId < 0)
        {
            return;
        }
        bool enabled = IsWindowEnabled(hWnd);
        ComboItem &item = comboItems[itemId];
        // Draw text
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, enabled
            ? m_theme.GetColorRef(Theme::Colors::Text)
            : m_theme.GetColorRef(Theme::Colors::TextSecondary)
        );

        HFONT hOldFont = (HFONT)SelectObject(hdc, m_theme.GetFont_Text());

        rect.left += m_theme.DpiScale(leftMargin);

        if (item.iconIndex != -1)
        {
            int imageY = rect.top + (rect.bottom - rect.top - m_theme.DpiScale(imageSize)) / 2;
            ImageList_Draw(m_hImageList, item.iconIndex, hdc, rect.left, imageY, ILD_NORMAL);
            rect.left += m_theme.DpiScale(imageSize + iconMargin);
        }

        ::DrawText(hdc, item.name.c_str(), -1, &rect,
                   DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, hOldFont);
        return;
    }
    void ComboBox::DrawComboChevron(HWND hWnd, HDC hdc, RECT rect)
    {
            rect.right -= m_theme.DpiScale(chevronMargin);

            HFONT hOldFont = (HFONT)SelectObject(hdc, m_theme.GetFont_Glyph());

            SetTextColor(hdc, m_theme.GetColorRef(Theme::Colors::Text));
            ::DrawText(hdc, L"\xE70D", -1, &rect,
                     DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);
    }

    void ComboBox::ShowPopup()
    {
        if (m_popupVisible)
            return;

        // Calculate popup position (below the button)
        RECT buttonRect;
        GetWindowRect(hCombo, &buttonRect);

        int popupWidth = buttonRect.right - buttonRect.left;
        int popupHeight = min(m_theme.DpiScale(400), (int)comboItems.size() * m_theme.DpiScale(itemHeight)); // Calculate height based on items

        // Create popup listbox window
        m_hPopupList = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"LISTBOX",
            L"",
            WS_CHILD | WS_POPUP | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | CS_DROPSHADOW | WS_TABSTOP,
            buttonRect.left,
            buttonRect.bottom + 2,
            popupWidth,
            popupHeight,
            hCombo,//GetParent(hCombo),
            NULL,
            GetModuleHandle(NULL),
            NULL);

        // Set transparency for shadow
        SetLayeredWindowAttributes(m_hPopupList, 0, 255, LWA_COLORKEY);

        // Add items to listbox (we'll use item data to store our ComboItem index)
        for (int i = 0; i < (int)comboItems.size(); i++)
        {
            int lbIndex = (int)SendMessage(m_hPopupList, LB_ADDSTRING, 0, (LPARAM)comboItems[i].name.c_str());
            SendMessage(m_hPopupList, LB_SETITEMDATA, lbIndex, (LPARAM)i); // Store index to our comboItems
            if (lbIndex == m_selectedIndex)
            {
                SendMessage(m_hPopupList, LB_SETCURSEL, m_selectedIndex, 0);  // ListBox
            }
        }
        m_originalIndex = m_selectedIndex;

        // Subclass the popup listbox
        SetWindowSubclass(m_hPopupList, PopupListSubclassProc, 0, (DWORD_PTR)this);

        // Set item height
        SendMessage(m_hPopupList, LB_SETITEMHEIGHT, 0, (LPARAM)m_theme.DpiScale(itemHeight)); // Match your DrawComboItem height

        ShowWindow(m_hPopupList, SW_SHOW);
        m_popupVisible = true;

        // Capture mouse to close when clicking outside
        SetFocus(m_hPopupList);
        SetCapture(m_hPopupList);
    }

    void ComboBox::HidePopup()
    {
        if (m_popupVisible && m_hPopupList)
        {
            ReleaseCapture();
            DestroyWindow(m_hPopupList);
            m_hPopupList = NULL;
            m_popupVisible = false;
            m_hoveredIndex = -1;
            SetFocus(hCombo);
            InvalidateRect(hCombo, NULL, TRUE); // Redraw main button
        }
    }

    // Single popup listbox window procedure
    LRESULT CALLBACK ComboBox::PopupListSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        ComboBox *This = reinterpret_cast<ComboBox *>(dwRefData);

        switch (uMsg)
        {
        case WM_ERASEBKGND:
            return 1; // We handle background

        case WM_PAINT:
        {
            FluentDesign::DoubleBuferedPaint paint(hWnd);

            This->DrawPopupBackground(hWnd, paint.MemDC(), paint.ClientRect());

            for (int i = 0; i < This->comboItems.size(); i++)
            {
                RECT itemRect;
                SendMessage(hWnd, LB_GETITEMRECT, i, (LPARAM)&itemRect);

                This->DrawPopupItem(hWnd, paint.MemDC(), itemRect, i);
            }
            if (This->m_theme.IsKeyboardFocused())
            {
                RECT itemRect;
                SendMessage(hWnd, LB_GETITEMRECT, This->m_selectedIndex, (LPARAM)&itemRect);
                InflateRect(&itemRect, This->m_theme.DpiScale(-leftMargin/4), This->m_theme.DpiScale(-2));
                This->m_theme.DrawFocusFrame(paint.MemDC(), itemRect, 0);
            }
        }
            return 0;

        case WM_NCPAINT:
            // Suppress non-client area painting
            return 0;

        case WM_MOUSEMOVE:
        {
            // Handle item hover
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            int hoverIndex = (int)SendMessage(hWnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
            if (hoverIndex >= 0 && hoverIndex < (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0))
            {
                This->m_hoveredIndex = hoverIndex;
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        break;

        case WM_LBUTTONDOWN:
        {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            int clickIndex = (int)SendMessage(hWnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
            if (clickIndex >= 0 && clickIndex < (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0))
            {
                // Get the actual ComboItem index from item data
                int itemIndex = (int)SendMessage(hWnd, LB_GETITEMDATA, clickIndex, 0);
                This->HandleListClick(itemIndex);
                This->HidePopup();
            }
            else
            {
                // Clicked outside items, hide popup
                This->m_selectedIndex = This->m_originalIndex;
                This->HidePopup();
            }
            This->m_theme.SetKeyboardFocused(NULL);
        }
        return 0;

        case WM_CAPTURECHANGED:
            // If we lose capture, hide the popup
            if ((HWND)lParam != hWnd && (HWND)lParam != This->hCombo )
            {
                This->HidePopup();
            }
            else
            {
                SetCapture(hWnd);
            }
            break;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE || wParam == VK_GAMEPAD_B)
            {
                This->m_selectedIndex = This->m_originalIndex;
                This->HidePopup();
                return 0;
            }
            else if (wParam == VK_RETURN || wParam == VK_SPACE || wParam == VK_GAMEPAD_A)
            {
                This->HandleListClick(This->m_selectedIndex);
                This->HidePopup();
                return 0;
            }
            else if (wParam == VK_UP || wParam == VK_GAMEPAD_DPAD_UP)
            {
                if (This->m_selectedIndex >0 )
                {
                    This->m_selectedIndex -= 1;
                    InvalidateRect(This->m_hPopupList, NULL, TRUE);
                    InvalidateRect(This->hCombo, NULL, TRUE);
                }
            }
            else if (wParam == VK_DOWN || wParam == VK_GAMEPAD_DPAD_DOWN)
            {
                if (This->m_selectedIndex < This->comboItems.size() - 1 )
                {
                    This->m_selectedIndex += 1;
                    InvalidateRect(This->m_hPopupList, NULL, TRUE);
                    InvalidateRect(This->hCombo, NULL, TRUE);
                }
            }
            This->m_theme.SetKeyboardFocused(This->m_hPopupList);
            return 0;

        case WM_DESTROY:
            RemoveWindowSubclass(hWnd, PopupListSubclassProc, uIdSubclass);
            break;
        }

        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void ComboBox::DrawPopupBackground(HWND hWnd, HDC hdcMem, RECT rect)
    {
        // Draw popup background with rounded corners
        HBRUSH hNilBrush = CreateSolidBrush(RGB(0,0,0));
        FillRect(hdcMem, &rect, hNilBrush);

        HPEN hBgPen = CreatePen(PS_SOLID, m_theme.DpiScale(1),  m_theme.GetColorRef(Theme::Colors::ComboPopupBorder));
        HBRUSH hBgBrush = CreateSolidBrush( m_theme.GetColorRef(Theme::Colors::ComboPopup));

        HGDIOBJ hOldBrush = SelectObject(hdcMem, hBgBrush);
        HGDIOBJ hOldPen = SelectObject(hdcMem, hBgPen);

        InflateRect(&rect, -1, -1);
        RoundRect(hdcMem, rect.left, rect.top, rect.right, rect.bottom, m_theme.GetSize_Corner(), m_theme.GetSize_Corner());

        SelectObject(hdcMem, hOldBrush);
        SelectObject(hdcMem, hOldPen);

        DeleteObject(hBgBrush);
        DeleteObject(hNilBrush);
        DeleteObject(hBgPen);

    }

    void ComboBox::DrawPopupItem(HWND hWnd, HDC hdcMem, RECT itemRect, int itemId)
    {
        RECT backgroundRect = itemRect;
        InflateRect(&backgroundRect, m_theme.DpiScale(-leftMargin/4), m_theme.DpiScale(-2));
        if ( itemId == m_hoveredIndex || itemId == m_selectedIndex)
        {
            COLORREF color = (itemId == m_hoveredIndex) ? m_theme.GetColorRef(Theme::Colors::ComboPopupHover)
                                                        : m_theme.GetColorRef(Theme::Colors::ComboPopupSelected);

            HBRUSH hHoverBrush = CreateSolidBrush(color);
            HPEN hBorderPen = CreatePen(PS_SOLID,  1, color);
            HPEN hOldPen = (HPEN)SelectObject(hdcMem, hBorderPen);
            HBRUSH hPrevBrush = (HBRUSH)SelectObject(hdcMem, hHoverBrush);
            RoundRect(hdcMem, backgroundRect.left,
                backgroundRect.top, backgroundRect.right,
                backgroundRect.bottom, m_theme.DpiScale(cornerRadius),
                m_theme.DpiScale(cornerRadius));
            SelectObject(hdcMem, hPrevBrush);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hHoverBrush);
            DeleteObject(hBorderPen);
        }
        if ( itemId == m_selectedIndex)
        {
            HBRUSH hGripBrush = CreateSolidBrush(m_theme.GetColorRef(Theme::Colors::ComboPopupSelectedMark));
            RECT gripRect = backgroundRect;
            gripRect.left += 1;
            gripRect.right = gripRect.left + m_theme.DpiScale(3);
            gripRect.top += (gripRect.bottom - gripRect.top - m_theme.DpiScale(14)) / 2;
            gripRect.bottom = gripRect.top + m_theme.DpiScale(14);
            FillRect(hdcMem, &gripRect, hGripBrush);
            DeleteObject(hGripBrush);
        }
        DrawComboItem(hWnd, hdcMem, itemRect, itemId);
    }

    void ComboBox::HandleListClick(int index)
    {
        if (index >= 0 && index < (int)comboItems.size())
        {
            m_selectedIndex = index;
            // Update your main button display here
            InvalidateRect(hCombo, NULL, TRUE);

            OnChanged.Notify();
        }
    }
    void ComboBox::UpdateLayout()
    {
        SetWindowPos(hCombo, 0, 0, 0,
                     m_theme.DpiScale(m_designWidth),
                     m_theme.DpiScale(itemHeight + 1),
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

        ImageList_RemoveAll(m_hImageList);
        ImageList_Destroy(m_hImageList);

        m_hImageList = ImageList_Create(
            m_theme.DpiScale(imageSize), m_theme.DpiScale(imageSize),
            ILC_COLOR32 | ILC_MASK, 3, 1);

        for(auto& item: comboItems)
        {
            if (item.iconIndex != -1 && !item.icon.empty())
            {
                HICON hIcon = Tools::LoadIconW(item.icon);
                item.iconIndex = hIcon ? ImageList_AddIcon(m_hImageList, hIcon) : -1;
            }
        }
    }
}