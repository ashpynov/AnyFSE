// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <functional>
#include "Tools/Event.hpp"
#include "Theme.hpp"

namespace FluentDesign
{
    class ComboBox
    {
        struct ComboItem
        {
            std::wstring name;
            std::wstring icon;
            std::wstring value;
            int iconIndex;
        };

    private:
        static const int Layout_ItemHeight = 38;
        static const int Layout_ImageSize = 20;
        static const int Layout_LeftMargin = 16;
        static const int Layout_IconMargin = 8;
        static const int Layout_ChevronMargin = 16;
        static const int Layout_CornerRadius = 8;

        HWND hCombo;
        HIMAGELIST m_hImageList = NULL;

        std::vector<ComboItem> m_comboItems;

        bool m_buttonPressed;
        bool m_buttonMouseOver;

        int m_designWidth;

        // Listbox part
        HWND m_hPopupList;
        bool m_popupVisible;
        int m_selectedIndex;
        int m_originalIndex;
        int m_hoveredIndex;

        static LRESULT CALLBACK ComboBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        static LRESULT CALLBACK PopupListSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void HandleMouse(HWND hWnd, UINT uMsg);
        void DrawComboBackground(HWND hWnd, HDC hdc, RECT rc);
        void DrawComboItem(HWND hWnd, HDC hdc, RECT rect, int itemId);
        void DrawComboChevron(HWND hWnd, HDC hdc, RECT rect);
        void DrawPopupBackground(HWND hWnd, HDC hdc, RECT rect);
        void DrawPopupItem(HWND hWnd, HDC hdc, RECT rect, int itemId);
        void HandleListClick(int index);

        void UpdateLayout();

        Theme &m_theme;

    public:
        ComboBox(Theme& theme);
        ComboBox(
            Theme& theme,
            HWND hParent,
            int x, int y,
            int width, int height);

        HWND Create(HWND hParent, int x, int y, int width, int height);

        ~ComboBox();

        int AddItem(const std::wstring &name, const std::wstring &icon, const std::wstring &value, int pos = -1);
        int Reset();
        void SelectItem(int index);
        std::wstring GetCurentValue();
        int GetSelectedIndex() const { return m_selectedIndex; };
        Event OnChanged;

        void ShowPopup();
        void HidePopup();
    };
}