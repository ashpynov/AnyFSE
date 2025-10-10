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
        static const int itemHeight = 38;
        static const int imageSize = 20;
        static const int leftMargin = 12;
        static const int iconMargin = 8;
        static const int chevronMargin = 16;
        static const int cornerRadius = 8;

        HWND hCombo;
        HIMAGELIST m_hImageList = NULL;

        std::vector<ComboItem> comboItems;

        bool buttonPressed;
        bool buttonMouseOver;

        int m_designWidth;

        // Listbox part
        HWND m_hPopupList;
        bool m_popupVisible;
        int m_selectedIndex;
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

        FluentDesign::Theme &m_theme;

    public:
        ComboBox(FluentDesign::Theme& theme);
        ComboBox(
            FluentDesign::Theme& theme,
            HWND hParent,
            int x, int y,
            int width, int height);

        HWND Create(HWND hParent, int x, int y, int width, int height);

        ~ComboBox();

        int AddItem(const std::wstring &name, const std::wstring &icon, const std::wstring &value, int pos = -1);
        int Reset();
        void SelectItem(int index);
        std::wstring GetCurentValue();
        Event OnChanged;

        void ShowPopup();
        void HidePopup();
    };
}