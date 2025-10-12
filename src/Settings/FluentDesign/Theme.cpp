#include "Theme.hpp"
#include <gdiplus.h>
#include <dwmapi.h>
#include <commctrl.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Gdiplus.lib")

namespace FluentDesign
{
    Theme::Theme(HWND hParentWnd)
        : m_hParentWnd(NULL)
        , m_dpi(96)
        , m_hPrimaryFont(NULL)
        , m_hSecondaryFont(NULL)
        , m_isDark(true)
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
            case WM_DESTROY:
                RemoveWindowSubclass(hWnd, DialogSubclassProc, uIdSubclass);
                break;
        }
        return DefSubclassProc(hWnd, msg, wParam, lParam);
    }


    Theme::~Theme()
    {
        // TODO: unsubclass parent wnd
        FreeFonts();
        Gdiplus::GdiplusShutdown(gdiplusToken);
    };

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
            m_colors[Max] = GetGrey(20);
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