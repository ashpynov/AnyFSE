#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <Uxtheme.h>
#include "Tools/Tools.hpp"
#include "Toggle.hpp"
#include "DoubleBufferedPaint.hpp"

#include <gdiplus.h>
#include "GdiPlus.hpp"
#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

namespace FluentDesign
{
    Toggle::Toggle(FluentDesign::Theme &theme)
        : m_theme(theme)
        , buttonMouseOver(false)
        , buttonPressed(false)
        , isChecked(false)
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
        hToggle = CreateWindow(
            L"BUTTON",
            L"TOGGLE",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, m_theme.DpiScale(itemWidth + textWidth), m_theme.DpiScale(itemHeight+1),
            hParent, NULL, GetModuleHandle(NULL), NULL);

        m_theme.RegisterChild(hToggle);
        SetWindowSubclass(hToggle, ToggleSubclassProc, 0, (DWORD_PTR)this);

        return hToggle;
    }

    Toggle::~Toggle()
    {
    }

    bool Toggle::GetCheck()
    {
        return isChecked;
    }

    void Toggle::SetCheck(bool check)
    {
        isChecked = check;
        InvalidateRect(hToggle, NULL, true);
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

        case WM_KEYDOWN:
            if ( This->m_theme.IsKeyboardFocused() && wParam == VK_SPACE || wParam == VK_GAMEPAD_A)
            {
                This->isChecked = !This->isChecked;
                InvalidateRect(hWnd, NULL, TRUE);
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
            else if (buttonPressed)
            {
                int currentPos = (int16_t)LOWORD(lParam);
                int currentShift = currentPos - pressedPos;
                pressedPath += abs(currentShift);

                if (isChecked)
                    thumbShift = min(max(-m_theme.DpiScale(itemWidth-itemHeight-1), currentShift), 0);
                else
                    thumbShift = min(max(0, currentShift), m_theme.DpiScale(itemWidth-itemHeight-1));
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;

        case WM_MOUSELEAVE:
            buttonMouseOver = false;
            SendMessage(GetParent(hWnd), WM_NCMOUSELEAVE, 0, 0);
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case WM_LBUTTONDOWN:
            buttonPressed = true;
            pressedPos = LOWORD(lParam);
            thumbShift = 0;
            pressedPath = 0;
            InvalidateRect(hWnd, NULL, TRUE);
            SetCapture(hWnd);
            break;

        case WM_LBUTTONUP:
            if (buttonPressed)
            {
                buttonPressed = false;
                ReleaseCapture();
                int treshhold = m_theme.DpiScale(itemWidth - itemHeight - 1) / 2;
                if (pressedPath < treshhold || abs(thumbShift) > treshhold)
                {
                    isChecked = !isChecked;
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

        br.Y += (gdiRect.Height - m_theme.DpiScaleF(itemHeight)) / 2.0f;
        br.Height = m_theme.DpiScaleF(itemHeight);
        br.X = br.GetRight() - m_theme.DpiScaleF(itemWidth);
        br.Width = m_theme.DpiScaleF(itemWidth);

        br.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));

        // Determine colors based on state
        Color borderColor(m_theme.GetColor(
            isChecked ? ( enabled ? Theme::Colors::ToggleBorderOn : Theme::Colors::ToggleBorderOnDisabled)
                        : ( enabled ? Theme::Colors::ToggleBorderOff : Theme::Colors::ToggleBorderOffDisabled)
        ));

        Color backColor(m_theme.GetColor(
              (isChecked && !enabled)         ? Theme::Colors::ToggleTrackOnDisabled
            : (isChecked && buttonPressed)    ? Theme::Colors::ToggleTrackOnPressed
            : (isChecked)                     ? Theme::Colors::ToggleTrackOn
            : (!isChecked && !enabled)        ? Theme::Colors::ToggleTrackOffDisabled
            : (!isChecked && buttonMouseOver) ? Theme::Colors::ToggleTrackOffHover
                                              : Theme::Colors::ToggleTrackOff
        ));

        Color thumbColor(m_theme.GetColor(
            isChecked ? (enabled ? Theme::Colors::ToggleThumbOn : Theme::Colors::ToggleThumbOnDisabled)
                      : (enabled ? Theme::Colors::ToggleThumbOff : Theme::Colors::ToggleThumbOffDisabled)
        ));

        Color thumbCircleColor = (buttonPressed || buttonMouseOver) ? thumbColor : backColor;

        // Draw outer rounded rectangle (track)
        Pen borderPen(borderColor, (REAL)m_theme.DpiScale(1));
        SolidBrush backBrush(backColor);

        // For rounded rectangles in GDI+, we need to use GraphicsPath
        Gdiplus::RoundRect(graphics, br, m_theme.DpiScaleF(itemHeight), &backBrush, borderPen);

        // Calculate thumb rectangle
        RectF tr = br;
        REAL inflateAmount = m_theme.DpiScaleF(3);
        tr.Inflate(-inflateAmount, -inflateAmount);

        REAL height = tr.Height;
        if (isChecked)
        {
            tr.X = tr.GetRight() - height - m_theme.DpiScaleF(buttonPressed ? 2 : 0) + (buttonPressed ? thumbShift : 0);
            tr.Width = height + m_theme.DpiScaleF(buttonPressed ? 2 : 0);
        }
        else
        {
            tr.X += (buttonPressed ? thumbShift : 0);
            tr.Width = height + m_theme.DpiScaleF(buttonPressed ? 2 : 0);
        }

        // Draw thumb (inner circle)
        Pen thumbPen(thumbCircleColor, m_theme.DpiScaleF(1));
        SolidBrush thumbBrush(thumbColor);

        graphics.FillEllipse(&thumbBrush, tr);
        graphics.DrawEllipse(&thumbPen, tr);

        // Define the rectangle (X, Y, Width, Height)
        RectF textRect = gdiRect;
        textRect.Width = m_theme.DpiScaleF(itemWidth);

        // Create the font and brush
        Font font(hdc, m_theme.GetFont_Text());
        SolidBrush textBrush(Color(m_theme.GetColor(enabled ? Theme::Colors::Text : Theme::Colors::TextDisabled)));

        WCHAR *text = isChecked ? L"On" : L"Off";
        StringFormat format;
        format.SetAlignment(StringAlignmentNear);       // Left
        format.SetLineAlignment(StringAlignmentCenter); // Vertical center

        graphics.DrawString(text, -1, &font, textRect, &format, &textBrush);

        return;
    }
    void Toggle::UpdateLayout()
    {
        SetWindowPos(hToggle, 0, 0, 0,
                     m_theme.DpiScale(itemWidth + textWidth),
                     m_theme.DpiScale(itemHeight + 1),
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    }
}