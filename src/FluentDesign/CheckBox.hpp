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

#include "Button.hpp"

namespace FluentDesign
{
    class CheckBox : public Button
    {
    private:
        static constexpr int Layout_BoxSize = 20;
        static constexpr int Layout_TextSpacing = 8;
        static constexpr int Layout_Padding = 2;
        static constexpr int Layout_CornerRadius = 4;

        bool m_isChecked = false;

    protected:
        void HandleClick() override;
        void DrawButton(HWND hWnd, HDC hdc, RECT rect) override;

    public:
        CheckBox(Theme& theme, Align::Anchor align = Align::None, GetParentRectFunc getParentRect = GetParentRect);
        CheckBox(Theme& theme, HWND hParent, int x, int y, int width, int height);

        CheckBox& Create(HWND hParent, int x, int y, int width, int height) override;
        CheckBox& Create(HWND hParent, const std::wstring& text, const std::function<void()>& callback,
                         int x, int y, int width, int height);

        // Like Toggle, programmatic changes notify OnChanged only when the value changes.
        CheckBox& SetCheck(bool isChecked);
        bool GetCheck() const;
        CheckBox& SetText(const std::wstring& text) override;
        SIZE GetMinSize() override;

        CheckBox& SetAnchor(Align::Anchor anchor, GetParentRectFunc getParentRect = GetParentRect)
        {
            Button::SetAnchor(anchor, getParentRect);
            return *this;
        }

        CheckBox& Enable(bool enable) { Button::Enable(enable); return *this; }
        CheckBox& Show(bool show) { Button::Show(show); return *this; }
        CheckBox& SetTabStop(bool tabStop) { Button::SetTabStop(tabStop); return *this; }
    };
}
