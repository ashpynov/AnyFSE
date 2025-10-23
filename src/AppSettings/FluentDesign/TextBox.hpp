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
#include <windowsx.h>
#include <commctrl.h>
#include <string>
#include "Theme.hpp"

namespace FluentDesign
{

    class TextBox
    {
    private:
        HWND m_hContainer = nullptr;
        HWND m_hTextBox = nullptr;
        HWND m_hParent = nullptr;

        int m_borderWidth = 1;

        int m_cornerRadius = 8;
        int m_marginLeft = 8;
        int m_marginRight = 8;
        int m_marginTop = 8;
        int m_marginBottom = 8;


        // State
        bool m_hasFocus = false;
        bool m_mouseOver = false;

        int m_designedWidth;

        // Text
        std::wstring m_placeholder;
        bool m_showPlaceholder = false;

        Theme &m_theme;

    public:
        TextBox(Theme &theme);
        TextBox(
            Theme& theme,
            HWND hParent,
            int x, int y,
            int width, int height);

        ~TextBox();

        HWND Create(HWND hParent, int x, int y, int width, int height);
        void Destroy();

        // Getters
        HWND GetHandle() const { return m_hContainer; }
        HWND GetTextBoxHandle() const { return m_hTextBox; }
        std::wstring GetText() const;

        // Setters
        void SetText(const std::wstring &text);
        void UpdateLayout();
        void SetPlaceholder(const std::wstring &placeholder);
        // Message handling
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        void OnSize();
        void UpdateTextBoxColors();
        void Invalidate();
        void DrawContainer(HDC hdc);
        static LRESULT CALLBACK ContainerSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        static LRESULT CALLBACK TextBoxSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };

} // namespace FluentDesign