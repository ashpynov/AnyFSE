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
#include "FluentControl.hpp"

namespace FluentDesign
{
    class Tooltip : public FluentControl
    {
        static constexpr int MaxTextWidth = 480;
        static constexpr int Padding = 16;
        static constexpr int MonitorMargin = 8;
        static constexpr int IconWidth = 20;
        static constexpr int IconSpacing = 6;
        static constexpr wchar_t FirstIcon = L'\xE000';
        static constexpr wchar_t LastIcon = L'\xF8FF';

        std::wstring m_text;
        HWND m_previousFocus = nullptr;
        POINT m_mousePosition {};

        void OnPaint(HDC targetDC = nullptr);
        SIZE LayoutText(HDC hdc, const std::wstring& text, const RECT& bounds, bool draw);
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);

    public:
        explicit Tooltip(Theme& theme) : FluentControl(theme) {}
        ~Tooltip() override;
        Tooltip(const Tooltip&) = delete;
        Tooltip& operator=(const Tooltip&) = delete;

        // Coordinates are in screen pixels; the popup prefers alignment to the right edge of x.
        // A leading private-use glyph on a line is drawn with Segoe Fluent Icons.
        void Show(HWND hParent, const std::wstring& text, int x, int y);
        void Hide(bool restoreFocus = true);
    };
}
