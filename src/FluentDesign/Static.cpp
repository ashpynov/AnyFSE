// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include <windows.h>
#include <string>
#include <functional>
#include <Uxtheme.h>
#include "Tools/Unicode.hpp"
#include "Static.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"
#include "Tools/Tools.hpp"

#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

namespace FluentDesign
{
    Static::Static(FluentDesign::Theme &theme)
        : m_theme(theme)
        , m_large(false)
        , m_colorId(Theme::Colors::Text)
        , m_pImage(nullptr)
    {
        m_format.SetAlignment(Gdiplus::StringAlignmentNear);
        m_format.SetLineAlignment(Gdiplus::StringAlignmentNear);
    }

    Static::Static(
        FluentDesign::Theme& theme,
        HWND hParent,
        int x, int y,
        int width, int height)
            : Static(theme)
    {
        Create(hParent, x, y, width, height);
    }

    HWND Static::Create(
        HWND hParent,
        int x, int y,
        int width, int height
    )
    {

        m_designHeight = m_theme.DpiUnscale(height);
        m_designWidth = m_theme.DpiUnscale(width);
        m_hStatic = CreateWindow(
            L"STATIC",
            L"",
            WS_VISIBLE | WS_CHILD,
            x, y, width, height,
            hParent, NULL, GetModuleHandle(NULL), NULL);


        if (GetWindowLong(hParent, GWL_STYLE)& WS_CHILD)
        {
            m_theme.OnDPIChanged += [This = this]() { This->UpdateLayout(); };
        }
        m_theme.RegisterChild(m_hStatic);
        SetWindowSubclass(m_hStatic, StaticSubclassProc, 0, (DWORD_PTR)this);
        return m_hStatic;
    }

    void Static::SetText(const std::wstring &text)
    {
        m_text = text;
        Invalidate();
    }

    void Static::SetColor(Theme::Colors colorId)
    {
        m_colorId = colorId;
        Invalidate();
    }

    void Static::SetLarge(bool large)
    {
        m_large = large;
        Invalidate();
    }

    void Static::Invalidate()
    {
        InvalidateRect(m_hStatic, NULL, TRUE);
    }

    void Static::LoadIcon(const std::wstring &iconPath, int iconSize)
    {
        if (m_pImage)
        {
            delete m_pImage;
            m_pImage = nullptr;
        }

        m_pImage = Tools::LoadBitmapFromIcon(iconPath, iconSize);
        Invalidate();
    }

    Static::~Static()
    {
        if (m_pImage)
        {
            delete m_pImage;
            m_pImage = nullptr;
        }
    }

    LRESULT Static::StaticSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {

        Static *This = reinterpret_cast<Static *>(dwRefData);
        switch (uMsg)
        {
        case WM_PAINT:
            {
                FluentDesign::DoubleBuferedPaint paint(hWnd);
                RECT rect = paint.ClientRect();
                This->DrawStatic(hWnd, paint.MemDC(), rect);
            }
            return 0;

        case WM_ERASEBKGND:
            return 1;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, StaticSubclassProc, uIdSubclass);
            break;
        }

        // Call original window procedure for default handling
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void Static::DrawStatic(HWND hWnd, HDC hdc, RECT rect)
    {
        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        RectF rectF = ToRectF(rect);
        rectF.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));

        if (m_pImage)
        {
            float imageWidth = (float)m_pImage->GetWidth();
            float imageHeight = (float)m_pImage->GetHeight();
            float scale = min(rectF.Width / imageWidth, rectF.Height / imageHeight);

            imageWidth *= scale;
            imageHeight *= scale;

            graphics.DrawImage(m_pImage,
                (rectF.Width - imageWidth) / 2,
                (rectF.Height - imageHeight) /2,
                imageWidth, imageHeight
            );
        }
        else
        {
            // Create the font and brush
            Font font(hdc, m_large ? m_theme.GetFont_TextLarge() : m_theme.GetFont_Text() );
            SolidBrush textBrush(Color(m_theme.GetColor(m_colorId)));
            graphics.DrawString(m_text.c_str(), -1, &font, rectF, &m_format, &textBrush);
        }
        return;
    }
    void Static::UpdateLayout()
    {
        SetWindowPos(m_hStatic, 0, 0, 0,
                     m_theme.DpiScale(m_designWidth),
                     m_theme.DpiScale(m_designHeight),
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    }
}