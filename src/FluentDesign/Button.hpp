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
#include "Popup.hpp"

namespace FluentDesign
{
    class Button
    {

    private:
        int m_designWidth;
        int m_designHeight;

        HWND m_hButton;

        bool m_buttonPressed;
        bool m_buttonMouseOver;
        bool m_isIconButton;
        bool m_bFlat;
        bool m_bSquare;
        bool m_isSmallIcon;
        int m_cornerRadius = 8;

        std::vector<Popup::PopupItem> m_menuItems;

        Theme::Colors m_textNormalColor;
        Theme::Colors m_backgroundNormalColor;
        Theme::Colors m_textHoverColor;
        Theme::Colors m_backgroundHoverColor;
        Theme::Colors m_textPressedColor;
        Theme::Colors m_backgroundPressedColor;

        std::wstring m_text;

        static LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void HandleMouse(HWND hWnd, UINT uMsg, LPARAM lParam);
        void DrawButton(HWND hWnd, HDC hdc, RECT rc);

        void UpdateLayout();

        Theme &m_theme;

    public:
        Button(Theme& theme);
        Button(
            Theme& theme,
            HWND hParent,
            int x, int y,
            int width, int height);

        HWND Create(HWND hParent, int x, int y, int width, int height);
        HWND Create(HWND hParent, const std::wstring& text, const std::function<void()>& callback, int x, int y, int width, int height);

        HWND GetHwnd() const { return m_hButton; }
        void SetText(const std::wstring& text);
        void SetIcon(const std::wstring& glyph, bool bSmall = false);
        void Enable(bool bEnable);
        void Show(bool bShow);

        void SetFlat(bool isFlat);
        void SetSquare(bool isSquare);

        void SetColors(Theme::Colors textNornal, Theme::Colors backgroundNormal = Theme::Colors::Default,
                       Theme::Colors textHover = Theme::Colors::Default, Theme::Colors backgroundHover = Theme::Colors::Default,
                       Theme::Colors textPressed = Theme::Colors::Default, Theme::Colors backgroundPressed = Theme::Colors::Default);

        void SetMenu(const std::vector<Popup::PopupItem> &items);
        void ShowMenu();

        ~Button();

        Event OnChanged;
        Event OnButtonDown;
    };
}