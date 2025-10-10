#include "Theme.hpp"
#include <gdiplus.h>
#include <commctrl.h>

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

        SetWindowSubclass(hParentWnd, DialogSubclassProc, 0, (DWORD_PTR)this);

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

    void Theme::LoadColors(bool isDark)
    {
        if (isDark)
        {
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