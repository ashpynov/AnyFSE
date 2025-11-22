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
#include "Button.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"

#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

namespace FluentDesign
{
    Button::Button(FluentDesign::Theme &theme)
        : m_theme(theme)
        , m_buttonMouseOver(false)
        , m_buttonPressed(false)
        , m_isIconButton(false)
        , m_bFlat(false)
        , m_bSquare(false)
        , m_hButton(nullptr)
        , m_textNormalColor(Theme::Colors::Text)
        , m_backgroundNormalColor(Theme::Colors::Button)
        , m_textHoverColor(Theme::Colors::Text)
        , m_backgroundHoverColor(Theme::Colors::ButtonHover)
        , m_textPressedColor(Theme::Colors::Text)
        , m_backgroundPressedColor(Theme::Colors::ButtonPressed)
    {
    }

    Button::Button(
        FluentDesign::Theme& theme,
        HWND hParent,
        int x, int y,
        int width, int height)
            : Button(theme)
    {
        Create(hParent, x, y, width, height);
    }

    HWND Button::Create(
        HWND hParent,
        int x, int y,
        int width, int height
    )
    {

        m_designHeight = m_theme.DpiUnscale(height);
        m_designWidth = m_theme.DpiUnscale(width);
        m_hButton = CreateWindow(
            L"BUTTON",
            L"",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
            x, y, width, height,
            hParent, NULL, GetModuleHandle(NULL), NULL);


        if (GetWindowLong(hParent, GWL_STYLE)& WS_CHILD)
        {
            m_theme.OnDPIChanged += [This = this]() { This->UpdateLayout(); };
        }
        m_theme.RegisterChild(m_hButton);
        SetWindowSubclass(m_hButton, ButtonSubclassProc, 0, (DWORD_PTR)this);
        return m_hButton;
    }

    HWND Button::Create(HWND hParent, const std::wstring &text, const std::function<void()>& callback, int x, int y, int width, int height)
    {
        Create(hParent, x, y, width, height);
        SetText(text);
        OnChanged += callback;
        return m_hButton;
    }

    void Button::SetText(const std::wstring &text)
    {
        m_text = text;
        m_isIconButton = false;
        InvalidateRect(m_hButton, NULL, TRUE);
    }

    void Button::SetIcon(const std::wstring &glyph)
    {
        m_text = glyph;
        m_isIconButton = true;
        InvalidateRect(m_hButton, NULL, TRUE);
    }

    void Button::Enable(bool bEnable)
    {
        EnableWindow(m_hButton, bEnable);
        InvalidateRect(m_hButton, NULL, TRUE);
    }

    void Button::Show(bool bShow)
    {
        ShowWindow(m_hButton, bShow ? SW_SHOW : SW_HIDE);
    }

    void Button::SetFlat(bool isFlat)
    {
        m_bFlat = isFlat;
        InvalidateRect(m_hButton, NULL, TRUE);
    }

    void Button::SetSquare(bool isSquare)
    {
        m_bSquare = isSquare;
        InvalidateRect(m_hButton, NULL, TRUE);
    }

    void Button::SetColors(Theme::Colors textNornal, Theme::Colors backgroundNormal, Theme::Colors textHover, Theme::Colors backgroundHover, Theme::Colors textPressed, Theme::Colors backgroundPressed)
    {
        m_textNormalColor = textNornal == Theme::Colors::Default ? m_textNormalColor : textNornal;
        m_backgroundNormalColor = backgroundNormal == Theme::Colors::Default ? m_backgroundNormalColor : backgroundNormal;
        m_textHoverColor = textHover == Theme::Colors::Default ? m_textHoverColor : textHover;
        m_backgroundHoverColor = backgroundHover == Theme::Colors::Default ? m_backgroundHoverColor : backgroundHover;
        m_textPressedColor = textPressed == Theme::Colors::Default ? m_textPressedColor : textPressed;
        m_backgroundPressedColor = backgroundPressed == Theme::Colors::Default ? m_backgroundPressedColor : backgroundPressed;
        InvalidateRect(m_hButton, NULL, TRUE);
    }

    void Button::SetMenu(const std::vector<Popup::PopupItem> &items)
    {
        m_menuItems = items;
    }

    void Button::ShowMenu()
    {
        static Popup popup(m_theme);

        RECT rect;
        GetWindowRect(m_hButton, &rect);
        popup.Show(m_hButton, rect.right, rect.bottom, m_menuItems, 300);
        m_theme.SwapFocus(popup.GetHwnd());
    }

    Button::~Button()
    {
        if (m_hButton)
        {
            DestroyWindow(m_hButton);
            m_hButton = nullptr;
        }
    }

    LRESULT Button::ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {

        Button *This = reinterpret_cast<Button *>(dwRefData);
        switch (uMsg)
        {
            case WM_MOUSEMOVE:
            case WM_MOUSELEAVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
                This->HandleMouse(hWnd, uMsg, lParam);
                if (This->m_menuItems.size())
                {
                    return 0;
                }
                break;

            case WM_GETDLGCODE:
                if ( wParam == VK_SPACE || wParam ==VK_RETURN || wParam ==VK_GAMEPAD_A)
                {
                    return DLGC_WANTALLKEYS;
                }
                break;

            case WM_KEYDOWN:
                if ( This->m_theme.IsKeyboardFocused() &&
                       wParam == VK_SPACE
                    || wParam == VK_RETURN
                    || wParam == VK_GAMEPAD_A)
                {
                    if (This->m_menuItems.size())
                    {
                        This->ShowMenu();
                    }
                    else
                    {
                        This->OnButtonDown.Notify();
                        This->OnChanged.Notify();
                    }
                }
                return 0;

            case WM_PAINT:
                {
                    FluentDesign::DoubleBuferedPaint paint(hWnd);

                    RECT rect = paint.ClientRect();
                    This->DrawButton(hWnd, paint.MemDC(), rect);
                }
                return 0;

            case WM_ERASEBKGND:
                return 1;
            case WM_NCDESTROY:
                RemoveWindowSubclass(hWnd, ButtonSubclassProc, uIdSubclass);
                break;
        }

        // Call original window procedure for default handling
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void Button::HandleMouse(HWND hWnd, UINT uMsg, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_MOUSEMOVE:
            if (!m_buttonMouseOver)
            {
                m_buttonMouseOver = true;
                InvalidateRect(hWnd, NULL, TRUE);

                // Track mouse leave
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
            }
            break;

        case WM_MOUSELEAVE:
            m_buttonMouseOver = false;
            m_buttonPressed = false;
            InvalidateRect(hWnd, NULL, FALSE);
            SendMessage(GetParent(hWnd), WM_NCMOUSELEAVE, 0, 0);
            break;

        case WM_LBUTTONDOWN:
            m_buttonPressed = true;
            InvalidateRect(hWnd, NULL, FALSE);
            SetCapture(hWnd);

            if (m_menuItems.size())
            {
                m_theme.SwapFocus(hWnd);
                ShowMenu();
            }
            else
            {
                OnButtonDown.Notify();
            }
            break;

        case WM_LBUTTONUP:
            if (m_buttonPressed)
            {
                m_buttonPressed = false;
                ReleaseCapture();
                InvalidateRect(hWnd, NULL, FALSE);

                POINT pt;
                GetCursorPos(&pt);
                MapWindowPoints(NULL, m_hButton, &pt, 1);

                RECT rc;
                GetClientRect(m_hButton, &rc);
                if (PtInRect(&rc, pt) && !m_menuItems.size())
                {
                    OnChanged.Notify();
                }
            }
            break;
        }
        return;
    }
    void Button::DrawButton(HWND hWnd, HDC hdc, RECT rect)
    {
        // Initialize GDI+ graphics object
        Graphics graphics(hdc);

        // Enable anti-aliasing for smooth edges
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        // Get button state
        bool enabled = IsWindowEnabled(hWnd);
        bool focused = (GetFocus() == hWnd);

        // Convert RECT to GDI+ RectF
        RectF gdiRect((REAL)rect.left, (REAL)rect.top,
                    (REAL)(rect.right - rect.left),
                    (REAL)(rect.bottom - rect.top));

        // Calculate button rectangle using GDI+ RectF
        RectF br = gdiRect;
        br.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));

        // Determine colors based on state
        Color borderColor(m_theme.GetColor(
                m_buttonPressed ? Theme::Colors::ButtonBorderPressed
            : m_buttonMouseOver ? Theme::Colors::ButtonBorderHover
                                : Theme::Colors::ButtonBorder
        ));

        Color backColor(m_theme.GetColor(
                m_buttonPressed ? m_backgroundPressedColor
            : m_buttonMouseOver ? m_backgroundHoverColor
                                : m_backgroundNormalColor
        ));

        if (m_bFlat)
        {
            borderColor = backColor;
        }

        if (!m_bFlat || m_buttonPressed || m_buttonMouseOver)
        {
            // Draw outer rounded rectangle (track)
            Pen borderPen(borderColor, (REAL)m_theme.DpiScale(1));
            SolidBrush backBrush(backColor);

            // For rounded rectangles in GDI+, we need to use GraphicsPath
            if (m_bSquare)
            {
                graphics.FillRectangle(&backBrush, br);
                if (!m_bFlat)
                {
                    graphics.DrawRectangle(&borderPen, br);
                }
            }
            else
            {
                Gdiplus::RoundRect(graphics, br, m_theme.DpiScaleF(m_cornerRadius), &backBrush, borderPen);
            }
        }

        // Create the font and brush
        Font font(hdc, m_isIconButton ? m_theme.GetFont_GlyphNormal() : m_theme.GetFont_Text());

        Color textColor(m_theme.GetColor(
            ! enabled           ? Theme::Colors::TextDisabled
            : m_buttonPressed   ? m_textPressedColor
            : m_buttonMouseOver ? m_textHoverColor
                                : m_textNormalColor
        ));

        SolidBrush textBrush(textColor);

        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);

        graphics.DrawString(m_text.c_str(), -1, &font, br, &format, &textBrush);

        return;
    }
    void Button::UpdateLayout()
    {
        SetWindowPos(m_hButton, 0, 0, 0,
                     m_theme.DpiScale(m_designWidth),
                     m_theme.DpiScale(m_designHeight),
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    }
}