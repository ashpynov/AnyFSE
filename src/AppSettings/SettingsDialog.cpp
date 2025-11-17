// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include "SettingsDialog.hpp"
#include "resource.h"
#include <commdlg.h>
#include <uxtheme.h>
#include "Tools/Tools.hpp"
#include "Tools/Unicode.hpp"
#include "Logging/LogManager.hpp"
#include "SettingsDialog.hpp"
#include "Configuration/Config.hpp"
#include "FluentDesign/Theme.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/TaskManager.hpp"
#include "Tools/Registry.hpp"

#define byte ::byte

#include "Tools/GdiPlus.hpp"


#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace AnyFSE::App::AppSettings::Settings
{
    Logger log = LogManager::GetLogger("Settings");

    INT_PTR SettingsDialog::Show(HINSTANCE hInstance)
    {
        size_t size = sizeof(DLGTEMPLATE) + sizeof(WORD) * 3; // menu, class, title
        HGLOBAL hGlobal = GlobalAlloc(GHND, size);

        LPDLGTEMPLATE dlgTemplate = (LPDLGTEMPLATE)GlobalLock(hGlobal);

        dlgTemplate->style = DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU;
        dlgTemplate->dwExtendedStyle = WS_EX_TOPMOST | WS_EX_WINDOWEDGE;
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
        // Get screen dimensions
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        int cx = m_theme.DpiScale(900);
        int cy = m_theme.DpiScale(720);

        // Calculate center position
        int x = (screenWidth - cx) / 2;
        int y = (screenHeight - cy) / 2;

        // Move dialog to center
        SetWindowPos(hwnd, NULL, x, y, cx, cy, SWP_NOZORDER);
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
                m_theme.Attach(hwnd);
                CenterDialog(hwnd);
                OnInitDialog(hwnd);
            }
            return TRUE;
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

                int captionHeight = m_theme.DpiScale(GetSystemMetrics(SM_CYCAPTION));
                // If mouse is in top-right area where buttons are, treat as client
                if (pt.y < captionHeight)
                {
                    SetWindowLong(m_hDialog, DWLP_MSGRESULT, HTCAPTION);
                    return TRUE;
                }

                SetWindowLong(m_hDialog, DWLP_MSGRESULT, hit);

            }
            return TRUE;
        case WM_NCACTIVATE:
            {
                BOOL bActive = (BOOL)wParam;
                m_captionCloseButton.SetColors(bActive ? Theme::Colors::Text : Theme::Colors::TextAccented);
            }
            return FALSE;

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

                r.left += m_theme.DpiScale(Layout_MarginLeft);
                r.top += m_theme.DpiScale(GetSystemMetrics(SM_CYCAPTION));

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
                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_removeButton.GetHwnd());
                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_captionBackButton.GetHwnd());
                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_captionCloseButton.GetHwnd());

                SelectObject(paint.MemDC(), oldFont);
            }
            return FALSE;

        case WM_DPICHANGED:
            UpdateDpiLayout();
            break;

        case WM_DESTROY:
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
            if (!m_pageName.empty() && !m_buttonMouseOver && Tools::MouseInClientRect(m_hDialog, &m_breadCrumbRect))
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
            if (!m_pageName.empty() && !m_buttonPressed && Tools::MouseInClientRect(m_hDialog, &m_breadCrumbRect))
            {
                m_buttonPressed = true;
                InvalidateRect(m_hDialog, &m_breadCrumbRect, TRUE);
                SetCapture(m_hDialog);
            }
            break;

        case WM_LBUTTONUP:
            if (m_buttonPressed && Tools::MouseInClientRect(m_hDialog, &m_breadCrumbRect))
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

    template<class T>
    FluentDesign::SettingsLine& SettingsDialog::AddSettingsLine(std::list<SettingsLine>& page,
        ULONG &top, const std::wstring& name, const std::wstring& desc, T& control,
        int height, int padding, int contentMargin, int contentWidth, int contentHeight )
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);
        int width = rect.right - rect.left
            - m_theme.DpiScale(Layout_MarginLeft)
            - m_theme.DpiScale(Layout_MarginRight);

        page.emplace_back(
            m_theme, m_hScrollView, name, desc,
            m_theme.DpiScale(Layout_MarginLeft),
            top, width,
            m_theme.DpiScale(height),
            padding,
            [&](HWND parent) { return control.Create(parent, 0, 0,
                m_theme.DpiScale(contentWidth), m_theme.DpiScale(contentHeight)); });

        top += m_theme.DpiScale(height + padding);
        if (contentMargin)
        {
            page.back().SetLeftMargin(contentMargin);
        }

        return page.back();
    }

    FluentDesign::SettingsLine& SettingsDialog::AddSettingsLine(
        std::list<SettingsLine>& page,
        ULONG &top, const std::wstring& name, const std::wstring& desc,
        int height, int padding, int contentMargin, int contentWidth, int contentHeight )
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);
        int width = rect.right - rect.left
            - m_theme.DpiScale(Layout_MarginLeft)
            - m_theme.DpiScale(Layout_MarginRight);


        page.emplace_back(
            m_theme, m_hScrollView, name, desc,
            m_theme.DpiScale(Layout_MarginLeft),
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

    void SettingsDialog::SwitchActivePage(std::list<SettingsLine> * pPageList, bool back)
    {
        if (pPageList == m_pActivePageList)
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
            Tools::MoveWindow(line.GetHWnd(), width * invert, 0, FALSE);
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
                    Tools::MoveWindow(line.GetHWnd(), -delta * invert, 0, TRUE);
                }
            }
            for (auto &line : *pPageList)
            {
                Tools::MoveWindow(line.GetHWnd(), -delta * invert, 0, TRUE);
            }
            RedrawWindow(m_hScrollView, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            Sleep(5);
        }

        for (auto &line : *m_pActivePageList)
        {
            ShowWindow(line.GetHWnd(), SW_HIDE);
            Tools::MoveWindow(line.GetHWnd(), width * invert, 0, FALSE);
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
            m_theme.DpiScale(Layout_BackButtonMargin * 2 + Layout_BackButtonSize),
            m_theme.DpiScale(Layout_BackButtonMargin),
            m_theme.DpiScale(200),
            m_theme.DpiScale(Layout_BackButtonSize - 4),
            FALSE);

        MoveWindow(m_captionBackButton.GetHwnd(),
            m_theme.DpiScale(Layout_BackButtonMargin),
            m_theme.DpiScale(Layout_BackButtonMargin),
            m_theme.DpiScale(Layout_BackButtonSize),
            m_theme.DpiScale(Layout_BackButtonSize),
            FALSE);

        MoveWindow(m_captionCloseButton.GetHwnd(),
            rect.right - m_theme.DpiScale(GetSystemMetrics(SM_CXSIZE)),
            -1,
            m_theme.DpiScale(GetSystemMetrics(SM_CXSIZE)),
            m_theme.DpiScale(GetSystemMetrics(SM_CYSIZE)),
            FALSE);

        RECT scrollRect {
            0,
            rect.top + m_theme.DpiScale(Layout_MarginTop + GetSystemMetrics(SM_CYCAPTION)),
            rect.right,
            rect.bottom
                - m_theme.DpiScale(Layout_MarginBottom)
                - m_theme.DpiScale(Layout_ButtonHeight)
                - m_theme.DpiScale(Layout_ButtonPadding)
            };

        Tools::MoveWindow(m_scrollView.GetHwnd(), &scrollRect, FALSE);

        UpdateLayout();

        GetClientRect(m_hDialog, &rect);

        rect.top = rect.bottom
            - m_theme.DpiScale(Layout_MarginBottom)
            - m_theme.DpiScale(Layout_ButtonHeight);

        rect.right -= m_theme.DpiScale(Layout_MarginRight);
        rect.left  += m_theme.DpiScale(Layout_MarginLeft);

        MoveWindow(m_removeButton.GetHwnd(),
            rect.left,
            rect.top,
            m_theme.DpiScale(Layout_UninstallWidth),
            m_theme.DpiScale(Layout_ButtonHeight),
            FALSE);

        MoveWindow(m_okButton.GetHwnd(),
            rect.right
                - m_theme.DpiScale(Layout_OKWidth)
                - m_theme.DpiScale(Layout_ButtonPadding)
                - m_theme.DpiScale(Layout_CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout_OKWidth),
            m_theme.DpiScale(Layout_ButtonHeight),
            FALSE);

        MoveWindow(m_closeButton.GetHwnd(),
            rect.right - m_theme.DpiScale(Layout_CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout_CloseWidth),
            m_theme.DpiScale(Layout_ButtonHeight),
            FALSE);

        InvalidateRect(m_hDialog, NULL, FALSE);
    }

    void SettingsDialog::UpdateLayout()
    {
        for (auto &page: { &m_customSettingPageList, &m_splashSettingPageList, &m_troubleshootSettingPageList })
        {
            if (page != m_pActivePageList)
            {
                for (auto& line: *page)
                {
                    ShowWindow(line.GetHWnd(), SW_HIDE);
                }
            }
        }

        RECT rect;
        GetClientRect(m_hDialog, &rect);
        int width = rect.right - rect.left
            - m_theme.DpiScale(Layout_MarginLeft)
            - m_theme.DpiScale(Layout_MarginRight);

        int left = m_theme.DpiScale(Layout_MarginLeft);

        ULONG top = -m_scrollView.GetScrollPos();

        for(auto& line : *m_pActivePageList)
        {
            line.UpdateLayout();
            ::MoveWindow(line.GetHWnd(), left, top, width, m_theme.DpiScale(line.GetDesignHeight()), FALSE);
            top += m_theme.DpiScale(line.GetDesignHeight() + line.GetDesignPadding());
        }

        m_scrollView.SetContentHeight(top + m_scrollView.GetScrollPos());
    }

    void SettingsDialog::AddCustomSettingsPage()
    {
    }

    void SettingsDialog::AddSplashSettingsPage()
    {
        ULONG top = 0;
        m_pSplashTextLine = &AddSettingsLine(m_splashSettingPageList, top,
            L"Show Loading text",
            L"",
            m_splashShowTextToggle,
            Layout_LineHeight, Layout_LinePaddingSmall, 0);

        m_pSplashTextLine->AddGroupItem(&AddSettingsLine(m_splashSettingPageList, top,
            L"Use Custom Text",
            L"",
            m_splashCustomTextEdit,
            Layout_LineHeightSmall, Layout_LinePadding, Layout_LineSmallMargin));

        m_pSplashLogoLine = &AddSettingsLine(m_splashSettingPageList, top,
            L"Show Home application Logo",
            L"",
            m_splashShowLogoToggle,
            Layout_LineHeight, Layout_LinePaddingSmall, 0);

        m_pSplashLogoLine->AddGroupItem(&AddSettingsLine(m_splashSettingPageList, top,
            L"Animate Logo",
            L"",
            m_splashShowAnimationToggle,
            Layout_LineHeightSmall, Layout_LinePadding, Layout_LineSmallMargin));

        m_pSplashVideoLine=&AddSettingsLine(m_splashSettingPageList, top,
            L"Show Video",
            L"",
            m_splashShowVideoToggle,
            Layout_LineHeight, Layout_LinePaddingSmall, 0);

        m_pSplashVideoLine->AddGroupItem(&AddSettingsLine(m_splashSettingPageList, top,
            L"Loop video",
            L"",
            m_splashShowVideoLoopToggle,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pSplashVideoLine->AddGroupItem(&AddSettingsLine(m_splashSettingPageList, top,
            L"Mute video",
            L"",
            m_splashShowVideoMuteToggle,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pSplashVideoLine->AddGroupItem(&AddSettingsLine(m_splashSettingPageList, top,
            L"Pause completed",
            L"Show last frame when video completed earlier than home app start",
            m_splashShowVideoPauseToggle,
            Layout_LineHeightSmall, Layout_LinePadding, Layout_LineSmallMargin));
    }

    void SettingsDialog::AddTroubleshootSettingsPage()
    {
        ULONG top = 0;
        FluentDesign::SettingsLine &logLevel = AddSettingsLine(m_troubleshootSettingPageList,
            top,
            L"Log level",
            L"Enable logs at specified level",
            m_troubleLogLevelCombo,
            Layout_LineHeight, Layout_LinePadding, 0,
            Layout_LauncherComboWidth);

        for (int i = (int)LogLevels::Disabled; i < (int)LogLevels::Max; i++)
        {
            std::wstring level = Unicode::to_wstring(LogManager::LogLevelToString((LogLevels)i));
            wchar_t buff[2] = {(wchar_t)i, 0};
            m_troubleLogLevelCombo.AddItem(level, L"", buff);
        }

        AddSettingsLine(m_troubleshootSettingPageList,
            top,
            L"Agressive Mode",
            L"Will react on XboxApp startup, with simple and robust logic, but you will lose any manual access of the XboxApp.",
            m_troubleAgressiveToggle,
            Layout_LineHeight, Layout_LinePadding, 0);
    }

    void SettingsDialog::OnInitDialog(HWND hwnd)
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);

        m_captionBackButton.Create(m_hDialog,
            m_theme.DpiScale(Layout_BackButtonMargin),
            m_theme.DpiScale(Layout_BackButtonMargin),
            m_theme.DpiScale(Layout_BackButtonSize),
            m_theme.DpiScale(Layout_BackButtonSize));

        m_captionStatic.Create(m_hDialog,
            m_theme.DpiScale(Layout_BackButtonMargin * 2 + Layout_BackButtonSize),
            m_theme.DpiScale(Layout_BackButtonMargin),
            m_theme.DpiScale(200),
            m_theme.DpiScale(Layout_BackButtonSize - 4));
        m_captionStatic.Format().SetLineAlignment(Gdiplus::StringAlignment::StringAlignmentCenter);

        m_captionStatic.SetText(L"Settings");

        m_captionBackButton.SetIcon(L"\xE64E");
        m_captionBackButton.OnChanged += delegate(OpenSettingsPage);
        m_captionBackButton.SetFlat(true);
        m_captionBackButton.Enable(false);

        m_captionCloseButton.Create(m_hDialog,
            rect.right - m_theme.DpiScale(GetSystemMetrics(SM_CXSIZE)),
            -1,
            m_theme.DpiScale(GetSystemMetrics(SM_CXSIZE)),
            m_theme.DpiScale(GetSystemMetrics(SM_CYSIZE)));

        SetWindowLong(m_captionCloseButton.GetHwnd(), GWL_STYLE,
            GetWindowLong(m_captionCloseButton.GetHwnd(), GWL_STYLE) & ~WS_TABSTOP );

        m_captionCloseButton.SetIcon(L"\xE106");
        m_captionCloseButton.OnChanged += delegate(OnClose);
        m_captionCloseButton.SetFlat(true);
        m_captionCloseButton.SetSquare(true);

        m_captionCloseButton.SetColors(
            Theme::Colors::Default, Theme::Colors::Default,
            Theme::Colors::Default, Theme::Colors::CloseButtonHover,
            Theme::Colors::Default, Theme::Colors::CloseButtonPressed);

        m_hScrollView = m_scrollView.Create(m_hDialog,
            0,
            rect.top + m_theme.DpiScale(Layout_MarginTop + GetSystemMetrics(SM_CYCAPTION)),
            rect.right,
            rect.bottom
                - m_theme.DpiScale(Layout_MarginBottom)
                - m_theme.DpiScale(Layout_MarginTop + GetSystemMetrics(SM_CYCAPTION))
                - m_theme.DpiScale(Layout_ButtonHeight)
                - m_theme.DpiScale(Layout_ButtonPadding)
        );



        ULONG top = 0;

        FluentDesign::SettingsLine &launcher = AddSettingsLine(m_settingPageList, top,
            L"Choose home app",
            L"Choose home application for full screen experience",
            m_launcherCombo,
            Layout_LineHeight, Layout_LauncherBrowsePadding, 0,
            Layout_LauncherComboWidth );

        launcher.SetFrame(Gdiplus::FrameFlags::SIDE_NO_BOTTOM | Gdiplus::FrameFlags::CORNER_TOP);

        FluentDesign::SettingsLine &browse = AddSettingsLine(m_settingPageList, top,
            L"",
            L"",
            m_browseButton,
            Layout_LauncherBrowseLineHeight, Layout_LinePadding, 0,
            Layout_BrowseWidth, Layout_BrowseHeight);

        browse.SetFrame(Gdiplus::FrameFlags::SIDE_NO_TOP | Gdiplus::FrameFlags::CORNER_BOTTOM);

        m_pFseOnStartupLine = &AddSettingsLine(m_settingPageList, top,
            L"Enter full screen experience on startup",
            L"",
            m_fseOnStartupToggle,
            Layout_LineHeight, Layout_LinePadding, 0);

        m_pCustomSettingsLine = &AddSettingsLine(m_settingPageList, top,
            L"Use custom settings",
            L"Change monitoring and startups settings for selected home application",
            m_customSettingsToggle,
            Layout_LineHeight, Layout_LinePaddingSmall, 0);

        m_pCustomSettingsLine->SetIcon(L'\xE115');

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(m_settingPageList, top,
            L"Additional arguments",
            L"Command line arguments passed to application",
            m_additionalArgumentsEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(m_settingPageList, top,
            L"Primary process name",
            L"Name of home application process",
            m_processNameEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(m_settingPageList, top,
            L"Primary window title",
            L"Title of app window when it have been activated",
            m_titleEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(m_settingPageList, top,
            L"Secondary process name",
            L"Name of app process for alternative mode",
            m_processNameAltEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(m_settingPageList, top,
            L"Secondary window title",
            L"Title of app window when it alternative mode have been activated",
            m_titleAltEdit,
            Layout_LineHeightSmall, Layout_LinePadding, Layout_LineSmallMargin));

        m_pSplashPageLine = &AddSettingsLine(m_settingPageList, top,
            L"Splash screen settings",
            L"Configure Look'n'Feel of splash screen during home app loading",
            Layout_LineHeight, Layout_LinePadding, 0);

        m_pSplashPageLine->SetState(FluentDesign::SettingsLine::Next);
        m_pSplashPageLine->SetIcon(L'\xEB9F');
        m_pSplashPageLine->OnChanged += delegate(OpenSplashSettingsPage);

        AddSplashSettingsPage();

        m_pTroubleshootPageLine = &AddSettingsLine(m_settingPageList, top,
            L"Debug and Troubleshoot",
            L"Configure logs and advanced parameters",
            Layout_LineHeight, Layout_LinePadding, 0);

        m_pTroubleshootPageLine->SetState(FluentDesign::SettingsLine::Next);
        m_pTroubleshootPageLine->SetIcon(L'\xEBE8');
        m_pTroubleshootPageLine->OnChanged += delegate(OpenTroubleshootSettingsPage);

        AddTroubleshootSettingsPage();

        m_pActivePageList = &m_settingPageList;
        UpdateLayout();

        GetClientRect(m_hDialog, &rect);
        rect.top = rect.bottom
            - m_theme.DpiScale(Layout_MarginBottom)
            - m_theme.DpiScale(Layout_ButtonHeight);

        rect.right -= m_theme.DpiScale(Layout_MarginRight);
        rect.left  += m_theme.DpiScale(Layout_MarginLeft);


        m_removeButton.Create(m_hDialog,
            rect.left,
            rect.top,
            m_theme.DpiScale(Layout_UninstallWidth),
            m_theme.DpiScale(Layout_ButtonHeight));

        m_removeButton.SetText(L"Uninstall");

        m_removeButton.OnChanged += delegate(OnUninstall);

        m_okButton.Create(m_hDialog,
            rect.right
                - m_theme.DpiScale(Layout_OKWidth)
                - m_theme.DpiScale(Layout_ButtonPadding)
                - m_theme.DpiScale(Layout_CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout_OKWidth),
            m_theme.DpiScale(Layout_ButtonHeight));

        m_okButton.SetText(L"Save");

        m_okButton.OnChanged += delegate(OnOk);

        m_closeButton.Create(m_hDialog,
            rect.right - m_theme.DpiScale(Layout_CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout_CloseWidth),
            m_theme.DpiScale(Layout_ButtonHeight));

        m_closeButton.OnChanged += delegate(OnClose);

        m_closeButton.SetText(L"Discard and Close");

        m_launcherCombo.OnChanged += delegate(OnLauncherChanged);

        m_customSettingsToggle.OnChanged += delegate(OnCustomChanged);
        m_pCustomSettingsLine->OnChanged += delegate(OnCustomLineChanged);

        m_splashShowTextToggle.OnChanged += delegate(OnShowTextChanged);
        m_pSplashTextLine->OnChanged += delegate(UpdateLayout);

        m_splashShowLogoToggle.OnChanged += delegate(OnShowLogoChanged);
        m_pSplashLogoLine->OnChanged += delegate(UpdateLayout);

        m_splashShowVideoToggle.OnChanged += delegate(OnShowVideoChanged);
        m_pSplashVideoLine->OnChanged += delegate(UpdateLayout);

        m_browseButton.SetText(L"Browse");
        m_browseButton.OnChanged += delegate(OnBrowseLauncher);

        m_currentLauncherPath = Config::Launcher.StartCommand;
        Config::FindLaunchers(m_launchersList);
        UpdateCombo();
        Config::LoadLauncherSettings(m_currentLauncherPath, m_config);

        bool customSettings = Config::CustomSettings;
        m_customSettingsState = customSettings ? FluentDesign::SettingsLine::Opened: FluentDesign::SettingsLine::Closed;

        m_isCustom = (customSettings || m_config.IsCustom) && (m_config.Type != LauncherType::Xbox && m_config.Type != LauncherType::ArmouryCrate) || m_config.Type == LauncherType::Custom;
        m_isAgressive = Config::AggressiveMode && m_config.Type != LauncherType::Xbox;

        UpdateControls();
        UpdateCustomSettings();
        m_customSettingsState = m_pCustomSettingsLine->GetState() != FluentDesign::SettingsLine::Normal
                                ? m_pCustomSettingsLine->GetState()
                                : FluentDesign::SettingsLine::Opened;
    }

    void SettingsDialog::ShowGroup(int groupIdx, bool show)
    {
        // To do
    }

    void SettingsDialog::OnBrowseLauncher()
    {
        OPENFILENAME ofn = {};
        WCHAR szFile[260] = {};

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hDialog;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L"Launchers (*.exe)\0*.exe\0\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn))
        {
            if (m_currentLauncherPath != szFile)
            {
                m_currentLauncherPath = szFile;
                Config::LoadLauncherSettings(m_currentLauncherPath, m_config);
                UpdateCombo();
                UpdateControls();
                UpdateCustomSettings();
            }
        }
    }

    void SettingsDialog::OnOk()
    {
        SaveSettings();
        EndDialog(m_hDialog, IDOK);
    }

    void SettingsDialog::OnClose()
    {
        EndDialog(m_hDialog, IDCANCEL);
    }

    void SettingsDialog::InitCustomControls()
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_STANDARD_CLASSES;
        ::InitCommonControlsEx(&icex);
    }

    void SettingsDialog::OnUninstall()
    {
        // uninstall
        InitCustomControls();
        int nButtonClicked = 0;
        if (SUCCEEDED(TaskDialog(m_hDialog, GetModuleHandle(NULL),
                       L"Uninstall",
                       L"Uninstall Any Fullscreen Experience?",
                       L"This will remove system task and all settings\n",
                       TDCBF_YES_BUTTON | TDCBF_CANCEL_BUTTON, TD_INFORMATION_ICON, &nButtonClicked))
            && nButtonClicked == IDYES)

        {
            //Registry::DeleteKey(L"HKCU\\Software\\AnyFSE");
            TaskManager::RemoveTask();
            EndDialog(m_hDialog, IDABORT);
        }
    }

    void SettingsDialog::OnShowTextChanged()
    {
        m_pSplashTextLine->SetState(m_splashShowTextToggle.GetCheck()
            ? FluentDesign::SettingsLine::Opened
            : FluentDesign::SettingsLine::Normal);

        UpdateLayout();
    }

    void SettingsDialog::OnShowLogoChanged()
    {
        m_pSplashLogoLine->SetState(m_splashShowLogoToggle.GetCheck()
            ? FluentDesign::SettingsLine::Opened
            : FluentDesign::SettingsLine::Normal);

        UpdateLayout();
    }

    void SettingsDialog::OnShowVideoChanged()
    {
        m_pSplashVideoLine->SetState(m_splashShowVideoToggle.GetCheck()
            ? FluentDesign::SettingsLine::Opened
            : FluentDesign::SettingsLine::Normal);

        UpdateLayout();
    }

    void SettingsDialog::OnCustomChanged()
    {
        m_isCustom = m_customSettingsToggle.GetCheck();
        UpdateControls();
        UpdateLayout();
        RedrawWindow(m_hDialog, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }

    void SettingsDialog::OnCustomLineChanged()
    {
        m_customSettingsState = m_pCustomSettingsLine->GetState();
        UpdateLayout();
        RedrawWindow(m_hDialog, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }

    void SettingsDialog::OpenSettingsPage()
    {
        m_pageName = L"";
        SwitchActivePage(&m_settingPageList, true);
    }

    void SettingsDialog::OpenCustomSettingsPage()
    {
        m_pageName = L"Custom settings";
        SwitchActivePage(&m_customSettingPageList);
    }

    void SettingsDialog::OpenSplashSettingsPage()
    {
        m_pageName = L"Splash settings";
        SwitchActivePage(&m_splashSettingPageList);
    }

    void SettingsDialog::OpenTroubleshootSettingsPage()
    {
        m_pageName = L"Troubleshoot";
        SwitchActivePage(&m_troubleshootSettingPageList);
    }

    void SettingsDialog::OnAggressiveChanged()
    {
        // m_isAgressive = BST_CHECKED != SendDlgItemMessage(m_hDialog, IDC_NOT_AGGRESSIVE_MODE, BM_GETCHECK, 0, 0);
        // UpdateCustom();
    }

    void SettingsDialog::UpdateControls()
    {
        LauncherConfig defaults;
        Config::GetLauncherDefaults(m_currentLauncherPath, defaults);

        if (defaults.Type == LauncherType::None)
        {
            m_pFseOnStartupLine->Enable(false);
            m_fseOnStartupToggle.SetCheck(false);
        }
        else
        {
            m_pFseOnStartupLine->Enable();
            m_fseOnStartupToggle.SetCheck(Config::FseOnStartup);
        }

        bool alwaysSettings = defaults.Type==LauncherType::Custom;
        bool noSettings = defaults.Type == LauncherType::Xbox ||  defaults.Type == LauncherType::ArmouryCrate || defaults.Type == LauncherType::None;

        bool haveSettings = m_isCustom && !noSettings || alwaysSettings;
        bool enableCheck = !alwaysSettings && !noSettings;

        m_customSettingsToggle.SetCheck(haveSettings);

        FluentDesign::SettingsLine::State state = haveSettings
                ? alwaysSettings
                    ? FluentDesign::SettingsLine::Opened
                    : m_customSettingsState
                : FluentDesign::SettingsLine::Normal;

        m_pCustomSettingsLine->SetState(state);
        m_pCustomSettingsLine->Enable(enableCheck);

        if (!haveSettings)
        {
            m_config = defaults;
            UpdateCustomSettings();
        }

        m_troubleLogLevelCombo.SelectItem(min(max((int)LogLevels::Disabled, (int)Config::LogLevel), (int)LogLevels::Max));
        m_troubleAgressiveToggle.SetCheck(Config::AggressiveMode);

        m_splashShowTextToggle.SetCheck(Config::SplashShowText);
        m_splashCustomTextEdit.SetText(Config::SplashCustomText);

        m_splashShowLogoToggle.SetCheck(Config::SplashShowLogo);
        m_splashShowAnimationToggle.SetCheck(Config::SplashShowAnimation);

        m_splashShowVideoToggle.SetCheck(Config::SplashShowVideo);
        m_splashShowVideoLoopToggle.SetCheck(Config::SplashVideoLoop);
        m_splashShowVideoMuteToggle.SetCheck(Config::SplashVideoMute);
        m_splashShowVideoPauseToggle.SetCheck(Config::SplashVideoPause);
    }

    void SettingsDialog::OnLauncherChanged()
    {
        m_currentLauncherPath = m_launcherCombo.GetCurentValue();;
        Config::LoadLauncherSettings(m_currentLauncherPath, m_config);
        UpdateControls();
        UpdateCustomSettings();
    }


    void SettingsDialog::UpdateCombo()
    {
        m_launcherCombo.Reset();

        for ( auto& launcher: m_launchersList)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(launcher, info);
            Config::UpdatePortableLauncher(info);
            m_launcherCombo.AddItem(info.Name, info.IconFile, info.StartCommand);
        }
        size_t index = Tools::index_of(m_launchersList, m_currentLauncherPath);

        if (index == Tools::npos)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(m_currentLauncherPath, info);
            Config::UpdatePortableLauncher(info);
            m_launcherCombo.AddItem(info.Name, info.IconFile, info.StartCommand, 1);
            index = 1;
        }

        m_launcherCombo.SelectItem((int)index);
    }

    void SettingsDialog::UpdateCustomSettings()
    {
        // Set values
        m_additionalArgumentsEdit.SetText(m_config.StartArg);
        m_processNameEdit.SetText(m_config.ProcessName);
        m_processNameAltEdit.SetText(m_config.ProcessNameAlt);
        m_titleEdit.SetText(m_config.WindowTitle);
        m_titleAltEdit.SetText(m_config.WindowTitleAlt);

        // SendDlgItemMessage(m_hDialog, IDC_NOT_AGGRESSIVE_MODE, BM_SETCHECK, (WPARAM)(m_isAgressive && config.Type != LauncherType::Xbox) ? BST_UNCHECKED : BST_CHECKED, 0 );
        // EnableWindow(GetDlgItem(m_hDialog, IDC_NOT_AGGRESSIVE_MODE), config.Type != LauncherType::Xbox);
    }
    void SettingsDialog::SaveSettings()
    {
        const std::wstring gamingConfiguration = L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration";
        const std::wstring startupToGamingHome = L"StartupToGamingHome";
        const std::wstring gamingHomeApp = L"GamingHomeApp";
        const std::wstring xboxApp = L"Microsoft.GamingApp_8wekyb3d8bbwe!Microsoft.Xbox.App";


        bool removeAnyFSE = false;

        if (m_config.Type == LauncherType::None)
        {
            Registry::DeleteValue(gamingConfiguration, gamingHomeApp);
            Registry::WriteBool(gamingConfiguration, startupToGamingHome, false);
            removeAnyFSE = true;
        }
        else
        {
            Registry::WriteBool(gamingConfiguration, startupToGamingHome, m_fseOnStartupToggle.GetCheck());
            Registry::WriteString(gamingConfiguration, gamingHomeApp, xboxApp);

            if (m_config.Type == LauncherType::Xbox)
            {
                removeAnyFSE = true;
            }
            else
            {
                // save FSE
            }
        }

        Config::Launcher.StartCommand = m_config.StartCommand;
        Config::CustomSettings = m_customSettingsToggle.GetCheck();
        Config::CustomSettings = m_customSettingsToggle.GetCheck();
        Config::Launcher.StartArg = m_additionalArgumentsEdit.GetText();
        Config::Launcher.ProcessName = m_processNameEdit.GetText();
        Config::Launcher.WindowTitle = m_titleEdit.GetText();
        Config::Launcher.ProcessNameAlt = m_processNameAltEdit.GetText();
        Config::Launcher.WindowTitleAlt = m_titleAltEdit.GetText();
        Config::Launcher.IconFile = m_config.IconFile;


        Config::SplashShowText = m_splashShowTextToggle.GetCheck();
        Config::SplashCustomText = m_splashCustomTextEdit.GetText();

        Config::SplashShowLogo = m_splashShowLogoToggle.GetCheck();
        Config::SplashShowAnimation = m_splashShowAnimationToggle.GetCheck();

        Config::SplashShowVideo = m_splashShowVideoToggle.GetCheck();
        Config::SplashVideoLoop = m_splashShowVideoLoopToggle.GetCheck();
        Config::SplashVideoMute = m_splashShowVideoMuteToggle.GetCheck();
        Config::SplashVideoPause = m_splashShowVideoPauseToggle.GetCheck();

        Config::LogLevel = (LogLevels)m_troubleLogLevelCombo.GetSelectedIndex();
        Config::AggressiveMode = m_troubleAgressiveToggle.GetCheck();

        Config::Save();

        if (removeAnyFSE)
        {
            TaskManager::RemoveTask();
        }
        else
        {
            //create app start
            TaskManager::CreateTask();
        }
    }
}