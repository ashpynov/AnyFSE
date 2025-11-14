// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include "Theme.hpp"
#include "Tools/GdiPlus.hpp"
#include "Tools/Tools.hpp"
#include "Logging/LogManager.hpp"
#include <dwmapi.h>
#include <commctrl.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Gdiplus.lib")

namespace FluentDesign
{
    static Logger log = LogManager::GetLogger("Theme");

    Theme::Theme(HWND hParentWnd)
        : m_hParentWnd(NULL)
        , m_dpi(96)
        , m_isDark(false)
        , m_isKeyboardFocus(false)
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        LoadColors();
        if (hParentWnd)
        {
            Attach(hParentWnd);
        }
        else
        {
            CreateFonts();
        }
    }

    void Theme::Attach(HWND hHostWnd)
    {
        m_lastFocused = GetFocus();
        m_isKeyboardFocus = false;
        m_isDark = false;
        m_hParentWnd = hHostWnd;
        m_dpi = GetDpiForWindow(m_hParentWnd);
        LoadColors();
        CreateFonts();


        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        SetWindowSubclass(hHostWnd, DialogSubclassProc, 0, (DWORD_PTR)this);

    }

    void Theme::RegisterChild(HWND hChild)
    {
        m_childsList.push_back(hChild);
        SetWindowSubclass(hChild, ControlSublassProc, 0, (DWORD_PTR)this);
        SetWindowSubclass(GetParent(hChild), ControlParentSublassProc, 0, (DWORD_PTR)this);
    }

    static void Invalidate(HWND hwnd, BOOL bErase = FALSE)
    {
        InvalidateRect(GetParent(hwnd), NULL, bErase);
        InvalidateRect(hwnd, NULL, bErase);
    }

    LRESULT CALLBACK Theme::ControlParentSublassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        Theme *This = reinterpret_cast<Theme *>(dwRefData);

        switch (msg)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                if (This->m_isKeyboardFocus)
                {
                    This->m_isKeyboardFocus = false;
                    for( auto &c : This->m_childsList)
                    {
                        Invalidate(c, FALSE);
                    }
                }
                break;
            }
        case WM_DESTROY:
            RemoveWindowSubclass(hWnd, ControlParentSublassProc, uIdSubclass);
            break;
        }
        return DefSubclassProc(hWnd, msg, wParam, lParam);
    }

    void Theme::SendNotifyFocus(HWND hWnd)
    {
        NMHDR nmhdr = {};
        nmhdr.hwndFrom = hWnd;
        nmhdr.idFrom = GetDlgCtrlID(hWnd);
        nmhdr.code = NM_SETFOCUS;

        SendMessage(m_hParentWnd, WM_NOTIFY, (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
    }

    LRESULT CALLBACK Theme::ControlSublassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        Theme *This = reinterpret_cast<Theme *>(dwRefData);

        switch (msg)
        {
        case WM_SETFOCUS:
            {
                HWND last = This->m_lastFocused;
                HWND next = hWnd;
                HWND prev = (HWND) wParam;

                if (last && last != next && !This->m_isKeyboardFocus)
                {
                    This->m_isKeyboardFocus = true;
                    SetFocus(last);
                    This->SendNotifyFocus(last);
                    Invalidate(last);
                }
                else if (last && last != next)
                {
                    This->m_isKeyboardFocus = true;
                    This->m_lastFocused = next;
                }
                else if (GetParent(next) != prev || !last)
                {
                    This->m_lastFocused = next;
                }
                Invalidate(next);
                This->SendNotifyFocus(next);
            }
            break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                This->m_lastFocused = hWnd;
                This->m_isKeyboardFocus = false;
                SetFocus(hWnd);
                This->SendNotifyFocus(hWnd);
                Invalidate(hWnd);
                break;
            }
        case WM_KILLFOCUS:
            {
                Invalidate(hWnd, TRUE);
            }
            break;
        case WM_DESTROY:
            RemoveWindowSubclass(hWnd, ControlSublassProc, uIdSubclass);
            break;

        case WM_NOTIFY:
            {
                if (((LPNMHDR)lParam)->code == NM_SETFOCUS)
                {
                    This->SendNotifyFocus(hWnd);
                }
            }
            break;
        }
        return DefSubclassProc(hWnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK Theme::DialogSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        Theme *This = reinterpret_cast<Theme *>(dwRefData);

        switch (msg)
        {
            case WM_SETTINGCHANGE:
            {
                if (lParam != NULL && wcscmp(L"ImmersiveColorSet", (LPCWSTR)lParam) == 0)
                {
                    This->LoadColors();
                    RedrawWindow(hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
                }
            }
            break;
            case WM_DWMCOLORIZATIONCOLORCHANGED:
            {
                This->LoadColors();
                RedrawWindow(hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
                break;
            }
            case WM_DPICHANGED:
            {
                ULONG oldDPI = This->m_dpi;
                This->m_dpi = HIWORD(wParam);
                This->CreateFonts();
                RECT* const prcNewWindow = (RECT*)lParam;
                SetWindowPos(hWnd,
                    NULL,
                    prcNewWindow->left,
                    prcNewWindow->top,
                    prcNewWindow->right - prcNewWindow->left,
                    prcNewWindow->bottom - prcNewWindow->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);

                This->OnDPIChanged.Notify();
            }
            break;
            case WM_CTLCOLORDLG:
            {
                static HBRUSH hBlackBrush = CreateSolidBrush(This->GetColorRef(FluentDesign::Theme::Colors::Dialog));
                return (LRESULT)hBlackBrush;
            }

            case WM_ERASEBKGND:
            {
                HWND child = (HWND)lParam;
                if(child && !DefSubclassProc(hWnd, msg, wParam, lParam))
                {
                    RECT childRect;
                    GetClientRect(child, &childRect);
                    HBRUSH hBrush = CreateSolidBrush(This->GetColorRef(Theme::Colors::Dialog));
                    FillRect((HDC)wParam, &childRect, hBrush);
                    DeleteObject(hBrush);

                    if ( child == GetFocus())
                    {
                        This->DrawChildFocus((HDC)wParam, child, child);
                    }
                }
            }
            return 1;

            case WM_DESTROY:
                RemoveWindowSubclass(hWnd, DialogSubclassProc, uIdSubclass);
                break;
        }
        return DefSubclassProc(hWnd, msg, wParam, lParam);
    }

    bool Theme::SetKeyboardFocused(HWND hwnd)
    {
        if (hwnd && !m_isKeyboardFocus)
        {
            m_isKeyboardFocus = true;
            Invalidate(hwnd, FALSE);
        }
        else if (!hwnd && m_isKeyboardFocus)
        {
            m_isKeyboardFocus = false;
            Invalidate(hwnd, FALSE);
        }
        return false;
    }

    Theme::~Theme()
    {
        // TODO: unsubclass parent wnd
        FreeFonts();
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
    };

    float Theme::FocusOffsetByWndClass(HWND hWnd)
    {
        WCHAR className[MAX_PATH + 1];
        GetClassNameW(hWnd, className, MAX_PATH);
        if (!_wcsicmp(className, L"STATIC") || !_wcsicmp(className, L"EDIT"))
        {
            return -1;
        }
        else if (!_wcsicmp(className, L"BUTTON"))
        {
            WCHAR name[MAX_PATH + 1];
            GetWindowTextW(hWnd, name, MAX_PATH);
            if (!_wcsicmp(name, L"TOGGLE"))
            {
                return DpiScaleF(m_focusToggleMargins);
            }
        }

        return (float)GetSize_FocusMargins();
    }

    void Theme::DrawFocusFrame(HDC hdc, const RECT &clientRect, float offset)
    {
        if (!m_isKeyboardFocus)
            return;

        using namespace Gdiplus;
        Graphics graphics(hdc);

        // Enable anti-aliasing for smooth edges
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        RectF focusRect = ToRectF(clientRect);
        focusRect.Inflate(offset, offset);

        Pen borderPen(GetColor(FluentDesign::Theme::Colors::FocusFrame), (REAL)GetSize_FocusWidth());

        // For rounded rectangles in GDI+, we need to use GraphicsPath
        Gdiplus::RoundRect(graphics, focusRect, (REAL)GetSize_FocusCornerSize() + offset, nullptr, borderPen);

    }

    void Theme::DrawChildFocus(HDC hdc, HWND parent, HWND child)
    {
        if (!child || !m_isKeyboardFocus || GetFocus() != child) return;

        float offset = FocusOffsetByWndClass(child);
        if (offset < 0 ) return;

        RECT clientRect;
        GetClientRect(child, &clientRect);
        MapWindowPoints(child, parent, (LPPOINT)&clientRect, 2);

        DrawFocusFrame(hdc, clientRect, offset);
    }


    void Theme::CreateFonts()
    {
        FreeFonts();

                // Get system font
        NONCLIENTMETRICS ncm = {};
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);

        // Create name font (slightly larger/bolder)
        //m_hPrimaryFont = CreateFontIndirect(&ncm.lfMessageFont);

        LOGFONT primaryFont = ncm.lfMessageFont;
        primaryFont.lfHeight = (LONG)(-GetSize_Text());
        primaryFont.lfWeight = FW_NORMAL;
        m_hPrimaryFont = CreateFontIndirect(&primaryFont);

        primaryFont.lfWeight = FW_BOLD;
        m_hPrimaryBoldFont = CreateFontIndirect(&primaryFont);

        primaryFont.lfHeight = (LONG)(-GetSize_TextLarge());
        primaryFont.lfWeight = FW_NORMAL;
        m_hLargeFont = CreateFontIndirect(&primaryFont);


        // Create description font (slightly smaller, normal weight)
        LOGFONT secondaryFont = ncm.lfMessageFont;
        secondaryFont.lfHeight = (LONG)(-GetSize_TextSecondary());
        m_hSecondaryFont = CreateFontIndirect(&secondaryFont);

        m_hGlyphFont = CreateFont(
            GetSize_Glyph(), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH,
            L"Segoe MDL2 Assets");

        m_hGlyphNormalFont = CreateFont(
            GetSize_Text(), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH,
            L"Segoe MDL2 Assets");

        m_hIconFont = CreateFont(
            GetSize_Icon(), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH,
            L"Segoe Fluent Icons");

        LOGFONT titleFont = ncm.lfMessageFont;
        titleFont.lfHeight = (LONG)(-GetSize_Title());
        titleFont.lfWeight = FW_BOLD;
        m_hTitleFont = CreateFontIndirect(&titleFont);
    }

    void Theme::FreeFonts()
    {
        if (m_hPrimaryFont)
            DeleteObject(m_hPrimaryFont);

        if (m_hLargeFont)
            DeleteObject(m_hLargeFont);

        if (m_hPrimaryBoldFont)
            DeleteObject(m_hPrimaryBoldFont);

        if (m_hSecondaryFont)
            DeleteObject(m_hSecondaryFont);

        if (m_hGlyphFont)
            DeleteObject(m_hGlyphFont);

        if (m_hGlyphNormalFont)
            DeleteObject(m_hGlyphNormalFont);

        if (m_hTitleFont)
            DeleteObject(m_hTitleFont);

        if (m_hIconFont)
            DeleteObject(m_hIconFont);

        m_hPrimaryFont = NULL;
        m_hLargeFont = NULL;
        m_hPrimaryBoldFont = NULL;
        m_hSecondaryFont = NULL;
        m_hGlyphFont = NULL;
        m_hTitleFont = NULL;
        m_hGlyphNormalFont = NULL;
        m_hIconFont = NULL;
    }

    const int Theme::DpiUnscale(int scaledSize)
    {
        return MulDiv(scaledSize, 96, m_dpi);
    }

    const float Theme::DpiUnscaleF(int scaledSize)
    {
        return DpiUnscaleF((float)scaledSize);
    }

    const float Theme::DpiUnscaleF(float scaledSize)
    {
        return scaledSize * 96 / m_dpi;
    }

    const int Theme::DpiScale(int designSize)
    {
        return MulDiv(designSize, m_dpi, 96);
    }

    const float Theme::DpiScaleF(int designSize)
    {
        return DpiScaleF((float)designSize);
    };

    const float Theme::DpiScaleF(float designSize)
    {
        return designSize * m_dpi / 96;
    }
    const Gdiplus::RectF Theme::DpiUnscaleF(const RECT& scaledRect)
    {
        return Gdiplus::RectF(
            DpiUnscaleF(scaledRect.left),
            DpiUnscaleF(scaledRect.top),
            DpiUnscaleF(scaledRect.right - scaledRect.left),
            DpiUnscaleF(scaledRect.bottom - scaledRect.top)
        );
    }

    const RECT Theme::DpiScale(const Gdiplus::RectF &designRect)
    {
        return RECT{
            (LONG)DpiScaleF(designRect.X),
            (LONG)DpiScaleF(designRect.Y),
            (LONG)DpiScaleF(designRect.GetRight()),
            (LONG)DpiScaleF(designRect.GetBottom()),
        };
    }

    const void Theme::DpiUnscaleChilds(HWND parent, std::map<HWND, Gdiplus::RectF> &storage)
    {
        HWND hChild = GetWindow(parent, GW_CHILD);
        while (hChild)
        {
            RECT rc;
            Tools::GetChildRect(hChild, &rc);
            storage[hChild] = DpiUnscaleF(rc);
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
    }

    const void Theme::DpiScaleChilds(HWND parent, const std::map<HWND, Gdiplus::RectF> &storage)
    {
        HWND hChild = GetWindow(parent, GW_CHILD);
        while (hChild)
        {
            RECT rc;
            Tools::GetChildRect(hChild, &rc);
            auto st = storage.find(hChild);
            if (st != storage.end())
            {
                RECT rc = DpiScale(st->second);
                MoveWindow(hChild, &rc, FALSE);
            }
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
    };
}