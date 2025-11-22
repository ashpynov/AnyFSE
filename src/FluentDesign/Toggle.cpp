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
#include "Toggle.hpp"
#include "Tools/DoubleBufferedPaint.hpp"

#include "Tools/GdiPlus.hpp"
#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

namespace FluentDesign
{
    Toggle::Toggle(FluentDesign::Theme &theme)
        : m_theme(theme)
        , m_buttonMouseOver(false)
        , m_buttonPressed(false)
        , m_isChecked(false)
        , m_hToggle(nullptr)
    {
        theme.OnDPIChanged += [This = this]() { This->UpdateLayout(); };
    }

    Toggle::Toggle(
        FluentDesign::Theme& theme,
        HWND hParent,
        int x, int y,
        int width, int height)
            : Toggle(theme)
    {
        Create(hParent, x, y, width, height);
    }

    HWND Toggle::Create(
        HWND hParent,
        int x, int y,
        int width, int height
    )
    {
        m_hToggle = CreateWindow(
            L"BUTTON",
            L"",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
            x, y, m_theme.DpiScale(Layout_ItemWidth + Layout_TextWidth), m_theme.DpiScale(Layout_ItemHeight+1),
            hParent, NULL, GetModuleHandle(NULL), NULL);

        m_theme.RegisterChild(m_hToggle);
        SetWindowSubclass(m_hToggle, ToggleSubclassProc, 0, (DWORD_PTR)this);

        return m_hToggle;
    }

    Toggle::~Toggle()
    {
        if (m_hToggle)
        {
            DestroyWindow(m_hToggle);
            m_hToggle = nullptr;
        }
    }

    bool Toggle::GetCheck()
    {
        return m_isChecked;
    }

    void Toggle::SetCheck(bool check)
    {
        if ( m_isChecked != check)
        {
            m_isChecked = check;
            InvalidateRect(m_hToggle, NULL, true);
            OnChanged.Notify();
        }
    }

    LRESULT Toggle::ToggleSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {

        Toggle *This = reinterpret_cast<Toggle *>(dwRefData);
        switch (uMsg)
        {
        case WM_MOUSEMOVE:
        case WM_MOUSELEAVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            This->HandleMouse(hWnd, uMsg, lParam);
            break;

        case WM_GETDLGCODE:
            if ( wParam == VK_SPACE || wParam ==VK_RETURN || wParam ==VK_GAMEPAD_A)
            {
                return DLGC_WANTALLKEYS;
            }
            break;

        case WM_KEYDOWN:
            if ( This->m_theme.IsKeyboardFocused() && wParam == VK_SPACE || wParam == VK_RETURN || wParam == VK_GAMEPAD_A)
            {
                This->m_isChecked = !This->m_isChecked;
                InvalidateRect(hWnd, NULL, TRUE);
                This->OnChanged.Notify();
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
            RemoveWindowSubclass(hWnd, ToggleSubclassProc, uIdSubclass);
            break;
        }

        // Call original window procedure for default handling
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void Toggle::HandleMouse(HWND hWnd, UINT uMsg, LPARAM lParam)
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
            else if (m_buttonPressed)
            {
                int currentPos = (int16_t)LOWORD(lParam);
                int currentShift = currentPos - m_pressedPos;
                m_pressedPath += abs(currentShift);

                if (m_isChecked)
                    m_thumbShift = min(max(-m_theme.DpiScale(Layout_ItemWidth-Layout_ItemHeight-1), currentShift), 0);
                else
                    m_thumbShift = min(max(0, currentShift), m_theme.DpiScale(Layout_ItemWidth-Layout_ItemHeight-1));
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;

        case WM_MOUSELEAVE:
            m_buttonMouseOver = false;
            SendMessage(GetParent(hWnd), WM_NCMOUSELEAVE, 0, 0);
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case WM_LBUTTONDOWN:
            m_buttonPressed = true;
            m_pressedPos = LOWORD(lParam);
            m_thumbShift = 0;
            m_pressedPath = 0;
            InvalidateRect(hWnd, NULL, TRUE);
            SetCapture(hWnd);
            break;

        case WM_LBUTTONUP:
            if (m_buttonPressed)
            {
                m_buttonPressed = false;
                ReleaseCapture();
                int treshhold = m_theme.DpiScale(Layout_ItemWidth - Layout_ItemHeight - 1) / 2;
                if (m_pressedPath < treshhold || abs(m_thumbShift) > treshhold)
                {
                    m_isChecked = !m_isChecked;
                    InvalidateRect(hWnd, NULL, TRUE);
                    OnChanged.Notify();
                }
            }

            break;
        }
        return;
    }
    void Toggle::DrawButton(HWND hWnd, HDC hdc, RECT rect)
    {
        // Initialize GDI+ graphics object
        Graphics graphics(hdc);

        // Enable anti-aliasing for smooth edges
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        // Get button state
        bool enabled = IsWindowEnabled(hWnd);
        bool focused = (GetFocus() == hWnd);

        // Convert RECT to GDI+ RectF
        RectF gdiRect((REAL)rect.left-1, (REAL)rect.top-1,
                    (REAL)(rect.right - rect.left),
                    (REAL)(rect.bottom - rect.top+1));

        // Calculate button rectangle using GDI+ RectF
        RectF br = gdiRect;

        br.Y += (gdiRect.Height - m_theme.DpiScaleF(Layout_ItemHeight)) / 2.0f;
        br.Height = m_theme.DpiScaleF(Layout_ItemHeight);
        br.X = br.GetRight() - m_theme.DpiScaleF(Layout_ItemWidth);
        br.Width = m_theme.DpiScaleF(Layout_ItemWidth);

        br.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));

        // Determine colors based on state
        Color borderColor(m_theme.GetColor(
            m_isChecked ? ( enabled ? Theme::Colors::ToggleBorderOn : Theme::Colors::ToggleBorderOnDisabled)
                        : ( enabled ? Theme::Colors::ToggleBorderOff : Theme::Colors::ToggleBorderOffDisabled)
        ));

        Color backColor(m_theme.GetColor(
              (m_isChecked && !enabled)         ? Theme::Colors::ToggleTrackOnDisabled
            : (m_isChecked && m_buttonPressed)    ? Theme::Colors::ToggleTrackOnPressed
            : (m_isChecked)                     ? Theme::Colors::ToggleTrackOn
            : (!m_isChecked && !enabled)        ? Theme::Colors::ToggleTrackOffDisabled
            : (!m_isChecked && m_buttonMouseOver) ? Theme::Colors::ToggleTrackOffHover
                                              : Theme::Colors::ToggleTrackOff
        ));

        Color thumbColor(m_theme.GetColor(
            m_isChecked ? (enabled ? Theme::Colors::ToggleThumbOn : Theme::Colors::ToggleThumbOnDisabled)
                      : (enabled ? Theme::Colors::ToggleThumbOff : Theme::Colors::ToggleThumbOffDisabled)
        ));

        Color thumbCircleColor = (m_buttonPressed || m_buttonMouseOver) ? thumbColor : backColor;

        // Draw outer rounded rectangle (track)
        Pen borderPen(borderColor, (REAL)m_theme.DpiScale(1));
        SolidBrush backBrush(backColor);

        // For rounded rectangles in GDI+, we need to use GraphicsPath
        Gdiplus::RoundRect(graphics, br, m_theme.DpiScaleF(Layout_ItemHeight), &backBrush, borderPen);

        // Calculate thumb rectangle
        RectF tr = br;
        REAL inflateAmount = m_theme.DpiScaleF(3);
        tr.Inflate(-inflateAmount, -inflateAmount);

        REAL height = tr.Height;
        if (m_isChecked)
        {
            tr.X = tr.GetRight() - height - m_theme.DpiScaleF(m_buttonPressed ? 2 : 0) + (m_buttonPressed ? m_thumbShift : 0);
            tr.Width = height + m_theme.DpiScaleF(m_buttonPressed ? 2 : 0);
        }
        else
        {
            tr.X += (m_buttonPressed ? m_thumbShift : 0);
            tr.Width = height + m_theme.DpiScaleF(m_buttonPressed ? 2 : 0);
        }

        // Draw thumb (inner circle)
        Pen thumbPen(thumbCircleColor, m_theme.DpiScaleF(1));
        SolidBrush thumbBrush(thumbColor);

        graphics.FillEllipse(&thumbBrush, tr);
        graphics.DrawEllipse(&thumbPen, tr);

        // Define the rectangle (X, Y, Width, Height)
        RectF textRect = gdiRect;
        textRect.Width = m_theme.DpiScaleF(Layout_ItemWidth);

        // Create the font and brush
        Font font(hdc, m_theme.GetFont_Text());
        SolidBrush textBrush(Color(m_theme.GetColor(enabled ? Theme::Colors::Text : Theme::Colors::TextDisabled)));

        WCHAR *text = m_isChecked ? L"On" : L"Off";
        StringFormat format;
        format.SetAlignment(StringAlignmentNear);       // Left
        format.SetLineAlignment(StringAlignmentCenter); // Vertical center

        graphics.DrawString(text, -1, &font, textRect, &format, &textBrush);

        return;
    }
    void Toggle::UpdateLayout()
    {
        SetWindowPos(m_hToggle, 0, 0, 0,
                     m_theme.DpiScale(Layout_ItemWidth + Layout_TextWidth),
                     m_theme.DpiScale(Layout_ItemHeight + 1),
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    }
}