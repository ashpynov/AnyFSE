#include "Theme.hpp"
#include "GdiPlus.hpp"
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
        , m_isDark(true)
        , m_isKeyboardFocus(false)
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
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
        m_isDark = true;
        m_hParentWnd = hHostWnd;
        m_dpi = GetDpiForWindow(m_hParentWnd);
        LoadColors();
        CreateFonts();
        BOOL useDarkMode = IsDark() ? TRUE : FALSE;
        DwmSetWindowAttribute(hHostWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

        COLORREF captionColor = GetColorRef(FluentDesign::Theme::Colors::Dialog);
        DwmSetWindowAttribute(hHostWnd, DWMWA_CAPTION_COLOR, &captionColor, sizeof(captionColor));
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        SetWindowSubclass(hHostWnd, DialogSubclassProc, 0, (DWORD_PTR)this);

    }

    void Theme::RegisterChild(HWND hChild)
    {
        childs.push_back(hChild);
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
                    for( auto &c : This->childs)
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
                    Invalidate(last);
                }
                else
                if (last && last != next)
                {
                    This->m_isKeyboardFocus = true;
                    This->m_lastFocused = next;
                }
                else if (GetParent(next) != prev || !last)
                {
                    This->m_lastFocused = next;
                }
                Invalidate(next);
            }
            break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            {
                This->m_lastFocused = hWnd;
                This->m_isKeyboardFocus = false;
                SetFocus(hWnd);
                Invalidate(hWnd);
                return 0;
            }
            // fall down
        case WM_KILLFOCUS:
            {
                Invalidate(hWnd, TRUE);
            }
            break;
        case WM_DESTROY:
            RemoveWindowSubclass(hWnd, ControlSublassProc, uIdSubclass);
            break;
        }
        return DefSubclassProc(hWnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK Theme::DialogSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        Theme *This = reinterpret_cast<Theme *>(dwRefData);

        switch (msg)
        {
            case WM_DPICHANGED:
                {
                    This->m_dpi = HIWORD(wParam);
                    This->CreateFonts();
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
                if(child)
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
        Gdiplus::GdiplusShutdown(gdiplusToken);
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

        RectF focusRect = FromRECT(clientRect);
        focusRect.Inflate(offset, offset);

        Pen borderPen(GetColor(FluentDesign::Theme::Colors::FocusFrame), (REAL)GetSize_FocusWidth());

        // For rounded rectangles in GDI+, we need to use GraphicsPath
        Gdiplus::RoundRect(graphics, focusRect, (REAL)GetSize_FocusCornerSize() + offset, nullptr, borderPen);

        log.Info("Focus frame");
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


    DWORD Theme::GetGrey(BYTE lumen)
    {
        return RGB(lumen, lumen, lumen);
    }
    void Theme::LoadColors()
    {
        if (m_isDark)
        {
            m_colors[Text] = GetGrey(255);
            m_colors[TextSecondary] = GetGrey(200);
            m_colors[TextDisabled] = GetGrey(128);

            m_colors[FocusFrame] = GetGrey(200);

            m_colors[Panel] = GetGrey(43);
            m_colors[PanelHover] = GetGrey(50);

            m_colors[Dialog] = GetGrey(32);

            m_colors[ToggleBorderOn] = GetGrey(89);
            m_colors[ToggleBorderOff] = GetGrey(207);
            m_colors[ToggleTrackOn] = GetGrey(89);           // accented color
            m_colors[ToggleTrackOnPressed] = GetGrey(84);    // accented color darker
            m_colors[ToggleTrackOnDisabled] = GetGrey(89);           // accented color
            m_colors[ToggleTrackOff] = GetGrey(39);
            m_colors[ToggleTrackOffHover] = GetGrey(52);
            m_colors[ToggleThumbOn] = GetGrey(0);
            m_colors[ToggleThumbOff] = GetGrey(207);

            m_colors[ToggleBorderOnDisabled] = GetGrey(60);
            m_colors[ToggleTrackOnDisabled] = GetGrey(60);
            m_colors[ToggleThumbOnDisabled] = GetGrey(0);
            m_colors[ToggleBorderOffDisabled] = GetGrey(60);
            m_colors[ToggleTrackOffDisabled] = GetGrey(39);
            m_colors[ToggleThumbOffDisabled] = GetGrey(60);


            m_colors[Combo] = GetGrey(55);
            m_colors[ComboDisabled] = GetGrey(50);
            m_colors[ComboHover] = GetGrey(61);
            m_colors[ComboPressed] = GetGrey(50);
            m_colors[ComboPopup] = GetGrey(38);
            m_colors[ComboPopupBorder] = GetGrey(1);
            m_colors[ComboPopupSelected] = GetGrey(61);
            m_colors[ComboPopupSelectedMark] = GetGrey(89);  // accented color
            m_colors[ComboPopupHover] = GetGrey(61);

            m_colors[Edit] = GetGrey(55);
            m_colors[EditHover] = GetGrey(61);
            m_colors[EditAccent] = GetGrey(154);
            m_colors[EditFocus] = GetGrey(31);
            m_colors[EditAccentFocus] = GetGrey(64);         // accented color
            m_colors[EditBorderFocus] = GetGrey(58);
            m_colors[Button] = GetGrey(55);
            m_colors[ButtonHover] = GetGrey(60);
            m_colors[ButtonPressed] = GetGrey(50);
            m_colors[ButtonBorder] = GetGrey(63);
            m_colors[ButtonBorderHover] = GetGrey(63);
            m_colors[ButtonBorderPressed] = GetGrey(58);
        }
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

        LOGFONT titleFont = ncm.lfMessageFont;
        titleFont.lfHeight = (LONG)(-GetSize_Title());
        titleFont.lfWeight = FW_BOLD;
        m_hTitleFont = CreateFontIndirect(&titleFont);
    }

    void Theme::FreeFonts()
    {
        if (m_hPrimaryFont)
            DeleteObject(m_hPrimaryFont);

        if (m_hSecondaryFont)
            DeleteObject(m_hSecondaryFont);

        if (m_hGlyphFont)
            DeleteObject(m_hGlyphFont);

        if (m_hGlyphNormalFont)
            DeleteObject(m_hGlyphNormalFont);

        if (m_hTitleFont)
            DeleteObject(m_hTitleFont);

        m_hPrimaryFont = NULL;
        m_hSecondaryFont = NULL;
        m_hGlyphFont = NULL;
        m_hTitleFont = NULL;
        m_hGlyphNormalFont = NULL;
    }

    const int Theme::DpiUnscale(int scaledSize)
    {
        return MulDiv(scaledSize, 96, m_dpi);
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
    };
}