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

#include "SettingsDialog.hpp"
#include <windows.h>
#include <Windows.h>
#include <algorithm>
#include "Logging/LogManager.hpp"
#include "SettingsDialog.hpp"
#include "Configuration/Config.hpp"
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/Popup.hpp"
#include "Tools/List.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/Window.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Process.hpp"

#define byte ::byte

#include "Tools/GdiPlus.hpp"


namespace AnyFSE::App::AppSettings::Settings
{
    static Logger log = LogManager::GetLogger("Settings");

    INT_PTR SettingsDialog::Show(HINSTANCE hInstance)
    {
        size_t size = sizeof(DLGTEMPLATE) + sizeof(WORD) * 3; // menu, class, title
        HGLOBAL hGlobal = GlobalAlloc(GHND, size);
        if (!hGlobal)
        {
            return -1;
        }

        LPDLGTEMPLATE dlgTemplate = (LPDLGTEMPLATE)GlobalLock(hGlobal);
        if (!dlgTemplate)
        {
            return -1;
        }

        dlgTemplate->style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
        dlgTemplate->dwExtendedStyle = WS_EX_WINDOWEDGE;
        dlgTemplate->cdit = 0;  // No controls
        dlgTemplate->x = 0;
        dlgTemplate->y = 0;
        dlgTemplate->cx = 0;
        dlgTemplate->cy = 0;

        // No menu, class, or title
        WORD* ptr = (WORD*)(dlgTemplate + 1);
        *ptr++ = 0; // No menu
        *ptr++ = 0; // Default dialog class
        *ptr++ = 0; // No title

        GlobalUnlock(hGlobal);
        INT_PTR res = DialogBoxIndirectParam(hInstance, dlgTemplate, NULL, DialogProc, (LPARAM)this);
        GlobalFree(hGlobal);
        if (res == -1)
        {
            log.Error(log.APIError(),"Cant create settings dialog)");
        }
        return res;
    }

    void SettingsDialog::CenterDialog(HWND hwnd)
    {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        int cx = m_theme.DpiScale(700);
        int cy = m_theme.DpiScale(600);

        // Calculate center position
        int x = (screenWidth - cx) / 2;
        int y = (screenHeight - cy) / 2;

        // Move dialog to center
        SetWindowPos(hwnd, NULL, x, y, cx, cy, SWP_NOZORDER);
    }

    void SettingsDialog::StoreWindowPlacement()
    {
        WINDOWPLACEMENT wp{sizeof(WINDOWPLACEMENT)};
        GetWindowPlacement(m_hDialog, &wp);
        wp.rcNormalPosition;
        Config::SaveWindowPlacement(wp.showCmd, m_theme.DpiUnscale(wp.rcNormalPosition));
    }

    void SettingsDialog::RestoreWindowPlacement()
    {
        WINDOWPLACEMENT wp{sizeof(WINDOWPLACEMENT)};
        wp.showCmd = Config::LoadWindowPlacement(&wp.rcNormalPosition);

        if (!wp.showCmd)
        {
            return CenterDialog(m_hDialog);
        }

        if (wp.showCmd != SW_MAXIMIZE)
        {
            wp.showCmd = SW_NORMAL;
        }

        wp.rcNormalPosition = m_theme.DpiScale(wp.rcNormalPosition);
        SetWindowPlacement(m_hDialog, &wp);
    }

    // Static dialog procedure - routes to instance method
    INT_PTR CALLBACK SettingsDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        SettingsDialog *This = nullptr;

        if (msg == WM_INITDIALOG)
        {
            // Store 'this' pointer in window user data
            This = (SettingsDialog *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)This);
            This->m_hDialog = hwnd;
        }
        else
        {
            // Get 'this' pointer from window user data
            This = (SettingsDialog *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (This)
        {
            return This->InstanceDialogProc(hwnd, msg, wParam, lParam);
        }

        return FALSE;
    }

    // Instance dialog procedure - has access to all member variables
    INT_PTR SettingsDialog::InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            {   // To enable immersive dark mode (for a dark title bar)

                m_theme.AttachDlg(hwnd);
                SetWindowText(hwnd, L"AnyFSE Settings");
                OnInitDialog(hwnd);
                RestoreWindowPlacement();
            }
            return TRUE;

        case WM_DESTROY:
            StoreWindowPlacement();
            return FALSE;

        case WM_KEYDOWN:
            if (   wParam == VK_LEFT
                || wParam == VK_UP
                || wParam == VK_RIGHT
                || wParam == VK_DOWN
                || wParam == VK_TAB
            )
            {
                //This->m_isKeyboardFocus = true;
                if (wParam == VK_TAB && (GetKeyState(VK_SHIFT) & 0x8000))
                {
                    m_theme.KewboardNavigate(VK_PRIOR);
                }
                else if (wParam == VK_TAB)
                {
                    m_theme.KewboardNavigate(VK_NEXT);
                }
                else
                {
                    m_theme.KewboardNavigate((WORD)wParam);
                }
                return FALSE;
            }
            return TRUE;

        case WM_NCCALCSIZE:
            {
                NCCALCSIZE_PARAMS* sz = reinterpret_cast< NCCALCSIZE_PARAMS* >( lParam );
                InflateRect(&sz->rgrc[0], -1, -1);
                SetWindowLong(m_hDialog, DWLP_MSGRESULT, 0);
            }
            return TRUE;

        case WM_NCHITTEST:
            {
                LONG hit = (LONG)DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
                // Get mouse position
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(hwnd, &pt);

                // Get client rect
                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                if (!Window::MouseInClientRect(hwnd, NULL, m_theme.DpiScale(5)))
                {
                    SetWindowLong(m_hDialog, DWLP_MSGRESULT, HTCLIENT);
                    return TRUE;
                }

                int sizeBorder = m_theme.DpiScale(5);
                bool bottom = (pt.y > clientRect.bottom - sizeBorder);
                bool top = (pt.y < sizeBorder);
                int captionHeight = m_theme.DpiScale(GetSystemMetrics(SM_CYCAPTION));
                // If mouse is in top-right area where buttons are, treat as client
                if (pt.y < captionHeight && !top)
                {
                    SetWindowLong(m_hDialog, DWLP_MSGRESULT, HTCAPTION);
                    return TRUE;
                }

                if (pt.x < sizeBorder)
                {
                    hit = bottom ? HTBOTTOMLEFT : top ? HTTOPLEFT : HTLEFT;
                }
                else if (pt.x > clientRect.right - sizeBorder)
                {
                    hit = bottom ? HTBOTTOMRIGHT : top ? HTTOPRIGHT : HTRIGHT;
                }
                else if (bottom)
                {
                    hit = HTBOTTOM;
                }
                else if (top)
                {
                    hit = HTTOP;
                }

                SetWindowLong(m_hDialog, DWLP_MSGRESULT, hit);

            }
            return TRUE;
        case WM_NCACTIVATE:
            {
                BOOL bActive = (BOOL)wParam;
                BOOL skipDefault = FALSE;
                if (!bActive)
                {

                    bool child = IsChild(m_hDialog, (HWND)lParam);
                    HWND own = GetWindowOwner((HWND)lParam);
                    bool ownchild = IsChild(m_hDialog, own);
                    bActive |= child || ownchild || own == m_hDialog;
                }
                m_captionMinimizeButton.SetColors(bActive ? Theme::Colors::Text : Theme::Colors::TextAccented);
                m_captionMaximizeButton.SetColors(bActive ? Theme::Colors::Text : Theme::Colors::TextAccented);
                m_captionCloseButton.SetColors(bActive ? Theme::Colors::Text : Theme::Colors::TextAccented);
            }
            return FALSE;

        case WM_UPDATE_NOTIFICATION:
            OnUpdateNotification();
            return TRUE;

        case WM_SIZE:
            UpdateLayout();
            if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
            {
                UpdateMaximizeButtonIcon();
            }
            return TRUE;

        case WM_GETMINMAXINFO:
            {
                MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
                pMMI->ptMinTrackSize.x = m_theme.DpiScale(670);
                pMMI->ptMinTrackSize.y = m_theme.DpiScale(400);
            }
            return TRUE;

        case WM_MOUSEWHEEL:
            {
                return m_scrollView.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            }
            break;

        case WM_ERASEBKGND:
            SetWindowLong(m_hDialog, DWLP_MSGRESULT, 0);
            return TRUE;
        case WM_NOTIFY:
            {
                LPNMHDR notification = (LPNMHDR)lParam;
                if (notification->code == NM_SETFOCUS)
                {
                    RECT clientRect;
                    HWND hFocused = notification->hwndFrom;
                    for (HWND parent = GetParent(hFocused); parent; parent = GetParent(parent))
                    {
                        if (parent == m_scrollView.GetHwnd())
                        {
                            GetClientRect(notification->hwndFrom, &clientRect);
                            MapWindowPoints(notification->hwndFrom, m_scrollView.GetHwnd(), (LPPOINT)&clientRect, 2);
                            OffsetRect(&clientRect, 0, m_scrollView.GetScrollPos());
                            m_scrollView.EnsureVisible(clientRect);
                            break;
                        }
                    }

                }
            }

            return FALSE;

        case WM_PAINT:
            {
                FluentDesign::DoubleBuferedPaint paint(hwnd);
                HBRUSH back = CreateSolidBrush(m_theme.GetColorRef(FluentDesign::Theme::Colors::Dialog));
                RECT r = paint.ClientRect();
                FillRect(paint.MemDC(), &r, back);
                DeleteObject(back);

                GetDialogRect(&r);
                r.left += m_theme.DpiScale(Layout::MarginLeft);
                r.top += m_theme.DpiScale(Layout::CaptionHeight);

                WCHAR *text1 = L"Any Full Screen Experience";

                SetBkMode(paint.MemDC(), TRANSPARENT);
                HGDIOBJ oldFont = SelectFont(paint.MemDC(), m_theme.GetFont_Title());
                ::DrawText(paint.MemDC(), text1, -1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT);

                m_breadCrumbRect = r;

                SetTextColor(paint.MemDC(), m_theme.GetColorRef(
                    m_pageName.empty()  ? FluentDesign::Theme::Colors::Breadcrumb
                    : m_buttonPressed   ? FluentDesign::Theme::Colors::BreadcrumbLinkPressed
                    : m_buttonMouseOver ? FluentDesign::Theme::Colors::BreadcrumbLinkHover
                    : FluentDesign::Theme::Colors::BreadcrumbLink
                ));

                ::DrawText(paint.MemDC(), text1, -1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);

                if (!m_pageName.empty())
                {
                    r.left = r.right;
                    r.right = paint.ClientRect().right;
                    wchar_t *chevron = L"  \xE00F  ";
                    SetTextColor(paint.MemDC(), m_theme.GetColorRef(FluentDesign::Theme::Colors::Text));
                    SelectFont(paint.MemDC(), m_theme.GetFont_GlyphNormal());
                    ::DrawText(paint.MemDC(), chevron, -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT);

                    r.top = m_breadCrumbRect.top + m_theme.DpiScale(8);
                    r.bottom = m_breadCrumbRect.bottom;
                    ::DrawText(paint.MemDC(), chevron, -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

                    r.top = m_breadCrumbRect.top;
                    r.bottom = m_breadCrumbRect.bottom;
                    r.left = r.right;
                    r.right = paint.ClientRect().right;
                    SelectFont(paint.MemDC(), m_theme.GetFont_Title());
                    ::DrawText(paint.MemDC(), m_pageName.c_str(), -1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
                }

                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_okButton.GetHwnd());
                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_closeButton.GetHwnd());
                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_updateCheckButton.GetHwnd());
                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_updateButton.GetHwnd());

                SelectObject(paint.MemDC(), oldFont);
            }
            return FALSE;

        case WM_DPICHANGED:
            UpdateDpiLayout();
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK:
                OnOk();
                EndDialog(hwnd, IDOK);
                return TRUE;
            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            case IDABORT:
                EndDialog(hwnd, IDABORT);
                return TRUE;
            }
            break;

        case WM_MOUSEMOVE:
            if (!m_pageName.empty() && !m_buttonMouseOver && Window::MouseInClientRect(m_hDialog, &m_breadCrumbRect))
            {
                m_buttonMouseOver = true;
                InvalidateRect(m_hDialog, &m_breadCrumbRect, TRUE);

                // Track mouse leave
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = m_hDialog;
                TrackMouseEvent(&tme);
            }
            break;

        case WM_MOUSELEAVE:
            m_buttonMouseOver = false;
            InvalidateRect(m_hDialog, &m_breadCrumbRect, TRUE);
            break;

        case WM_LBUTTONDOWN:
            if (!m_pageName.empty() && !m_buttonPressed && Window::MouseInClientRect(m_hDialog, &m_breadCrumbRect))
            {
                m_buttonPressed = true;
                InvalidateRect(m_hDialog, &m_breadCrumbRect, TRUE);
                SetCapture(m_hDialog);
            }
            break;

        case WM_LBUTTONUP:
            if (m_buttonPressed && Window::MouseInClientRect(m_hDialog, &m_breadCrumbRect))
            {
                OpenSettingsPage();
            }
            m_buttonPressed = false;
            InvalidateRect(m_hDialog, &m_breadCrumbRect, TRUE);
            ReleaseCapture();
            break;
        }

        return FALSE;
    }

    void SettingsDialog::GetDialogRect(RECT *prc)
    {
        GetClientRect(m_hDialog, prc);
        int width = min(m_theme.DpiScale(Layout::MaxWidth), prc->right - prc->left);
        prc->left = (prc->right - prc->left - width) / 2;
        prc->right = prc->left + width;
    }

    FluentDesign::SettingsLine& SettingsDialog::AddSettingsLine(
        std::list<SettingsLine>& page,
        ULONG &top, const std::wstring& name, const std::wstring& desc,
        int height, int padding, int contentMargin, int contentWidth, int contentHeight )
    {
        RECT rect;
        GetDialogRect(&rect);
        int width = rect.right - rect.left
            - m_theme.DpiScale(Layout::MarginLeft - Layout::Border)
            - m_theme.DpiScale(Layout::MarginRight - Layout::Border);


        page.emplace_back(
            m_theme, m_hScrollView, name, desc,
            m_theme.DpiScale(Layout::MarginLeft - Layout::Border ),
            top, width,
            m_theme.DpiScale(height),
            padding
        );

        top += m_theme.DpiScale(height + padding);
        if (contentMargin)
        {
            page.back().SetLeftMargin(contentMargin);
        }

        return page.back();
    }

    void SettingsDialog::SwitchActivePage(const std::wstring& pageName, std::list<SettingsLine> * pPageList, bool back)
    {
        m_pageName = pageName;

        if (!pPageList || pPageList == m_pActivePageList)
        {
            return;
        }

        int invert = back ? -1 : 1;

        RECT scrollRect;
        GetClientRect(m_hScrollView, &scrollRect);
        int width = scrollRect.right - scrollRect.left;
        int steps = 10;
        int delta = width / steps;

        for (auto& line: *pPageList)
        {
            Window::MoveWindow(line.GetHWnd(), width * invert, 0, FALSE);
            if (!line.IsNested())
            {
                ShowWindow(line.GetHWnd(), SW_SHOW);
                line.UpdateLayout();
            }
        }

        RedrawWindow(m_hDialog, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

        for (int i = 0; i < steps; i++)
        {
            if (m_pActivePageList)
            {
                for (auto &line : *m_pActivePageList)
                {
                    Window::MoveWindow(line.GetHWnd(), -delta * invert, 0, TRUE);
                }
            }
            for (auto &line : *pPageList)
            {
                Window::MoveWindow(line.GetHWnd(), -delta * invert, 0, TRUE);
            }
            RedrawWindow(m_hScrollView, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            Sleep(5);
        }
        if (m_pActivePageList)
        {
            for (auto& line : *m_pActivePageList)
            {
                ShowWindow(line.GetHWnd(), SW_HIDE);
                Window::MoveWindow(line.GetHWnd(), width * invert, 0, FALSE);
            }
        }
        m_pActivePageList = pPageList;
        UpdateLayout();
        RedrawWindow(m_hDialog, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

        m_captionBackButton.Enable(m_pActivePageList != &m_settingPageList);
    }

    void SettingsDialog::UpdateDpiLayout()
    {

        RECT rect;
        GetClientRect(m_hDialog, &rect);

        MoveWindow(m_captionStatic.GetHwnd(),
            m_theme.DpiScale(Layout::BackButtonMargin * 2 + Layout::BackButtonSize),
            m_theme.DpiScale(Layout::BackButtonMargin),
            m_theme.DpiScale(200),
            m_theme.DpiScale(Layout::BackButtonSize - 4),
            FALSE);

        MoveWindow(m_captionBackButton.GetHwnd(),
            m_theme.DpiScale(Layout::BackButtonMargin),
            m_theme.DpiScale(Layout::BackButtonMargin),
            m_theme.DpiScale(Layout::BackButtonSize),
            m_theme.DpiScale(Layout::BackButtonSize),
            FALSE);

        MoveWindow(m_captionMaximizeButton.GetHwnd(),
            rect.right - m_theme.DpiScale(Layout::CaptionButtonWidth * 2),
            -1,
            m_theme.DpiScale(Layout::CaptionButtonWidth ),
            m_theme.DpiScale(Layout::CaptionHeight),
            FALSE);

        MoveWindow(m_captionMinimizeButton.GetHwnd(),
            rect.right - m_theme.DpiScale(Layout::CaptionButtonWidth * 3),
            -1,
            m_theme.DpiScale(Layout::CaptionButtonWidth),
            m_theme.DpiScale(Layout::CaptionHeight),
            FALSE);

        MoveWindow(m_captionCloseButton.GetHwnd(),
            rect.right - m_theme.DpiScale(Layout::CaptionButtonWidth),
            -1,
            m_theme.DpiScale(Layout::CaptionButtonWidth),
            m_theme.DpiScale(Layout::CaptionHeight),
            FALSE);

        RECT scrollRect {
            m_theme.DpiScale(Layout::Border),
            rect.top + m_theme.DpiScale(Layout::MarginTop + GetSystemMetrics(SM_CYCAPTION)),
            rect.right - m_theme.DpiScale(Layout::Border),
            rect.bottom
                - m_theme.DpiScale(Layout::MarginBottom)
                - m_theme.DpiScale(Layout::ButtonHeight)
                - m_theme.DpiScale(Layout::ButtonPadding)
            };

        Window::MoveWindow(m_scrollView.GetHwnd(), &scrollRect, FALSE);


        GetDialogRect(&rect);

        rect.top = rect.bottom
            - m_theme.DpiScale(Layout::MarginBottom)
            - m_theme.DpiScale(Layout::ButtonHeight);

        rect.right -= m_theme.DpiScale(Layout::MarginRight);
        rect.left  += m_theme.DpiScale(Layout::MarginLeft);

        MoveWindow(m_okButton.GetHwnd(),
            rect.right
                - m_theme.DpiScale(Layout::OKWidth)
                - m_theme.DpiScale(Layout::ButtonPadding)
                - m_theme.DpiScale(Layout::CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout::OKWidth),
            m_theme.DpiScale(Layout::ButtonHeight),
            FALSE);

        MoveWindow(m_closeButton.GetHwnd(),
            rect.right - m_theme.DpiScale(Layout::CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout::CloseWidth),
            m_theme.DpiScale(Layout::ButtonHeight),
            FALSE);

        MoveUpdateControls();

        UpdateLayout();
    }

    void SettingsDialog::ArrangeControls()
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);

        bool bMaximized = (GetWindowLong(m_hDialog, GWL_STYLE) & WS_MAXIMIZE);

        if (bMaximized)
        {
            InflateRect(&rect, -m_theme.DpiScale(5), -m_theme.DpiScale(5));
        }
        else
        {
            rect.top = -1;
        }

        MoveWindow(m_captionMaximizeButton.GetHwnd(),
            rect.right - m_theme.DpiScale(Layout::CaptionButtonWidth * 2 ),
            rect.top,
            m_theme.DpiScale(Layout::CaptionButtonWidth),
            m_theme.DpiScale(Layout::CaptionHeight),
            FALSE
        );

        MoveWindow(m_captionMinimizeButton.GetHwnd(),
            rect.right - m_theme.DpiScale(Layout::CaptionButtonWidth * 3 ),
            rect.top,
            m_theme.DpiScale(Layout::CaptionButtonWidth),
            m_theme.DpiScale(Layout::CaptionHeight),
            FALSE
        );

        MoveWindow(m_captionCloseButton.GetHwnd(),
            rect.right - m_theme.DpiScale(Layout::CaptionButtonWidth),
            rect.top,
            m_theme.DpiScale(Layout::CaptionButtonWidth),
            m_theme.DpiScale(Layout::CaptionHeight),
            FALSE
        );

        MoveWindow(m_scrollView.GetHwnd(),
            m_theme.DpiScale(Layout::Border),
            rect.top + m_theme.DpiScale(Layout::MarginTop + GetSystemMetrics(SM_CYCAPTION)),
            rect.right - m_theme.DpiScale(Layout::Border * 2),
            rect.bottom
                - m_theme.DpiScale(Layout::MarginBottom)
                - m_theme.DpiScale(Layout::MarginTop + GetSystemMetrics(SM_CYCAPTION))
                - m_theme.DpiScale(Layout::ButtonHeight)
                - m_theme.DpiScale(Layout::ButtonPadding),
            FALSE
        );

        GetDialogRect(&rect);
        rect.top = rect.bottom
            - m_theme.DpiScale(Layout::MarginBottom)
            - m_theme.DpiScale(Layout::ButtonHeight);

        rect.right -= m_theme.DpiScale(Layout::MarginRight);
        rect.left  += m_theme.DpiScale(Layout::MarginLeft);

        MoveWindow(m_okButton.GetHwnd(),
            rect.right
                - m_theme.DpiScale(Layout::OKWidth)
                - m_theme.DpiScale(Layout::ButtonPadding)
                - m_theme.DpiScale(Layout::CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout::OKWidth),
            m_theme.DpiScale(Layout::ButtonHeight),
            FALSE
        );

        MoveWindow(m_closeButton.GetHwnd(),
            rect.right - m_theme.DpiScale(Layout::CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout::CloseWidth),
            m_theme.DpiScale(Layout::ButtonHeight),
            FALSE
        );

        MoveUpdateControls();
    }

    void SettingsDialog::UpdateLine(SettingsLine * line)
    {
        UpdateLayout();
        RECT rc;
        Window::GetChildRect(line->GetHWnd(), &rc);
        OffsetRect(&rc, 0, m_scrollView.GetScrollPos());
        rc.bottom = rc.top + line->GetTotalHeight();

        m_scrollView.EnsureVisible(rc);
    }

    void SettingsDialog::UpdateLayout()
    {
        for (auto &page: m_pages)
        {
            if (&(page->GetSettingsLines()) != m_pActivePageList)
            {
                for (auto& line: page->GetSettingsLines())
                {
                    ShowWindow(line.GetHWnd(), SW_HIDE);
                }
            }
        }

        ArrangeControls();

        RECT rect;
        GetDialogRect(&rect);
        int width = rect.right - rect.left
            - m_theme.DpiScale(Layout::MarginLeft)
            - m_theme.DpiScale(Layout::MarginRight);

        int left = rect.left + m_theme.DpiScale(Layout::MarginLeft);

        ULONG top = -m_scrollView.GetScrollPos();
        if (m_pActivePageList)
        {
            for(auto& line : *m_pActivePageList)
            {
                if (!line.IsNested())
                {
                    ShowWindow(line.GetHWnd(), SW_SHOW);
                    ::MoveWindow(line.GetHWnd(), left, top, width, m_theme.DpiScale(line.GetDesignHeight()), FALSE);
                    line.UpdateLayout();
                    top += line.GetTotalHeight();
                }
            }
        }

        m_scrollView.SetContentHeight(top + m_scrollView.GetScrollPos());

        RedrawWindow(m_hDialog, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }

    HWND SettingsDialog::GetMainWindow()
    {
        return FindWindow(L"AnyFSE", NULL);
    }

    void SettingsDialog::AddMinimizeButton()
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);

        m_captionMinimizeButton.Create(m_hDialog,
            rect.right - m_theme.DpiScale(Layout::CaptionButtonWidth * 3 ),
            -1,
            m_theme.DpiScale(Layout::CaptionButtonWidth),
            m_theme.DpiScale(Layout::CaptionHeight));

        SetWindowLong(m_captionMinimizeButton.GetHwnd(), GWL_STYLE,
            GetWindowLong(m_captionMinimizeButton.GetHwnd(), GWL_STYLE) & ~WS_TABSTOP );

        m_captionMinimizeButton.SetIcon(L"\xE921", true);
        m_captionMinimizeButton.OnChanged += delegate(OnMinimize);
        m_captionMinimizeButton.SetFlat(true);
        m_captionMinimizeButton.SetSquare(true);
    }

    void SettingsDialog::AddMaximizeButton()
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);
        m_captionMaximizeButton.Create(m_hDialog,
            rect.right - m_theme.DpiScale(Layout::CaptionButtonWidth * 2 ),
            -1,
            m_theme.DpiScale(Layout::CaptionButtonWidth),
            m_theme.DpiScale(Layout::CaptionHeight));

        UpdateMaximizeButtonIcon();

        SetWindowLong(m_captionMaximizeButton.GetHwnd(), GWL_STYLE,
            GetWindowLong(m_captionMaximizeButton.GetHwnd(), GWL_STYLE) & ~WS_TABSTOP );

        m_captionMaximizeButton.SetFlat(true);
        m_captionMaximizeButton.SetSquare(true);
        m_captionMaximizeButton.OnChanged += delegate(OnMaximize);
    }

    void SettingsDialog::AddCloseButton()
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);

        m_captionCloseButton.Create(m_hDialog,
            rect.right - m_theme.DpiScale(Layout::CaptionButtonWidth),
            -1,
            m_theme.DpiScale(Layout::CaptionButtonWidth),
            m_theme.DpiScale(Layout::CaptionHeight));

        SetWindowLong(m_captionCloseButton.GetHwnd(), GWL_STYLE,
            GetWindowLong(m_captionCloseButton.GetHwnd(), GWL_STYLE) & ~WS_TABSTOP );

        m_captionCloseButton.SetIcon(L"\xE8BB", true);
        m_captionCloseButton.OnChanged += delegate(OnClose);
        m_captionCloseButton.SetFlat(true);
        m_captionCloseButton.SetSquare(true);

        m_captionCloseButton.SetColors(
            Theme::Colors::Default, Theme::Colors::Default,
            Theme::Colors::Default, Theme::Colors::CloseButtonHover,
            Theme::Colors::Default, Theme::Colors::CloseButtonPressed);
    }

    void SettingsDialog::AddCaption()
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);

        m_captionStatic.Create(m_hDialog,
            m_theme.DpiScale(Layout::BackButtonMargin * 2 + Layout::BackButtonSize),
            m_theme.DpiScale(Layout::BackButtonMargin),
            m_theme.DpiScale(200),
            m_theme.DpiScale(Layout::BackButtonSize - 4));
        m_captionStatic.Format().SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);

        m_captionStatic.SetText(L"Settings");

        m_captionBackButton.Create(m_hDialog,
            m_theme.DpiScale(Layout::BackButtonMargin),
            m_theme.DpiScale(Layout::BackButtonMargin),
            m_theme.DpiScale(Layout::BackButtonSize),
            m_theme.DpiScale(Layout::BackButtonSize));

        m_captionBackButton.SetIcon(L"\xE64E");
        m_captionBackButton.OnChanged += delegate(OpenSettingsPage);
        m_captionBackButton.SetFlat(true);
        m_captionBackButton.Enable(false);
    }

    void SettingsDialog::AddScrollPanel()
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);
        m_hScrollView = m_scrollView.Create(m_hDialog,
            m_theme.DpiScale(Layout::Border),
            rect.top + m_theme.DpiScale(Layout::MarginTop + GetSystemMetrics(SM_CYCAPTION)),
            rect.right - m_theme.DpiScale(Layout::Border * 2),
            rect.bottom
                - m_theme.DpiScale(Layout::MarginBottom)
                - m_theme.DpiScale(Layout::MarginTop + GetSystemMetrics(SM_CYCAPTION))
                - m_theme.DpiScale(Layout::ButtonHeight)
                - m_theme.DpiScale(Layout::ButtonPadding)
        );
    }

    void SettingsDialog::OnInitDialog(HWND hwnd)
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);

        AddCaption();
        AddMinimizeButton();
        AddMaximizeButton();
        AddCloseButton();
        AddScrollPanel();

        ULONG top = 0;

        for (auto& page: m_pages)
        {
            page->AddPage(m_settingPageList, top);
        }


        m_pActivePageList = &m_settingPageList;
        UpdateLayout();

        GetDialogRect(&rect);
        rect.top = rect.bottom
            - m_theme.DpiScale(Layout::MarginBottom)
            - m_theme.DpiScale(Layout::ButtonHeight);

        rect.right -= m_theme.DpiScale(Layout::MarginRight);
        rect.left  += m_theme.DpiScale(Layout::MarginLeft);

        AddUpdateControls();

        m_okButton.Create(m_hDialog,
            rect.right
                - m_theme.DpiScale(Layout::OKWidth)
                - m_theme.DpiScale(Layout::ButtonPadding)
                - m_theme.DpiScale(Layout::CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout::OKWidth),
            m_theme.DpiScale(Layout::ButtonHeight));

        m_okButton.SetText(L"Save");

        m_okButton.OnChanged += delegate(OnOk);

        m_closeButton.Create(m_hDialog,
            rect.right - m_theme.DpiScale(Layout::CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout::CloseWidth),
            m_theme.DpiScale(Layout::ButtonHeight));

        m_closeButton.OnChanged += delegate(OnClose);

        m_closeButton.SetText(L"Discard and Close");

        for (auto& page: m_pages)
        {
            page->LoadControls();
        }
    }

    void SettingsDialog::OnOk()
    {
        SaveSettings();
        EndDialog(m_hDialog, IDOK);
    }

    void SettingsDialog::OnMinimize()
    {
        ShowWindow(m_hDialog, SW_MINIMIZE);
    }

    void SettingsDialog::OnMaximize()
    {
        ShowWindow(m_hDialog, (GetWindowLong(m_hDialog, GWL_STYLE) & WS_MAXIMIZE) ? SW_RESTORE : SW_MAXIMIZE);
        UpdateMaximizeButtonIcon();
    }

    void SettingsDialog::UpdateMaximizeButtonIcon()
    {
        m_captionMaximizeButton.SetIcon(
            (GetWindowLong(m_hDialog, GWL_STYLE) & WS_MAXIMIZE) ? L"\xE923" : L"\xE922", true);
    }

    void SettingsDialog::OnClose()
    {
        EndDialog(m_hDialog, IDCANCEL);
    }

    void SettingsDialog::SaveSettings()
    {
        for (auto& page: m_pages)
        {
            page->SaveControls();
        }

        Config::Save();
    }

    void SettingsDialog::OpenSettingsPage()
    {
        SwitchActivePage(L"", &m_settingPageList, true);
        m_theme.SwapFocus(m_settingPageList.front().GetHWnd());
    }
}