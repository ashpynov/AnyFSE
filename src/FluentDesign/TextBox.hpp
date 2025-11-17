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
#include <windowsx.h>
#include <commctrl.h>
#include <string>
#include "Theme.hpp"
#include "Tools/Event.hpp"

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

        void Show(bool bShow);

        Event OnChanged;


    private:
        void OnSize();
        void UpdateTextBoxColors();
        void Invalidate();
        void DrawContainer(HDC hdc);
        static LRESULT CALLBACK ContainerSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        static LRESULT CALLBACK TextBoxSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };

} // namespace FluentDesign