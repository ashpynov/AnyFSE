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


#include "Theme.hpp"
#include "Tools/GdiPlus.hpp"
#include "Tools/Window.hpp"
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
        , m_swappingFocus(false)
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        LoadColors();
        if (hParentWnd)
        {
            AttachDlg(hParentWnd);
        }
        else
        {
            CreateFonts();
        }
    }

    void Theme::AttachWindow(HWND hHostWnd)
    {
        m_lastFocused = GetFocus();
        m_isKeyboardFocus = false;
        m_isDark = false;
        m_hParentWnd = hHostWnd;
        m_dpi = GetDpiForWindow(m_hParentWnd);
        LoadColors();
        CreateFonts();
    }

    void Theme::AttachDlg(HWND hHostWnd)
    {
        AttachWindow(hHostWnd);

        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        SetWindowSubclass(hHostWnd, DialogSubclassProc, 0, (DWORD_PTR)this);
    }

    void Theme::RegisterChild(HWND hChild)
    {
        m_childsList.push_back(hChild);
        SetWindowSubclass(hChild, ControlSublassProc, 1, (DWORD_PTR)this);
        SetWindowSubclass(GetParent(hChild), ControlParentSublassProc, 1, (DWORD_PTR)this);
    }

    static void Invalidate(HWND hwnd, BOOL bErase = FALSE)
    {
        RedrawWindow(GetParent(hwnd), NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE);
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
    void Theme::SwapFocus(HWND hWnd)
    {
        m_swappingFocus = true;
        SetFocus(hWnd);
        m_swappingFocus = false;
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

                if (This->m_swappingFocus)
                {
                    This->m_lastFocused = next;
                }
                else if (last && last != next && !This->m_isKeyboardFocus)
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
                HWND lastFocused = This->m_lastFocused;
                This->m_lastFocused = hWnd;
                This->m_isKeyboardFocus = false;
                SetFocus(hWnd);
                This->SendNotifyFocus(hWnd);
                Invalidate(lastFocused);
                Invalidate(hWnd);
                return 0;
            }
        case WM_KILLFOCUS:
            {
                Invalidate(hWnd, TRUE);
            }
            break;
        case WM_DESTROY:
            RemoveWindowSubclass(hWnd, ControlSublassProc, uIdSubclass);
            break;

        case WM_KEYDOWN:
        {
            if (wParam == VK_LEFT  || wParam == VK_UP
                || wParam == VK_RIGHT || wParam == VK_DOWN)
            {
                LRESULT ret = DefSubclassProc(hWnd, msg, wParam, lParam);
                if (!ret)
                {
                    This->KeyboardNavigate((WORD)wParam);
                    return 0;
                }
            }
            break;
        }

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
            case WM_CTLCOLORBTN:
            case WM_CTLCOLORSTATIC:
            {
                return (LRESULT)This->m_hDialogBack;
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

                    HWND focused = GetFocus();
                    if (child == focused)
                    {
                        This->DrawChildFocus((HDC)wParam, child, child);
                    }
                    else if (GetParent(focused) == hWnd)
                    {
                        This->DrawChildFocus((HDC)wParam, child, focused);
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
        if (!_wcsicmp(className, L"STATIC")
        || !_wcsicmp(className, L"EDIT")
        || !_wcsicmp(className, L"LISTBOX"))
        {
            return NAN;
        }
        else if (!_wcsicmp(className, L"BUTTON"))
        {
            if ((GetWindowLong(hWnd, GWL_STYLE) & BS_TYPEMASK) == BS_CHECKBOX)
            {
                return DpiScaleF(m_focusToggleMargins);
            }
            if ((GetWindowLong(hWnd, GWL_STYLE) & (BS_FLAT | WS_TABSTOP)) == BS_FLAT )
            {
                return -(float)GetSize_FocusMargins();
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

        if (!child || !m_isKeyboardFocus || GetFocus() != child || !IsWindowVisible(child)) return;

        float offset = FocusOffsetByWndClass(child);
        if (std::isnan(offset)) return;

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
            L"Segoe Fluent Icons");

        m_hGlyphNormalFont = CreateFont(
            GetSize_GlyphNormal(), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH,
            L"Segoe Fluent Icons");

        m_hIconFont = CreateFont(
            GetSize_GlyphIcon(), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
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
    const RECT Theme::DpiScale(const RECT &designRect)
    {
        return RECT {
            (LONG)DpiScale(designRect.left),
            (LONG)DpiScale(designRect.top),
            (LONG)DpiScale(designRect.right),
            (LONG)DpiScale(designRect.bottom),
        };
    }
    const RECT Theme::DpiUnscale(const RECT &actualRect)
    {
        return RECT {
            (LONG)DpiUnscale(actualRect.left),
            (LONG)DpiUnscale(actualRect.top),
            (LONG)DpiUnscale(actualRect.right),
            (LONG)DpiUnscale(actualRect.bottom),
        };
    }
    const Gdiplus::RectF Theme::DpiUnscaleF(const RECT &scaledRect)
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
            Window::GetChildRect(hChild, &rc);
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
            Window::GetChildRect(hChild, &rc);
            auto st = storage.find(hChild);
            if (st != storage.end())
            {
                RECT rc = DpiScale(st->second);
                Window::MoveWindow(hChild, &rc, FALSE);
            }
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
    }

    POINT GetCenterPoint(HWND hwnd)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        return POINT{(rc.right+rc.left) / 2, (rc.bottom+rc.top) / 2};
    }


    RECT GetRECT(HWND hwnd)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        return rc;
    }

    double Theme::GetDistance(RECT from, RECT to, POINT direction )
    {
        POINT fromPt = POINT{(from.right + from.left) / 2, (from.bottom + from.top) / 2};
        POINT toPt = POINT{(to.right + to.left) / 2, (to.bottom + to.top) / 2};

        if ( direction.x &&
            (  (to.left >= from.left && to.left <= from.right)
            || (to.right >= from.left && to.right <= from.right)
            || (from.right >= to.left && from.right <= to.right)
            || (from.left >= to.left && from.left <= to.right)
        ))
        {
            fromPt.x = toPt.x;
        }

        if ( direction.x &&
            (  (to.left >= from.left && to.left <= from.right)
            || (to.right >= from.left && to.right <= from.right)
            || (from.right >= to.left && from.right <= to.right)
            || (from.left >= to.left && from.left <= to.right)
        ))
        {
            fromPt.x = toPt.x;
        }

        if ( direction.y &&
            (  (to.top >= from.top && to.top <= from.bottom)
            || (to.bottom >= from.top && to.bottom <= from.bottom)
            || (from.bottom >= to.top && from.bottom <= to.bottom)
            || (from.top >= to.top && from.top <= to.bottom)
        ))
        {
            fromPt.y = toPt.y;
        }

        return GetDistance(fromPt, toPt, direction);
    }

    double Theme::GetDistance(POINT from, POINT to, POINT direction )
    {
        // Calculate the vector from 'from' to 'to'
        POINT delta;
        delta.x = to.x - from.x;
        delta.y = to.y - from.y;

        // Calculate Euclidean distance (actual distance)
        double distance = sqrt((double)(delta.x * delta.x + delta.y * delta.y));

        // If same point, return 0
        if (distance == 0) return 0;

        // Calculate dot product to measure alignment with direction
        // This gives positive values for vectors in same direction,
        // negative for opposite directions
        double dot = (delta.x * direction.x + delta.y * direction.y);

        // Normalize dot product to get cosine of angle
        double directionLength = sqrt((double)(direction.x * direction.x +
                                            direction.y * direction.y));
        double cosAngle = dot / (distance * directionLength);

        // Proximity factor - lower is better (closer and more aligned)
        // Option 1: Weighted by alignment (recommended)
        // Points exactly in direction get full distance value
        // Points at 90 degrees get INFINITY (not considered)
        if (cosAngle <= 0) {
            // Behind or perpendicular - not in direction at all
            return INFINITY;
        }

        // Proximity = distance / alignment
        // This makes aligned points have lower proximity values
        double proximity = distance / cosAngle;

        // Option 2: Alternative formula if you want pure distance for exact direction
        // proximity = distance + (1 - cosAngle) * SCALE_FACTOR;

        return proximity;
    }

    void Theme::KeyboardNavigate(WORD direction)
    {
        HWND focused = GetFocus();
            std::list<HWND> focusable = {};
        HWND hChild = GetWindow(m_hParentWnd, GW_CHILD);

        while (hChild)
        {
            if (IsWindowEnabled(hChild) && IsWindowVisible(hChild))
            {
                if ((GetWindowLongW(hChild, GWL_EXSTYLE) & WS_EX_CONTROLPARENT) == 0
                    && (GetWindowLongW(hChild, GWL_STYLE) & WS_TABSTOP) != 0
                    || focused == hChild)
                {
                    focusable.push_back(hChild);
                }
                if (/*(GetWindowLongW(hChild, GWL_EXSTYLE) & WS_EX_CONTROLPARENT) != 0 && */ GetWindow(hChild, GW_CHILD))
                {
                    hChild = GetWindow(hChild, GW_CHILD);
                    continue;
                }
            }

            if (!GetWindow(hChild, GW_HWNDNEXT) && GetParent(hChild) != m_hParentWnd)
            {
                hChild = GetParent(hChild);
                hChild = GetWindow(hChild, GW_HWNDNEXT);
            }
            else
            {
                hChild = GetWindow(hChild, GW_HWNDNEXT);
            }
        }
        if (focusable.size() == 0) return;

        HWND next = 0;
        auto pCurrent = std::find(focusable.begin(), focusable.end(), focused);
        if ( direction == VK_PRIOR || direction == VK_LEFT || direction == VK_UP )
        {
            next = pCurrent == focusable.end() || pCurrent == focusable.begin()
                ? focusable.back() : *(--pCurrent);
        }
        else if ( direction == VK_NEXT || direction == VK_RIGHT || direction == VK_DOWN )
        {
            next = pCurrent == focusable.end() || *pCurrent == focusable.back()
                ? focusable.front() : *(++pCurrent);
        }
        else if (pCurrent == focusable.end())
        {
            next = focusable.front();
        }
        else
        {
            POINT dir =
                    direction == VK_UP ? POINT{0,-1}
                :   direction == VK_DOWN ? POINT{0,1}
                :   direction == VK_LEFT ? POINT{-1,0}
                :   POINT{1,0};

            RECT from = GetRECT(focused);

            next = focused;
            double min_distance = FLT_MAX;

            log.Trace("From %x: (%d, %d) dir (%d, %d) ", focused, from.left, from.top, dir.x, dir.y);

            for (auto & hwnd : focusable)
            {
                if (hwnd == focused) continue;
                RECT to = GetRECT(hwnd);
                double distance = GetDistance(from, to, dir);

                log.Trace("Candidate %x: (%d, %d) distance %f ", hwnd, to.left, to.top, distance);

                if (distance > 0 && distance < min_distance )
                {
                    next = hwnd;
                    min_distance = distance;
                }
            }
        }
        m_isKeyboardFocus = true;
        m_lastFocused = next;
        log.Trace("Set focus to  %x", next);
        SetFocus(next);
    }
}