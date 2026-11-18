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
#include "FluentControl.hpp"
#include "Tools/Event.hpp"
#include "Tools/Process.hpp"
#include "Button.hpp"
#include "Popup.hpp"
#include "Tooltip.hpp"

namespace FluentDesign
{
    class SettingsLine : public FluentControl
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
            Menu,
            Info
        };

    private:
        int m_linePadding;
        int m_leftMargin;

    protected:
        HWND m_hParent = nullptr;

        std::list<HWND> m_childControlsList;

        std::wstring m_name;
        std::wstring m_description;

        std::map<int, std::wstring> m_data;

        wchar_t m_icon;

        HICON m_hIcon;
        HICON m_hOverlayIcon = nullptr;

        int m_left = 0, m_top = 0;
        int m_width = 0, m_height = 0;

        int m_designHeight = 0;
        int m_designPadding = 0;
        mutable int m_actualDesignHeight = 0;

        bool m_visible;
        bool m_enabled;
        bool m_hovered;
        bool m_childFocused = false;
        SettingsLine *m_groupLine;

        std::list<Button> m_linkButtons;
        int m_nMaxColumns = 2;



        UINT m_frameFlags;

        State m_state;

        RECT m_secondaryTextRect;

        const int LINK_PADDING = 48;
        const int LINK_VPADDING = 8;
        const int CHEVRON_SIZE = 24;
        const int CHEVRON_SPACE = 9;

        Button m_chevronButton;
        Tooltip m_tooltip;
        std::wstring m_infoText;

    private:
        virtual LPARAM OnCommand(HWND hwnd, int msg, WPARAM wParam, LPARAM lParam) { return 0; };
        virtual LPARAM OnDrawItem(HWND hwnd, LPDRAWITEMSTRUCT dis) { return 0; };

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
        SettingsLine& AddChildControl(HWND hChildControl);
        HWND GetChildControl(int n = 0);

        // Text management
        SettingsLine& SetName(const std::wstring &name);
        SettingsLine& SetDescription(const std::wstring &description);
        SettingsLine& SetInfo(const std::wstring &text);
        const std::wstring &GetName() const { return m_name; }
        const std::wstring &GetDescription() const { return m_description; }

        // State management
        SettingsLine& Enable(bool enable = true);
        SettingsLine& Disable();
        SettingsLine& Show(bool show = true);
        bool IsVisible() const { return m_visible; }
        bool IsEnabled() const { return m_enabled; }

        SettingsLine& SetFrame(UINT flags)
        {
            m_frameFlags = flags;
            InvalidateRect(m_hWnd, NULL, FALSE);
            return *this;
        }

        // Sizing
        SettingsLine& SetSize(int width, int height);
        int GetDesignHeight() const;
        int GetDesignPadding() const;

        int GetTotalHeight() const;
        SettingsLine& SetTop(int top);
        SettingsLine& SetLeftMargin(int margin);

        SettingsLine& SetState(State state);
        State GetState() { return m_state; }

        SettingsLine& AddGroupItem(SettingsLine *groupItem);
        void DeleteGroupItem(SettingsLine *groupItem);
        SettingsLine *GetGroupHeader() { return m_groupLine; }
        std::list<SettingsLine *> &GetGroupItems() { return m_groupItemsList; }
        void EnsureChevron();
        SettingsLine& SetChevron(State state);
        bool HasChevron() const;
        void UpdateLayout();
        bool IsNested() const { return m_groupLine; }
        void Invalidate(BOOL bErase = FALSE);

        SettingsLine& SetIcon(wchar_t icon);
        SettingsLine& SetIcon(const std::wstring& path);
        // Draw over the main icon at its bottom-right corner. Empty path clears the overlay.
        SettingsLine& SetOverlayIcon(const std::wstring& path);

        SettingsLine& SetMenu(const std::vector<Popup::PopupItem> &items);
        SettingsLine& SetData(int index, const std::wstring &data)
        {
            m_data[index] = data;
            return *this;
        }
        std::wstring GetData(int index);

        void NavigateTo(const std::wstring &url);

        SettingsLine& AddLinkButton(const std::wstring &name, const std::wstring &url);
        SettingsLine& SetMaxColumns(int nColumns);

        Event OnChanged;
        Event OnLink;

        protected:
        // Window procedure and message handlers
        static LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

        int ReflowLinkButtons(int width);

        // Message handlers
        void OnPaint(HDC targetDC = nullptr);

        void DrawBackground(HDC hdc, const RECT &rect, bool frame=true);

        void OnSize(int width, int height);
        void OnMouseMove();
        void OnMouseLeave();
        void OnEnable(bool enabled);
        void OnLButtonDown();
        void OnChevronClick();

        // Drawing methods
        void DrawText(HDC hdc);
        void DrawIcon(HDC hdc);

        // Layout management

        RECT GetTextRect(const RECT &clientRect) const;
        int GetTextBlockHeight(int maximumWidth) const;
        int GetActualDesignHeight() const;
        void PositionChildControl();

    };
}
