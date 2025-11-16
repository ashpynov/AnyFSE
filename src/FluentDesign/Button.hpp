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
        int m_cornerRadius = 8;

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
        void SetIcon(const std::wstring& glyph);
        void Enable(bool bEnable);
        void Show(bool bShow);

        void SetFlat(bool isFlat);
        void SetSquare(bool isSquare);

        void SetColors(Theme::Colors textNornal, Theme::Colors backgroundNormal = Theme::Colors::Default,
                       Theme::Colors textHover = Theme::Colors::Default, Theme::Colors backgroundHover = Theme::Colors::Default,
                       Theme::Colors textPressed = Theme::Colors::Default, Theme::Colors backgroundPressed = Theme::Colors::Default);

        ~Button();

        Event OnChanged;
    };
}