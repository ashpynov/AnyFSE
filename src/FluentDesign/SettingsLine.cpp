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


#include "SettingsLine.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"
#include "Tools/List.hpp"
#include "Tools/Window.hpp"
#include "Tools/Icon.hpp"
#include <commctrl.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <vssym32.h>

namespace FluentDesign
{
    static Logger log = LogManager::GetLogger("SettingsLine");

    // Window class registration
    static const wchar_t *SETTINGS_LINE_CLASS = L"AnyFSE_SettingsLineClass";

    SettingsLine::SettingsLine(FluentDesign::Theme& theme)
        : m_theme(theme)
        , m_chevronButton(theme)
        , m_enabled(true)
        , m_hovered(false)
        , m_hWnd(nullptr)
        , m_linePadding(4)
        , m_leftMargin(16)
        , m_state(State::Normal)
        , m_frameFlags(Gdiplus::FrameFlags::CORNER_ALL | Gdiplus::FrameFlags::SIDE_ALL)
        , m_visible(true)
        , m_groupLine(nullptr)
        , m_icon(L'\0')
        , m_hIcon(nullptr)
    {
    }

    SettingsLine::SettingsLine(
            FluentDesign::Theme& theme,
            HWND hParent,
            const std::wstring &name,
            const std::wstring &description,
            int x, int y, int width, int height, int padding)
        : SettingsLine(theme)
    {
        m_designPadding = padding;
        Create(hParent, name, description, x, y, width, height);
    }

    SettingsLine::SettingsLine(
            FluentDesign::Theme& theme,
            HWND hParent,
            const std::wstring &name,
            const std::wstring &description,
            int x, int y, int width, int height, int padding,
            std::function<HWND(HWND)> createChild)
        : SettingsLine(theme, hParent, name, description, x,y, width, height, padding)
    {
        AddChildControl(createChild(GetHWnd()));
    }

    SettingsLine::~SettingsLine()
    {
        if (m_hIcon)
        {
            DestroyIcon(m_hIcon);
            m_hIcon = nullptr;
        }
        if (m_hWnd)
        {
            DestroyWindow(m_hWnd);
            m_hWnd = nullptr;
        }
    }

    HWND SettingsLine::Create(
        HWND hParent,
        const std::wstring &name,
        const std::wstring &description,
        int x, int y,
        int width, int height
    )
    {

        m_hParent = hParent;
        m_name = name;
        m_description = description;
        m_left = x;
        m_top = y;
        m_width = width;
        m_height = height;
        m_designHeight = m_theme.DpiUnscale(height);

        // Calculate position and size
        RECT parentRect;
        GetClientRect(m_hParent, &parentRect);

        // Create the window
        m_hWnd = CreateWindowEx(
            WS_EX_CONTROLPARENT,
            L"BUTTON",
            L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP,
            m_left, m_top, m_width, m_height,
            m_hParent,
            NULL,
            GetModuleHandle(NULL),
            this);

        if (!m_hWnd)
        {
            return NULL;
        }
        SetWindowSubclass(m_hWnd, WndProc, 0, (DWORD_PTR)this);

        return m_hWnd;
    }

    // Window procedure
    LRESULT CALLBACK SettingsLine::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        SettingsLine *This = reinterpret_cast<SettingsLine *>(dwRefData);

        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(hWnd, WndProc, uIdSubclass);
            return DefSubclassProc(hWnd, message, wParam, lParam);
        }
        return This->HandleMessage(message, wParam, lParam);
    }

    LRESULT SettingsLine::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_PAINT:
            OnPaint();
            return 0;

        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_MOUSEMOVE:
            OnMouseMove();
            return 0;

        case WM_NCMOUSELEAVE:
        case WM_MOUSELEAVE:
            OnMouseLeave();
            return 0;

        case WM_LBUTTONDOWN:
            OnLButtonDown();
            return 0;

        case WM_ENABLE:
            OnEnable(wParam != 0);
            return 0;

        case WM_ERASEBKGND:
            {
                if(lParam)
                {
                    RECT childRect;
                    GetClientRect((HWND)lParam, &childRect);
                    DrawBackground((HDC)wParam, childRect, false);
                    m_theme.DrawChildFocus((HDC)wParam, (HWND)lParam, (HWND)lParam);
                }
            }
            return 1; // We handle background in WM_PAINT

        case WM_SETFOCUS:
            {
                SetFocus(m_childControlsList.size() ? m_childControlsList.front() : m_chevronButton.GetHwnd());
                Invalidate();
            }
            break;

        }

        return DefSubclassProc(m_hWnd, message, wParam, lParam);
    }

    void SettingsLine::Invalidate(BOOL bErase)
    {
        RedrawWindow(m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
    }

    void SettingsLine::OnPaint()
    {
        FluentDesign::DoubleBuferedPaint paint(m_hWnd);
        // Draw background

        DrawBackground(paint.MemDC(), paint.ClientRect());

        DrawIcon(paint.MemDC());

        // Draw text
        DrawText(paint.MemDC());

        for (auto child : m_childControlsList)
        {
            m_theme.DrawChildFocus(paint.MemDC(), m_hWnd, child);
        }

        m_theme.DrawChildFocus(paint.MemDC(), m_hWnd, m_chevronButton.GetHwnd());
    }

    void SettingsLine::DrawBackground(HDC hdc, const RECT &rect, bool frame)
    {
        if (m_state == State::Caption)
        {
            return;
        }

        using namespace Gdiplus;
        Graphics graphics(hdc);

        RectF br = ToRectF(rect);

        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetPixelOffsetMode(PixelOffsetModeNone);

        SolidBrush backBrush(m_theme.GetColor(m_hovered? Theme::Colors::PanelHover : Theme::Colors::Panel));
        if (frame)
        {
            br.Offset(-0.5, -0.5);
            Pen borderPen(m_theme.GetColor(Theme::Colors::PanelBorder),m_theme.DpiScaleF(1));
            Gdiplus::RoundRect(graphics, br, (REAL)m_theme.GetSize_Corner(), &backBrush, borderPen, m_frameFlags);
        }
        else
        {
            br.Inflate(1, 1);
            graphics.FillRectangle(&backBrush, br);
        }
    }

    void SettingsLine::DrawText(HDC hdc)
    {
        RECT rect;
        GetClientRect(m_hWnd, &rect);

        COLORREF textColor = m_enabled ? m_theme.GetColorRef(Theme::Colors::Text)
                                       : m_theme.GetColorRef(Theme::Colors::TextDisabled);

        COLORREF descColor = m_enabled ? m_theme.GetColorRef(Theme::Colors::TextSecondary)
                                       : m_theme.GetColorRef(Theme::Colors::TextDisabled);

        SetBkMode(hdc, TRANSPARENT);

        // Draw name
        rect.left += (m_state != State::Caption) ? m_theme.DpiScale(m_leftMargin) : 0;   // Margin

        if (m_icon || m_hIcon)
        {
            rect.left += m_theme.GetSize_Icon() + m_theme.DpiScale(m_leftMargin);
        }
        //rect.right -= 160; // Space for child control

        SelectObject(hdc, (m_state != State::Caption) ? m_theme.GetFont_Text() : m_theme.GetFont_TextBold());
        SetTextColor(hdc, textColor);

        int height = m_theme.GetSize_Text();
        if (!m_description.empty())
        {
            height += m_theme.DpiScale(m_linePadding) + m_theme.GetSize_TextSecondary();
        }

        RECT nameRect = rect;
        nameRect.top = (m_state != State::Caption)
            ? (rect.bottom - rect.top - height) / 2
            : (rect.bottom - height - m_theme.DpiScale(m_linePadding));

        nameRect.bottom = nameRect.top + m_theme.GetSize_Text();

        ::DrawText(hdc, m_name.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

        // Draw description
        SelectObject(hdc, m_theme.GetFont_TextSecondary());
        SetTextColor(hdc, descColor);

        RECT descRect = rect;
        descRect.top = nameRect.bottom + m_theme.DpiScale(m_linePadding);
        descRect.bottom = descRect.top + m_theme.GetSize_TextSecondary();

        ::DrawText(hdc, m_description.c_str(), -1, &descRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE| DT_NOCLIP);
    }

    void SettingsLine::DrawIcon(HDC hdc)
    {
        if (!m_icon && !m_hIcon)
        {
            return;
        }

        RECT rect;
        GetClientRect(m_hWnd, &rect);

        rect.left += (m_state != State::Caption) ? m_theme.DpiScale(m_leftMargin) : 0;
        rect.top = (rect.bottom - rect.top - m_theme.GetSize_Icon()) / 2;
        rect.bottom = rect.top + m_theme.GetSize_Icon();


        if (m_hIcon)
        {
            ::DrawIconEx(hdc, rect.left, rect.top, m_hIcon, m_theme.GetSize_Icon(), m_theme.GetSize_Icon(), 0, nullptr, DI_NORMAL);
        }
        else
        {
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, m_theme.GetFont_Icon());
            SetTextColor(hdc, m_theme.GetColorRef(m_enabled ? FluentDesign::Theme::Text : FluentDesign::Theme::TextDisabled));


            ::DrawText(hdc, &m_icon, 1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
        }

    }

    void SettingsLine::OnSize(int width, int height)
    {
        m_width = width;
        m_height = height;
        UpdateLayout();
    }

    void SettingsLine::OnMouseMove()
    {
        if (!m_hovered && (m_childControlsList.size() || HasChevron()) && m_state != State::Normal && m_state != State::Menu)
        {
            POINT pt;
            GetCursorPos(&pt);
            HWND hWndUnderCursor = WindowFromPoint(pt);

            if (hWndUnderCursor == m_hWnd)
            {
                m_hovered = true;
                Invalidate();
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
                TrackMouseEvent(&tme);
            }
            else if (List::index_of(m_childControlsList, hWndUnderCursor) != List::npos
                || hWndUnderCursor == m_chevronButton.GetHwnd())
            {
                m_hovered = true;
                Invalidate();
            }
        }
    }

    void SettingsLine::OnMouseLeave()
    {
        if ((m_childControlsList.size() || HasChevron()) && m_state != State::Normal && m_state != State::Menu)
        {
            POINT pt;
            GetCursorPos(&pt);
            HWND hWndUnderCursor = WindowFromPoint(pt);

            if (hWndUnderCursor == m_hWnd)
            {
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
                TrackMouseEvent(&tme);
            }
            else if (List::index_of(m_childControlsList, hWndUnderCursor) == List::npos
                 && hWndUnderCursor != m_chevronButton.GetHwnd())
            {
                m_hovered = false;
                Invalidate();
            }
        }
        else
        {
            m_hovered = false;
            Invalidate();
        }
    }

    void SettingsLine::OnEnable(bool enabled)
    {
        m_enabled = enabled;
        for (auto hwnd: m_childControlsList)
        {
            SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
            EnableWindow(hwnd, enabled);
            SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
        }
        Invalidate();
    }

    void SettingsLine::OnLButtonDown()
    {
        // Focus the child control when clicked
        if (m_state == State::Opened || m_state == State::Closed )
        {
            SetState(m_state == State::Closed ?  State::Opened : State::Closed);
            OnChanged.Notify();
        }
        else if (m_state != State::Menu && HasChevron())
        {
            OnChanged.Notify();
        }

        Invalidate();
    }

    void SettingsLine::EnsureChevron()
    {
        if (m_state != State::Normal && m_state != State::Caption && !m_chevronButton.GetHwnd())
        {
            m_chevronButton.Create(
                m_hWnd,
                m_width - m_theme.DpiScale(20) - m_theme.DpiScale(CHEVRON_SIZE),
                (m_height - m_theme.DpiScale(CHEVRON_SIZE))/2,
                m_theme.DpiScale(CHEVRON_SIZE),
                m_theme.DpiScale(CHEVRON_SIZE)
            );
            m_chevronButton.SetFlat(true);
            m_chevronButton.OnButtonDown += delegate(OnLButtonDown);
            m_chevronButton.SetColors(
                Theme::Colors::Default, Theme::Colors::Default,
                Theme::Colors::Default, Theme::Colors::PanelHover,
                Theme::Colors::Default, Theme::Colors::PanelHover
            );
        }
    }

    void SettingsLine::SetChevron(State state)
    {
        bool bShow = state != State::Normal && state != State::Caption;
        if (bShow)
        {
            EnsureChevron();
            std::wstring icon;
            switch (state)
            {
                case State::Closed:
                    icon = L"\xE70D";
                    break;
                case State::Opened:
                    icon = L"\xE70E";
                    break;
                case State::Link:
                    icon = L"\xe8a7";
                    break;
                case State::Next:
                    icon = L"\xE76c";
                    break;
                case State::Menu:
                    icon = L"\xE712";
                    break;
            }
            m_chevronButton.SetIcon(icon);
            ShowWindow(m_chevronButton.GetHwnd(), SW_SHOW);
            UpdateLayout();
            OnMouseMove();
        }
        else if (m_chevronButton.GetHwnd())
        {
            ShowWindow(m_chevronButton.GetHwnd(), SW_HIDE);
            UpdateLayout();
            OnMouseLeave();
        }
    }

    static bool IsSelfVisible(HWND hwnd)
    {
        return hwnd && (GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE);
    }

    bool SettingsLine::HasChevron()
    {
        return m_chevronButton.GetHwnd() && IsSelfVisible(m_chevronButton.GetHwnd());
    }

    void SettingsLine::UpdateLayout()
    {
        PositionChildControl();

        RECT rect;
        Window::GetChildRect(m_hWnd, &rect);
        UINT top = rect.top + m_theme.DpiScale(GetDesignHeight() + GetDesignPadding());
        int width = rect.right - rect.left;

        if (m_groupItemsList.size() > 0)
        {
            bool bShow = m_state != State::Closed && m_state != State::Normal && IsSelfVisible(m_hWnd);

            for (auto& gr : m_groupItemsList)
            {
                gr->m_visible = bShow;
                ShowWindow(gr->m_hWnd, bShow ? SW_SHOW : SW_HIDE);
                if (bShow)
                {
                    ::MoveWindow(gr->m_hWnd, rect.left, top, width, m_theme.DpiScale(gr->GetDesignHeight()), FALSE);
                    gr->UpdateLayout();
                    top += gr->GetTotalHeight();
                }
            }
        }
        Invalidate();
    }

    void SettingsLine::SetIcon(wchar_t icon)
    {
        m_icon = icon;
        UpdateLayout();
    }

    void SettingsLine::SetIcon(const std::wstring &path)
    {
        m_hIcon = Icon::LoadIcon(path);
        UpdateLayout();
    }

    void SettingsLine::SetMenu(const std::vector<Popup::PopupItem> &items)
    {
        SetState(State::Menu);
        m_chevronButton.SetMenu(items);
        UpdateLayout();
    }

    void SettingsLine::PositionChildControl()
    {
        if (m_childControlsList.size()==0 && !HasChevron())
            return;

        int rightPos = m_width - (m_state != State::Caption ? m_theme.DpiScale(20) : 0);

        if( HasChevron() )
        {
            MoveWindow( m_chevronButton.GetHwnd(),
                        rightPos - m_theme.DpiScale(CHEVRON_SIZE),
                        (m_height - m_theme.DpiScale(CHEVRON_SIZE))/2,
                        m_theme.DpiScale(CHEVRON_SIZE),
                        m_theme.DpiScale(CHEVRON_SIZE),
                        FALSE
            );
            rightPos -= m_theme.DpiScale(CHEVRON_SIZE + CHEVRON_SPACE);
        }

        for (auto it = m_childControlsList.rbegin(); it != m_childControlsList.rend(); ++it)
        {
            HWND hChildControl = *it;

            RECT rc;
            GetWindowRect(hChildControl, &rc);
            MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, 2);

            int controlWidth = rc.right - rc.left;
            int controlHeight = rc.bottom - rc.top;
            int controlY = (m_height - controlHeight) / 2;

            SetWindowPos(hChildControl, NULL,
                        rightPos - controlWidth,
                        controlY,
                        0, 0,
                        SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);

            rightPos -= controlWidth + m_theme.DpiScale(4);
        }
    }

    std::wstring SettingsLine::GetData(int index)
    {
        auto d = m_data.find(index);
        return d != m_data.end() ? d->second : L"";
    }

    // Public methods implementation
    void SettingsLine::AddChildControl(HWND hChildControl)
    {
        SetParent(hChildControl, m_hWnd);
        PositionChildControl();
        EnableWindow(hChildControl, m_enabled);
        ShowWindow(hChildControl, SW_SHOWNOACTIVATE);
        m_childControlsList.push_back(hChildControl);
    }

    HWND SettingsLine::GetChildControl(int n)
    {
        if (n < 0 || n > m_childControlsList.size())
        {
            return nullptr;
        }
        auto it = m_childControlsList.begin();
        std::advance(it, n);
        return *it;
    }

    void SettingsLine::SetName(const std::wstring &name)
    {
        m_name = name;
        Invalidate();
    }

    void SettingsLine::SetDescription(const std::wstring &description)
    {
        m_description = description;
        Invalidate();
    }

    void SettingsLine::Enable(bool enable)
    {
        m_enabled = enable;
        EnableWindow(m_hWnd, enable);
        for (auto hwnd: m_childControlsList)
        {
            EnableWindow(hwnd, enable);
        }
        Invalidate();
    }

    void SettingsLine::Disable()
    {
        Enable(false);
    }

    void SettingsLine::SetSize(int width, int height)
    {
        m_width = width;
        m_height = height;
        //m_designHeight = m_theme.DpiUnscale(height);
        SetWindowPos(m_hWnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }

    int SettingsLine::GetDesignHeight() const
    {
        return m_visible ? m_designHeight : 0;
    }

    int SettingsLine::GetDesignPadding() const
    {
        int padding = m_designPadding;
        if (m_groupItemsList.size() > 0 && m_state != State::Opened)
        {
            padding = m_groupItemsList.back()->m_designPadding;
        }
        return m_visible ? padding : 0;
    }

    int SettingsLine::GetTotalHeight() const
    {
        UINT height = m_theme.DpiScale(GetDesignHeight() + GetDesignPadding());
        if (m_groupItemsList.size() > 0)
        {
            for (auto& gr : m_groupItemsList)
            {
                if (gr->m_visible)
                {
                    height += m_theme.DpiScale(gr->GetDesignHeight() + gr->GetDesignPadding());
                }
            }
        }
        return height;
    }

    void SettingsLine::SetTop(int top)
    {
        m_top = top;
        SetWindowPos(m_hWnd, NULL, m_left, m_top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    void SettingsLine::SetLeftMargin(int margin)
    {
        m_leftMargin = margin;
        Invalidate();
    }
    void SettingsLine::SetState(State state)
    {
        m_state = state;

        if (m_state == State::Opened && m_groupItemsList.size())
        {
            m_frameFlags = Gdiplus::FrameFlags::SIDE_NO_BOTTOM | Gdiplus::FrameFlags::CORNER_TOP;

            for (auto& gr : m_groupItemsList)
            {
                gr->SetFrame(Gdiplus::FrameFlags::SIDE_NO_BOTTOM);
            }
            m_groupItemsList.back()->SetFrame(Gdiplus::FrameFlags::SIDE_ALL | Gdiplus::FrameFlags::CORNER_BOTTOM);

        }
        else if (m_state == State::Closed)
        {
            m_frameFlags = Gdiplus::FrameFlags::SIDE_ALL | Gdiplus::FrameFlags::CORNER_ALL;
        }
        SetChevron(m_state);

        UpdateLayout();
    }

    void SettingsLine::AddGroupItem(SettingsLine *groupItem)
    {
        m_groupItemsList.push_back(groupItem);
        groupItem->m_groupLine = this;
    }

    void SettingsLine::DeleteGroupItem(SettingsLine *groupItem)
    {
        m_groupItemsList.remove_if([groupItem](const SettingsLine* obj) {return obj == groupItem;});
    }
}