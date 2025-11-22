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
#include "Popup.hpp"

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
            Caption,
            Menu
        };

    private:
        int m_linePadding;
        int m_leftMargin;

    protected:
        HWND m_hWnd;
        HWND m_hParent;

        std::list<HWND> m_childControlsList;

        std::wstring m_name;
        std::wstring m_description;

        std::map<int, std::wstring> m_data;

        wchar_t m_icon;

        HICON m_hIcon;

        int m_left, m_top;
        int m_width, m_height;

        int m_designHeight;
        int m_designPadding;

        bool m_visible;
        bool m_enabled;
        bool m_hovered;
        bool m_childFocused;
        SettingsLine *m_groupLine;

        UINT m_frameFlags;

        State m_state;

        const int CHEVRON_SIZE = 24;

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
        void AddChildControl(HWND hChildControl);
        HWND GetChildControl(int n = 0);

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

        int GetTotalHeight() const;
        void SetTop(int top);
        void SetLeftMargin(int margin);

        void SetState(State state);
        State GetState() { return m_state; }

        void AddGroupItem(SettingsLine *groupItem);
        void DeleteGroupItem(SettingsLine *groupItem);
        SettingsLine *GetGroupHeader() { return m_groupLine; }
        void EnsureChevron();
        void SetChevron(State state);
        bool HasChevron();
        void UpdateLayout();
        bool IsNested() const { return m_groupLine; }
        void Invalidate(BOOL bErase = FALSE);

        void SetIcon(wchar_t icon);
        void SetIcon(const std::wstring& path);

        void SetMenu(const std::vector<Popup::PopupItem> &items);
        void SetData(int index, const std::wstring &data) { m_data[index] = data; }
        std::wstring GetData(int index);

        Event OnChanged;

    protected:
        // Window procedure and message handlers
        static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);


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

        // Layout management

        void PositionChildControl();

    };
}