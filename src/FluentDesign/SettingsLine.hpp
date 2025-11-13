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
#include <string>
#include <functional>
#include "Theme.hpp"
#include "Tools/Event.hpp"

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