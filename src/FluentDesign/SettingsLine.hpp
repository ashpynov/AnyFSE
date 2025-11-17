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
#include <string>
#include <functional>
#include "Theme.hpp"
#include "Tools/Event.hpp"
#include "Button.hpp"

namespace FluentDesign
{
    class SettingsLine
    {
    public:
        enum State
        {
            Normal,
            Closed,
            Opened,
            Next,
            Link,
            Caption
        };

    private:
        int m_linePadding;
        int m_leftMargin;

    protected:
        HWND m_hWnd;
        HWND m_hParent;
        HWND m_hChildControl;

        std::wstring m_name;
        std::wstring m_description;
        wchar_t m_icon;

        int m_left, m_top;
        int m_width, m_height;

        int m_designHeight;
        int m_designPadding;

        bool m_visible;
        bool m_enabled;
        bool m_hovered;
        bool m_childFocused;
        bool m_isGroupItem;

        UINT m_frameFlags;

        State m_state;

        const int CHEVRON_SIZE = 12;

        Button m_chevronButton;

    private:
        virtual LPARAM OnCommand(HWND hwnd, int msg, WPARAM wParam, LPARAM lParam) { return 0; };
        virtual LPARAM OnDrawItem(HWND hwnd, LPDRAWITEMSTRUCT dis) { return 0; };

        Theme &m_theme;
        std::list<SettingsLine *> m_groupItemsList;


    public:

        explicit SettingsLine(Theme &theme);

        explicit SettingsLine(Theme &theme, HWND hParent,
            const std::wstring &name, const std::wstring &description,
            int x, int y, int width, int height, int padding);

        explicit SettingsLine(Theme &theme, HWND hParent,
            const std::wstring &name, const std::wstring &description,
            int x, int y, int width, int height, int padding,
            std::function<HWND(HWND)> createChild);

        ~SettingsLine();

        HWND Create(HWND hParent, const std::wstring &name,
            const std::wstring &description,
            int x, int y, int width, int height);

        HWND GetHWnd() const { return m_hWnd; }

        // Child control management
        void SetChildControl(HWND hChildControl);
        HWND GetChildControl() const { return m_hChildControl; }

        // Text management
        void SetName(const std::wstring &name);
        void SetDescription(const std::wstring &description);
        const std::wstring &GetName() const { return m_name; }
        const std::wstring &GetDescription() const { return m_description; }

        // State management
        void Enable(bool enable = true);
        void Disable();
        bool IsEnabled() const { return m_enabled; }

        void SetFrame(UINT flags)
        {
            m_frameFlags = flags;
            InvalidateRect(m_hWnd, NULL, FALSE);
        }

        // Sizing
        void SetSize(int width, int height);
        int GetDesignHeight() const;
        int GetDesignPadding() const;
        void SetTop(int top);
        void SetLeftMargin(int margin);

        void SetState(State state);
        State GetState() { return m_state; }

        void AddGroupItem(SettingsLine *groupItem);
        void UpdateLayout();
        bool IsNested() const { return m_isGroupItem; }

        void SetIcon(wchar_t icon);

        Event OnChanged;

    protected:
        // Window procedure and message handlers
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

        void Invalidate(BOOL bErase = FALSE);

        // Message handlers
        void OnPaint();

        void DrawBackground(HDC hdc, const RECT &rect, bool frame=true);

        void OnSize(int width, int height);
        void OnMouseMove();
        void OnMouseLeave();
        void OnEnable(bool enabled);
        void OnLButtonDown();

        // Drawing methods
        void DrawText(HDC hdc);
        void DrawIcon(HDC hdc);
        void DrawChevron(HDC hdc);

        // Layout management

        void PositionChildControl();
    };
}