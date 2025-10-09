#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <Uxtheme.h>
#include "SettingsLine.hpp"
#include "SettingsComboBox.hpp"
#include "Tools/Tools.hpp"

namespace AnyFSE::Settings::Controls
{
    SettingsComboBox::SettingsComboBox(
        HWND hParent,
        const std::wstring &name,
        const std::wstring &description,
        int x, int y,
        int width, int height)
            : SettingsLine(hParent, name, description, x,y, width, height)
    {
        Create();
        g_hImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 3, 1);
        SendMessage(hCombo, CBEM_SETIMAGELIST, 0, (LPARAM)g_hImageList);

    }

    SettingsComboBox::~SettingsComboBox()
    {
        SendMessage(hCombo, CBEM_SETIMAGELIST, 0, 0);
        ImageList_RemoveAll(g_hImageList);
        ImageList_Destroy(g_hImageList);
    }

    int SettingsComboBox::AddItem(const std::wstring &name, const std::wstring &icon, const std::wstring &value, int pos)
    {
        comboItems.push_back(ComboItem {name, icon, value, -1});
        ComboItem &cb = comboItems.back();

        HICON hIcon = Tools::LoadIconW(cb.icon);
        cb.iconIndex = hIcon ? ImageList_AddIcon(g_hImageList, hIcon) : -1;
        comboItems.push_back(cb);

        COMBOBOXEXITEM cbei = {0};
        cbei.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_LPARAM;
        cbei.iItem = pos;
        cbei.pszText = (LPWSTR)name.c_str();
        cbei.iImage = cb.iconIndex;
        cbei.iSelectedImage = cb.iconIndex;
        cbei.lParam = (LPARAM)&cb;
        return (int)SendMessage(hCombo, CBEM_INSERTITEM, 0, (LPARAM)&cbei);
    }

    int SettingsComboBox::Reset()
    {
        SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
        comboItems.clear();
        ImageList_RemoveAll(g_hImageList);
        return 0;
    }

    void SettingsComboBox::SelectItem(int index)
    {

        SendMessage(hCombo, CB_SETCURSEL, 0, (LPARAM) index);
    }

    std::wstring SettingsComboBox::GetCurentValue()
    {
        int index = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
        if (index >= 0)
        {
            ComboItem * item = (ComboItem*)SendMessage(hCombo, CB_GETITEMDATA, (WPARAM)index, (LPARAM)0);
            if (item)
            {
                return item->value;
            }
        }
        return L"";
    }

    HWND SettingsComboBox::CreateControl(HWND hWnd)
    {
        hCombo = CreateWindow(L"ComboBoxEx32", NULL,
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL,
            0, 0, 250, 200, hWnd, NULL, GetModuleHandle(NULL), NULL);

        SetWindowSubclass(hCombo, ComboBoxSubclassProc, 0, (DWORD_PTR)this);
        return hCombo;
    }

    bool SettingsComboBox::ApplyTheme(bool isDark)
    {
        LPCWSTR theme = isDark ? L"DarkMode_CFD" : L"Explorer";
        SetWindowTheme(hCombo, theme, NULL);
        SetWindowTheme(GetWindow(hCombo, GW_CHILD), theme, NULL);
        SetWindowTheme(GetWindow(hCombo, GW_ENABLEDPOPUP), theme, NULL);
        return false;
    }

    LRESULT SettingsComboBox::ComboBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {

        SettingsComboBox * This = reinterpret_cast<SettingsComboBox*>(dwRefData);

        switch (uMsg)
        {
            case WM_MOUSEMOVE:
                // Handle mouse movement
                break;

            case WM_SIZE:
                This->PositionChildControl();
                break;

            // case WM_MEASUREITEM:
            //     return This->OnMeasureItem((MEASUREITEMSTRUCT*)lParam);

            // case WM_DRAWITEM:
            //     return This->OnDrawItem((DRAWITEMSTRUCT*)lParam);

            case WM_COMMAND:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    This->OnChanged.Notify();
                }
                break;
            case WM_NCDESTROY:
                RemoveWindowSubclass(hWnd, ComboBoxSubclassProc, uIdSubclass);
                break;
        }

        // Call original window procedure for default handling
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
        }

        BOOL SettingsComboBox::OnMeasureItem(MEASUREITEMSTRUCT *mis)
        {
            if (mis->CtlType == ODT_COMBOBOX)
            {
                // Set item height
                mis->itemHeight = 48; // Custom height for image + text
                return TRUE;
            }
            return FALSE;
        }

        BOOL SettingsComboBox::OnDrawItem(DRAWITEMSTRUCT *dis)
        {
            if (dis->CtlType == ODT_COMBOBOX)
            {
                //if (dis->itemID == -1)
                {
                    // Draw the main ComboBox control (when closed)
                    DrawComboBox(dis);
                }
                //else
                {
                    // Draw individual items in the dropdown
                    //DrawComboBoxItem(dis);
                }
                return TRUE;
            }
            return FALSE;
        }

        void SettingsComboBox::DrawComboBox(DRAWITEMSTRUCT *dis)
        {
            HDC hdc = dis->hDC;
            RECT rect = dis->rcItem;

            bool disabled = (dis->itemState & ODS_DISABLED) != 0;
            bool hasFocus = (dis->itemState & ODS_FOCUS) != 0;

            ComboItem *item = (ComboItem *)SendMessage(hCombo, CB_GETITEMDATA, (WPARAM)dis->itemID, (LPARAM)0);

            // Draw background
            HBRUSH hBackground = this->m_hBackgroundBrush;
            FillRect(hdc, &rect, hBackground);

            // Draw border
            if (hasFocus && !disabled)
            {
                HBRUSH hBorder = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                FrameRect(hdc, &rect, hBorder);
                DeleteObject(hBorder);
            }

            // Draw image if available
            if (g_hImageList && item->iconIndex >= 0)
            {
                int imageSize = 32;
                int imageY = rect.top + (rect.bottom - rect.top - imageSize) / 2;

                ImageList_Draw(g_hImageList, item->iconIndex, hdc,
                               rect.left + 4, imageY,
                               ILD_NORMAL);
            }

            // Draw text
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, m_nameColor);

            HFONT hOldFont = (HFONT)SelectObject(hdc, m_hNameFont);

            RECT textRect = rect;
            textRect.left += (item->iconIndex >= 0 ? 40 : 8);

            ::DrawText(hdc, item->name.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);
        }


}