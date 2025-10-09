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

ULONG_PTR gdiplusToken;
using namespace Gdiplus;

namespace FluentDesign
{
    Toggle::Toggle(FluentDesign::Theme &theme)
        : m_theme(theme)
        , buttonMouseOver(false)
        , buttonPressed(false)
        , isChecked(false)
    {

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
            L"",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, m_theme.DpiScale(itemWidth + textWidth), m_theme.DpiScale(itemHeight+1),
            hParent, NULL, GetModuleHandle(NULL), NULL);

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
                This->HandleMouse(hWnd, uMsg, lParam);
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
            //buttonPressed = false;
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
            buttonPressed = false;
            ReleaseCapture();
            int treshhold = m_theme.DpiScale(itemWidth - itemHeight - 1) / 2;
            if (pressedPath < treshhold || abs(thumbShift) > treshhold)
            {
                isChecked = !isChecked;
                InvalidateRect(hWnd, NULL, TRUE);
                OnChanged.Notify();
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

        // Create color objects and set from COLORREF
        Color bgColor;
        bgColor.SetFromCOLORREF(m_theme.BaseSecondaryColorBg());
        SolidBrush backgroundBrush(bgColor);
        graphics.FillRectangle(&backgroundBrush, gdiRect);

        // Calculate button rectangle using GDI+ RectF
        RectF br = gdiRect;

        br.Y += (gdiRect.Height - m_theme.DpiScaleF(itemHeight)) / 2.0f;
        br.Height = m_theme.DpiScaleF(itemHeight);
        br.X = br.GetRight() - m_theme.DpiScaleF(itemWidth);
        br.Width = m_theme.DpiScaleF(itemWidth);

        br.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));

        // Determine colors based on state
        COLORREF border = isChecked ? m_theme.ThemeColor(89) : m_theme.ThemeColor(207);
        COLORREF back =
            (isChecked && buttonPressed)      ? m_theme.ThemeColor(84)
            : (isChecked && !buttonPressed)   ? m_theme.ThemeColor(89)
            : (!isChecked && buttonMouseOver) ? m_theme.ThemeColor(52)
                                              : m_theme.ThemeColor(39);

        COLORREF thumb = isChecked ? m_theme.ThemeColor(0) : m_theme.ThemeColor(207);
        COLORREF thumbCircle = (buttonPressed || buttonMouseOver) ? thumb : back;

        // Convert COLORREF to GDI+ Color using SetFromCOLORREF
        Color borderColor;
        borderColor.SetFromCOLORREF(border);

        Color backColor;
        backColor.SetFromCOLORREF(back);

        Color thumbColor;
        thumbColor.SetFromCOLORREF(thumb);

        Color thumbCircleColor;
        thumbCircleColor.SetFromCOLORREF(thumbCircle);

        // Draw outer rounded rectangle (track)
        Pen borderPen(borderColor, (REAL)m_theme.DpiScale(1));
        SolidBrush backBrush(backColor);

        // For rounded rectangles in GDI+, we need to use GraphicsPath
        Gdiplus::RoundRect(graphics, br, m_theme.DpiScaleF(itemHeight), backBrush, borderPen);

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
        Font font(hdc, m_theme.PrimaryFont());

        Color primaryColor;
        primaryColor.SetFromCOLORREF(m_theme.PrimaryColor());
        SolidBrush textBrush(primaryColor);

        WCHAR *text = isChecked ? L"On" : L"Off";
        StringFormat format;
        format.SetAlignment(StringAlignmentNear);       // Left
        format.SetLineAlignment(StringAlignmentCenter); // Vertical center

        graphics.DrawString(text, -1, &font, textRect, &format, &textBrush);

        return;
    }
}