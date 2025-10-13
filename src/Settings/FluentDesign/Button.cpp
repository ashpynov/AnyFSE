#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <Uxtheme.h>
#include "Tools/Tools.hpp"
#include "Button.hpp"
#include "DoubleBufferedPaint.hpp"

#include <gdiplus.h>
#include "GdiPlus.hpp"
#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

namespace FluentDesign
{
    Button::Button(FluentDesign::Theme &theme)
        : m_theme(theme)
        , buttonMouseOver(false)
        , buttonPressed(false)
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
        hButton = CreateWindow(
            L"BUTTON",
            L"",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
            x, y, width, height,
            hParent, NULL, GetModuleHandle(NULL), NULL);


        if (GetWindowLong(hParent, GWL_STYLE)& WS_CHILD)
        {
            m_theme.OnDPIChanged += [This = this]() { This->UpdateLayout(); };
        }
        m_theme.RegisterChild(hButton);
        SetWindowSubclass(hButton, ButtonSubclassProc, 0, (DWORD_PTR)this);
        return hButton;
    }

    void Button::SetText(const std::wstring &text)
    {
        m_text = text;
        InvalidateRect(hButton, NULL, TRUE);
    }

    Button::~Button()
    {
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
                break;

            case WM_KEYDOWN:
                if (This->m_theme.IsKeyboardFocused() && wParam == VK_SPACE || wParam == VK_GAMEPAD_A)
                {
                    This->OnChanged.Notify();
                }
                break;
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
            if (!buttonMouseOver)
            {
                buttonMouseOver = true;
                InvalidateRect(hWnd, NULL, TRUE);

                // Track mouse leave
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
            }
            break;

        case WM_MOUSELEAVE:
            buttonMouseOver = false;
            buttonPressed = false;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case WM_LBUTTONDOWN:
            buttonPressed = true;
            InvalidateRect(hWnd, NULL, TRUE);
            SetCapture(hWnd);
            break;

        case WM_LBUTTONUP:
            if (buttonPressed)
            {
                buttonPressed = false;
                ReleaseCapture();
                InvalidateRect(hWnd, NULL, TRUE);

                POINT pt;
                GetCursorPos(&pt);
                MapWindowPoints(NULL, hButton, &pt, 1);

                RECT rc;
                GetClientRect(hButton, &rc);
                if (PtInRect(&rc, pt))
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
        Color borderColor(
                buttonPressed ? m_theme.GetColor(Theme::Colors::ButtonBorderPressed)
            : buttonMouseOver ? m_theme.GetColor(Theme::Colors::ButtonBorderHover)
                              : m_theme.GetColor(Theme::Colors::ButtonBorder)
        );

        Color backColor(
                buttonPressed ? m_theme.GetColor(Theme::Colors::ButtonPressed)
            : buttonMouseOver ? m_theme.GetColor(Theme::Colors::ButtonHover)
                              : m_theme.GetColor(Theme::Colors::Button)
        );

        // Draw outer rounded rectangle (track)
        Pen borderPen(borderColor, (REAL)m_theme.DpiScale(1));
        SolidBrush backBrush(backColor);

        // For rounded rectangles in GDI+, we need to use GraphicsPath
        Gdiplus::RoundRect(graphics, br, m_theme.DpiScaleF(m_cornerRadius), &backBrush, borderPen);

        // Create the font and brush
        Font font(hdc, m_theme.GetFont_Text());
        SolidBrush textBrush(Color(m_theme.GetColor(Theme::Colors::Text)));

        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);

        graphics.DrawString(m_text.c_str(), -1, &font, br, &format, &textBrush);

        return;
    }
    void Button::UpdateLayout()
    {
        SetWindowPos(hButton, 0, 0, 0,
                     m_theme.DpiScale(m_designWidth),
                     m_theme.DpiScale(m_designHeight),
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    }
}