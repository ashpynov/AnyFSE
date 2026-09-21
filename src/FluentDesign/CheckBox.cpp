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




#include "CheckBox.hpp"
#include <cmath>

namespace FluentDesign
{
    CheckBox::CheckBox(Theme& theme, Align::Anchor align, GetParentRectFunc getParentRect)
        : Button(theme, align, getParentRect)
    {
    }

    CheckBox::CheckBox(Theme& theme, HWND hParent, int x, int y, int width, int height)
        : CheckBox(theme)
    {
        Create(hParent, x, y, width, height);
    }

    CheckBox& CheckBox::Create(HWND hParent, int x, int y, int width, int height)
    {
        Button::Create(hParent, x, y, width, height);
        if (m_hWnd)
        {
            // Manual checkbox: Button routes mouse and keyboard activation through HandleClick.
            LONG_PTR style = GetWindowLongPtr(m_hWnd, GWL_STYLE);
            SetWindowLongPtr(m_hWnd, GWL_STYLE, (style & ~(BS_TYPEMASK | BS_CENTER)) | BS_CHECKBOX | BS_LEFT);
            SendMessage(m_hWnd, BM_SETCHECK, m_isChecked ? BST_CHECKED : BST_UNCHECKED, 0);
            SetWindowTextW(m_hWnd, GetText().c_str());
        }
        return *this;
    }

    CheckBox& CheckBox::Create(HWND hParent, const std::wstring& text, const std::function<void()>& callback,
                               int x, int y, int width, int height)
    {
        Create(hParent, x, y, width, height);
        SetText(text);
        OnChanged += callback;
        return *this;
    }

    bool CheckBox::GetCheck() const
    {
        return m_hWnd ? SendMessage(m_hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED : m_isChecked;
    }

    CheckBox& CheckBox::SetCheck(bool isChecked)
    {
        const bool changed = GetCheck() != isChecked;
        m_isChecked = isChecked;
        if (m_hWnd)
        {
            SendMessage(m_hWnd, BM_SETCHECK, isChecked ? BST_CHECKED : BST_UNCHECKED, 0);
            if (changed)
                InvalidateRect(m_hWnd, nullptr, FALSE);
        }
        if (changed)
            OnChanged.Notify();
        return *this;
    }

    CheckBox& CheckBox::SetText(const std::wstring& text)
    {
        // Retain Button's text/layout API and expose the label to the native control.
        if (m_hWnd)
        {
            Button::SetText(text);
            SetWindowTextW(m_hWnd, text.c_str());
        }
        else
        {
            GetText() = text;
        }
        return *this;
    }

    void CheckBox::HandleClick()
    {
        if (!m_hWnd || !IsWindowEnabled(m_hWnd))
            return;

        SetCheck(!GetCheck());
        OnButtonDown.Notify();
    }

    SIZE CheckBox::GetMinSize()
    {
        HDC hdc = GetDC(m_hWnd);
        Gdiplus::RectF bounds;
        {
            Gdiplus::Graphics graphics(hdc);
            Gdiplus::Font font(hdc, m_theme.GetFont_Text());
            Gdiplus::StringFormat format;
            format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
            if (!GetText().empty())
                graphics.MeasureString(GetText().c_str(), -1, &font, Gdiplus::PointF(0, 0), &format, &bounds);
        }
        ReleaseDC(m_hWnd, hdc);

        const float boxSize = m_theme.DpiScaleF(Layout_BoxSize);
        const float padding = m_theme.DpiScaleF(Layout_Padding * 2);
        const float spacing = GetText().empty() ? 0.0f : m_theme.DpiScaleF(Layout_TextSpacing);
        return SIZE{ static_cast<LONG>(std::ceil(boxSize + spacing + bounds.Width + padding)),
                     static_cast<LONG>(std::ceil(max(boxSize, bounds.Height) + padding)) };
    }

    void CheckBox::DrawButton(HWND hWnd, HDC hdc, RECT rect)
    {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        const bool enabled = IsWindowEnabled(hWnd);
        const bool checked = GetCheck();
        const bool pressed = enabled && m_mousePressed;
        const bool hovered = enabled && IsMouseOver();
        const float padding = m_theme.DpiScaleF(Layout_Padding);
        const float boxSize = m_theme.DpiScaleF(Layout_BoxSize);
        Gdiplus::RectF box(static_cast<float>(rect.left) + padding,
                          (rect.top + rect.bottom - boxSize) / 2.0f, boxSize, boxSize);

        Theme::Colors border = checked
            ? (enabled ? Theme::Colors::ToggleBorderOn : Theme::Colors::ToggleBorderOnDisabled)
            : (enabled ? Theme::Colors::ToggleBorderOff : Theme::Colors::ToggleBorderOffDisabled);
        Theme::Colors background = checked
            ? (!enabled ? Theme::Colors::ToggleTrackOnDisabled
                        : pressed || hovered ? Theme::Colors::ToggleTrackOnPressed : Theme::Colors::ToggleTrackOn)
            : (!enabled ? Theme::Colors::ToggleTrackOffDisabled
                        : pressed ? Theme::Colors::ButtonPressed
                        : hovered ? Theme::Colors::ToggleTrackOffHover : Theme::Colors::ToggleTrackOff);

        Gdiplus::Pen borderPen(m_theme.GetColor(border), m_theme.DpiScaleF(1));
        Gdiplus::SolidBrush backgroundBrush(m_theme.GetColor(background));
        Gdiplus::RoundRect(graphics, box, m_theme.DpiScaleF(Layout_CornerRadius), &backgroundBrush, borderPen);

        if (checked)
        {
            Gdiplus::Pen checkPen(m_theme.GetColor(enabled ? Theme::Colors::ToggleThumbOn : Theme::Colors::ToggleThumbOnDisabled),
                                  m_theme.DpiScaleF(2));
            checkPen.SetStartCap(Gdiplus::LineCapRound);
            checkPen.SetEndCap(Gdiplus::LineCapRound);
            checkPen.SetLineJoin(Gdiplus::LineJoinRound);
            const Gdiplus::PointF points[] = {
                { box.X + boxSize * 0.25f, box.Y + boxSize * 0.50f },
                { box.X + boxSize * 0.43f, box.Y + boxSize * 0.68f },
                { box.X + boxSize * 0.76f, box.Y + boxSize * 0.33f }
            };
            graphics.DrawLines(&checkPen, points, 3);
        }

        if (!GetText().empty())
        {
            const float textLeft = box.GetRight() + m_theme.DpiScaleF(Layout_TextSpacing);
            Gdiplus::RectF textRect(textLeft, static_cast<float>(rect.top),
                                   max(0.0f, rect.right - padding - textLeft), static_cast<float>(rect.bottom - rect.top));
            Gdiplus::Font font(hdc, m_theme.GetFont_Text());
            Gdiplus::SolidBrush textBrush(m_theme.GetColor(enabled ? Theme::Colors::Text : Theme::Colors::TextDisabled));
            Gdiplus::StringFormat format;
            format.SetAlignment(Gdiplus::StringAlignmentNear);
            format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
            format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
            if (textRect.Width > 0)
                graphics.DrawString(GetText().c_str(), -1, &font, textRect, &format, &textBrush);
        }
    }
}
