#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <Uxtheme.h>
#include "Tools/Tools.hpp"
#include "ComboBox.hpp"
#include "DoubleBufferedPaint.hpp"

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
        hCombo = CreateWindow(
            L"BUTTON",
            L"",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, width, m_theme.DpiScale(itemHeight),
            hParent, NULL, GetModuleHandle(NULL), NULL);

        m_hImageList = ImageList_Create(
            m_theme.DpiScale(imageSize), m_theme.DpiScale(imageSize),
            ILC_COLOR32 | ILC_MASK, 3, 1);

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

        return pos == -1 ? (int)comboItems.size() : pos;
    }

    int ComboBox::Reset()
    {
        comboItems.clear();
        ImageList_RemoveAll(m_hImageList);
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

            HBRUSH hBackgroundBrush = CreateSolidBrush(m_theme.GetColorRef(Theme::Colors::Panel));  // TODO use helper
            FillRect(hdc, &rect, hBackgroundBrush);
            DeleteObject(hBackgroundBrush);

            // Draw button background
            COLORREF color =
                  !enabled          ? m_theme.GetColorRef(Theme::Colors::ComboDisabled)                       // TODO Disabled Color
                : buttonPressed     ? m_theme.GetColorRef(Theme::Colors::ComboPressed)
                : buttonMouseOver   ? m_theme.GetColorRef(Theme::Colors::ComboHover)
                : m_theme.GetColorRef(Theme::Colors::Combo);

            HPEN hBorderPen = CreatePen(PS_SOLID,  1,  m_theme.GetColorRef(Theme::Colors::Combo));
            HBRUSH hBrush = CreateSolidBrush(color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, m_theme.DpiScale(cornerRadius), m_theme.DpiScale(cornerRadius));

            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);

            DeleteObject(hBorderPen);
            DeleteObject(hBrush);

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

        int imageY = rect.top + (rect.bottom - rect.top - m_theme.DpiScale(imageSize)) / 2;

        ImageList_Draw(m_hImageList, item.iconIndex, hdc, rect.left, imageY, ILD_NORMAL);

        rect.left += m_theme.DpiScale(imageSize + iconMargin);
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
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            L"LISTBOX",
            L"",
            WS_POPUP | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | CS_DROPSHADOW,
            buttonRect.left,
            buttonRect.bottom + 2,
            popupWidth,
            popupHeight,
            GetParent(hCombo),
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
        }

        // Subclass the popup listbox
        SetWindowSubclass(m_hPopupList, PopupListSubclassProc, 0, (DWORD_PTR)this);

        // Set item height
        SendMessage(m_hPopupList, LB_SETITEMHEIGHT, 0, (LPARAM)m_theme.DpiScale(itemHeight)); // Match your DrawComboItem height

        ShowWindow(m_hPopupList, SW_SHOWNOACTIVATE);
        m_popupVisible = true;

        // Capture mouse to close when clicking outside
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
                This->HidePopup();
            }
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
            if (wParam == VK_ESCAPE)
            {
                This->HidePopup();
                return 0;
            }
            else if (wParam == VK_RETURN)
            {
                int selIndex = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
                if (selIndex >= 0)
                {
                    int itemIndex = (int)SendMessage(hWnd, LB_GETITEMDATA, selIndex, 0);
                    This->HandleListClick(itemIndex);
                    This->HidePopup();
                }
                return 0;
            }
            break;

        case WM_DESTROY:
            RemoveWindowSubclass(hWnd, PopupListSubclassProc, uIdSubclass);
            break;
        }

        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void ComboBox::DrawPopupBackground(HWND hWnd, HDC hdcMem, RECT rect)
    {
        // Draw popup background with rounded corners
        HBRUSH hBgBrush = CreateSolidBrush( m_theme.GetColorRef(Theme::Colors::ComboPressed));
        HRGN hRgn = CreateRoundRectRgn(rect.left, rect.top, rect.right, rect.bottom, 8, 8);
        SelectClipRgn(hdcMem, hRgn);

        FillRect(hdcMem, &rect, hBgBrush);

        DeleteObject(hBgBrush);
        DeleteObject(hRgn);
    }

    void ComboBox::DrawPopupItem(HWND hWnd, HDC hdcMem, RECT itemRect, int itemId)
    {
        RECT backgroundRect = itemRect;
        InflateRect(&backgroundRect, m_theme.DpiScale(-4), m_theme.DpiScale(-2));
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
}