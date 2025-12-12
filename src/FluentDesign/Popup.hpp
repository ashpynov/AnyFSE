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


#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <functional>
#include "Tools/Event.hpp"
#include "Theme.hpp"

namespace FluentDesign
{
    class Popup
    {
    public:
        struct PopupItem
        {
            std::wstring name;
            std::wstring icon;
            std::function<void()> callback;

            PopupItem(const std::wstring& itemGlyph, const std::wstring& itemName,  std::function<void()> itemCallback)
                : name(itemName)
                , icon(itemGlyph)
                , callback(itemCallback)
            {}
        };
    private:
        static const int Layout_ItemHeight = 38;
        static const int Layout_LeftMargin = 16;
        static const int Layout_ImageSize = 18;
        static const int Layout_IconMargin = 8;
        static const int Layout_CornerRadius = 8;

        Theme &m_theme;

        HWND m_hPopupList;
        bool m_popupVisible;
        int m_selectedIndex;
        int m_hoveredIndex;
        std::vector<PopupItem> m_popupItems;

    public:
        Popup(Theme &theme)
            : m_theme(theme)
            , m_selectedIndex(-1)
            , m_hoveredIndex(-1)
            , m_popupVisible(false)
            , m_hPopupList(nullptr)
        {
        }

        HWND GetHwnd() { return m_popupVisible ? m_hPopupList : nullptr; }

        void Show(HWND hParent, int x, int y, const std::vector<PopupItem>& items, int width, int flags = TPM_RIGHTALIGN);
        void Hide();

        void OnPaint(HWND hWnd);
        void OnMouseMove(HWND hWnd, LPARAM lParam);
        void OnLButtonDown(HWND hWnd, LPARAM lParam);
        void OnCaptureChanged(HWND hWnd, LPARAM lParam);
        void OnKeyDown(HWND hWnd, WPARAM wParam);

        static LRESULT PopupSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void DrawPopupBackground(HWND hWnd, HDC hdc, RECT rect);
        void DrawPopupItemBackground(HWND hWnd, HDC hdc, RECT itemRect, int itemId);
        void DrawPopupItem(HWND hWnd, HDC hdc, RECT itemRect, int itemId);

        void HandleListClick(int index);
    };
}