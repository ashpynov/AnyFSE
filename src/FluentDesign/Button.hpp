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
#include "FluentControl.hpp"

namespace FluentDesign
{
    class Button : public FluentControl
    {

    private:
        int m_designWidth = 0;
        int m_designHeight = 0;

        int m_iconAngle;
        int m_startAngle;
        int m_endAngle;
        int m_stepAngle;
        UINT_PTR m_animationTimer;
        bool m_animationLoop;

        bool m_buttonPressed;
        bool m_buttonMouseOver;
        bool m_isIconButton;
        bool m_bFlat;
        bool m_bSquare;
        bool m_isSmallIcon;
        int m_cornerRadius = 8;

        std::vector<Popup::PopupItem> m_menuItems;
        int m_menuWidth = 300;
        int m_menuAlingment = TPM_RIGHTALIGN;

        Theme::Colors m_textNormalColor;
        Theme::Colors m_backgroundNormalColor;
        Theme::Colors m_textHoverColor;
        Theme::Colors m_backgroundHoverColor;
        Theme::Colors m_textPressedColor;
        Theme::Colors m_backgroundPressedColor;

        std::wstring m_text;

        LRESULT OnTimer(UINT timerId);

        static LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                                   UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void HandleMouse(HWND hWnd, UINT uMsg, LPARAM lParam);
        void DrawButton(HWND hWnd, HDC hdc, RECT rc);

        void UpdateLayout();

    public:
        Button(Theme &theme, Align::Anchor align = Align::None, GetParentRectFunc getParentRect = GetParentRect);
        Button(
            Theme& theme,
            HWND hParent,
            int x, int y,
            int width, int height);

        Button& SetAnchor(Align::Anchor anchor, GetParentRectFunc getParentRect = GetParentRect)
        {
            FluentControl::SetAnchor(anchor, getParentRect);
            return *this;
        }

        Button& Create(HWND hParent, int x, int y, int width, int height);
        Button& Create(HWND hParent, const std::wstring& text, const std::function<void()>& callback, int x, int y, int width, int height);

        Button& SetText(const std::wstring& text);
        Button &SetIcon(const std::wstring &glyph, bool bSmall = false);
        Button& Enable(bool bEnable);
        Button& SetTabStop(bool bTabStop);
        Button& Show(bool bShow);

        Button& SetFlat(bool isFlat);
        Button& SetSquare(bool isSquare);
        Button& SetAlign(int bsStyle);
        void SetAngle(int angle);
        Button& SetLinkStyle();

        void SetSize(SIZE sz) { SetSize(sz.cx, sz.cy); };
        void SetSize(LONG cx, LONG cy);

        Button&  SetColors(Theme::Colors textNornal, Theme::Colors backgroundNormal = Theme::Colors::Default,
                       Theme::Colors textHover = Theme::Colors::Default, Theme::Colors backgroundHover = Theme::Colors::Default,
                       Theme::Colors textPressed = Theme::Colors::Default, Theme::Colors backgroundPressed = Theme::Colors::Default);

        void SetMenu(const std::vector<Popup::PopupItem> &items, int nMenuWidth = 300, int nAlignment = TPM_RIGHTALIGN);
        void ShowMenu();
        SIZE GetMinSize();
        ~Button();

        void Animate(int startAngle, int stopAngle, int duration, bool bInfinite);
        void CompleteAnimation();
        void CancelAnimation(int endAngle = 0);

        std::wstring& GetText();

        Event OnChanged;
        Event OnButtonDown;
    };
}