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


#include <windows.h>
#include <string>
#include <functional>
#include <Uxtheme.h>
#include "Tools/Unicode.hpp"
#include "Static.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"
#include "Tools/Icon.hpp"

#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

namespace FluentDesign
{
    Static::Static(FluentDesign::Theme &theme)
        : m_theme(theme)
        , m_large(false)
        , m_colorId(Theme::Colors::Text)
        , m_pImage(nullptr)
        , m_hStatic(nullptr)
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

    HWND Static::Create(HWND hParent, const std::wstring& text, int x, int y, int width, int height )
    {
        Create(hParent, x, y, width, height);
        SetText(text);
        return m_hStatic;
    }

    HWND Static::Create(HWND hParent, int x, int y, int width, int height )
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

        m_pImage = Icon::LoadBitmapFromIcon(iconPath, iconSize);
        Invalidate();
    }

    Static::~Static()
    {
        if (m_pImage)
        {
            delete m_pImage;
            m_pImage = nullptr;
        }
        if (m_hStatic)
        {
            DestroyWindow(m_hStatic);
            m_hStatic = nullptr;
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