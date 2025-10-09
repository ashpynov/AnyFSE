#include "Theme.hpp"
#include <gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")

namespace FluentDesign
{
    Theme::Theme(HWND hParentWnd)
        : m_hParentWnd(NULL)
        , m_dpi(96)
        , m_hPrimaryFont(NULL)
        , m_hSecondaryFont(NULL)
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        LoadColors(true);
        if (hParentWnd)
        {
            Attach(hParentWnd);
        }
        else
        {
            CreateFonts();
        }
    }

    void Theme::Attach(HWND hParentWnd)
    {
        m_hParentWnd = hParentWnd;
        m_dpi = GetDpiForWindow(m_hParentWnd);
        CreateFonts();

        // TODO: subclass parent wnd
    }

    Theme::~Theme()
    {
        // TODO: unsubclass parent wnd
        FreeFonts();
        Gdiplus::GdiplusShutdown(gdiplusToken);
    };

    void Theme::LoadColors(bool isDark)
    {
        if (isDark)
        {
            m_primaryColor = RGB(255, 255, 255);
            m_secondaryColor = RGB(200, 200, 200);
            m_disabledColor = RGB(128, 128, 128);

            m_baseColorBg = RGB(32, 32, 32);
            m_baseSecondaryColorBg = RGB(43, 43, 43);
            m_popupColorBg = RGB(38, 38, 38);
            m_pressedColorBg = RGB(50, 50, 50);
            m_controlColorBg = RGB(55, 55, 55);
            m_hoveredColorBg = RGB(61, 61, 61);
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
        primaryFont.lfHeight = (LONG)(-PrimarySize());
        primaryFont.lfWeight = FW_NORMAL;
        m_hPrimaryFont = CreateFontIndirect(&primaryFont);

        // Create description font (slightly smaller, normal weight)
        LOGFONT secondaryFont = ncm.lfMessageFont;
        secondaryFont.lfHeight = (LONG)(-SecondarySize());
        m_hSecondaryFont = CreateFontIndirect(&secondaryFont);

        m_hGlyphFont = CreateFont(
            GlyphSize(), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH,
            L"Segoe MDL2 Assets");
    }

    void Theme::FreeFonts()
    {
        if (m_hPrimaryFont)
            DeleteObject(m_hPrimaryFont);

        if (m_hSecondaryFont)
            DeleteObject(m_hSecondaryFont);

        if (m_hGlyphFont)
            DeleteObject(m_hGlyphFont);

        m_hPrimaryFont = NULL;
        m_hSecondaryFont = NULL;
        m_hGlyphFont = NULL;
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