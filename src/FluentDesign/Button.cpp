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
#include <algorithm>
#include <Uxtheme.h>
#include "Tools/Unicode.hpp"
#include "Button.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"

#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

namespace FluentDesign
{
    static RectF MeasureIconLayers(Graphics& graphics, const std::wstring& glyphs, const Font& font, const StringFormat& format)
    {
        const RectF bounds(0.0f, 0.0f, 100000.0f, 100000.0f);
        RectF iconRect{};
        for (const wchar_t glyph : glyphs)
        {
            RectF layerRect{};
            graphics.MeasureString(&glyph, 1, &font, bounds, &format, &layerRect);
            iconRect.Width = (std::max)(iconRect.Width, layerRect.Width);
            iconRect.Height = (std::max)(iconRect.Height, layerRect.Height);
        }
        return iconRect;
    }

    Button::Button(Theme &theme, Align::Anchor align, GetParentRectFunc getParentRect)
        : FluentControl(theme, align, getParentRect)
        , m_buttonMouseOver(false)
        , m_bFlat(false)
        , m_bSquare(false)
        , m_isSmallIcon(false)
        , m_popup(theme)
        , m_textNormalColor(Theme::Colors::Text)
        , m_backgroundNormalColor(Theme::Colors::Button)
        , m_textHoverColor(Theme::Colors::Text)
        , m_backgroundHoverColor(Theme::Colors::ButtonHover)
        , m_textPressedColor(Theme::Colors::Text)
        , m_backgroundPressedColor(Theme::Colors::ButtonPressed)
        , m_iconAngle(0)
        , m_startAngle(0)
        , m_endAngle(0)
        , m_stepAngle(0)
        , m_animationLoop(false)
        , m_animationTimer(0)
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

    Button& Button::Create(
        HWND hParent,
        int x, int y,
        int width, int height
    )
    {
        AnchoredSizePos(hParent, x, y, width, height);
        m_designHeight = m_theme.DpiUnscale(height);
        m_designWidth = m_theme.DpiUnscale(width);
        m_hWnd = CreateWindow(
            L"BUTTON",
            L"",
            WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_CENTER,
            x, y, width, height,
            hParent, NULL, GetModuleHandle(NULL), NULL);


        if (GetWindowLong(hParent, GWL_STYLE)& WS_CHILD)
        {
            m_theme.OnDPIChanged += [This = this]() { This->UpdateLayout(); };
        }
        m_theme.RegisterChild(m_hWnd);
        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
        SetWindowSubclass(m_hWnd, ButtonSubclassProc, 0, (DWORD_PTR)this);
        EnablePanGesture();
        OnClick += delegate(HandleClick);
        return *this;
    }

    Button& Button::Create(HWND hParent, const std::wstring &text, const std::function<void()>& callback, int x, int y, int width, int height)
    {
        Create(hParent, x, y, width, height);
        SetText(text);
        OnChanged += callback;
        return *this;
    }

    Button& Button::SetText(const std::wstring &text)
    {
        if (m_text == text)
            return *this;

        m_text = text;
        InvalidateRect(m_hWnd, NULL, FALSE);
        return *this;
    }

    std::wstring & Button::GetText()
    {
        return m_text;
    }

    Button& Button::SetIcon(const std::wstring &glyph, bool bSmall)
    {
        if (m_icon == glyph && m_isSmallIcon == bSmall)
            return *this;

        m_icon = glyph;
        m_isSmallIcon = bSmall;

        InvalidateRect(m_hWnd, NULL, FALSE);
        return *this;
    }

    Button&  Button::Enable(bool bEnable)
    {
        if ((bool)IsWindowEnabled(m_hWnd) == bEnable)
            return *this;

        SetWindowLong(m_hWnd, GWL_STYLE, (GetWindowLong(m_hWnd, GWL_STYLE) & ~WS_DISABLED) | (bEnable ? 0 : WS_DISABLED));
        InvalidateRect(m_hWnd, NULL, FALSE);
        return *this;
    }

    Button& Button::SetTabStop(bool bTabStop)
    {
        SetWindowLong(m_hWnd, GWL_STYLE, (GetWindowLong(m_hWnd, GWL_STYLE) & ~WS_TABSTOP) | (bTabStop ? WS_TABSTOP : 0));
        return *this;
    }

    Button& Button::Show(bool bShow)
    {
        ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
        return *this;
    }

    Button& Button::SetFlat(bool isFlat)
    {
        m_bFlat = isFlat;
        SetWindowLong(m_hWnd, GWL_STYLE, (GetWindowLong(m_hWnd, GWL_STYLE) & ~BS_FLAT) | (isFlat ? BS_FLAT : 0));
        InvalidateRect(m_hWnd, NULL, FALSE);
        return *this;
    }

    Button& Button::SetSquare(bool isSquare)
    {
        m_bSquare = isSquare;
        InvalidateRect(m_hWnd, NULL, FALSE);
        return *this;
    }

    Button& Button::SetAlign(int bsStyle)
    {
        long style = GetWindowLong(m_hWnd, GWL_STYLE) & ~BS_CENTER;
        SetWindowLong(m_hWnd, GWL_STYLE, style | (bsStyle & BS_CENTER));
        InvalidateRect(m_hWnd, NULL, FALSE);
        return *this;
    }

    void Button::SetAngle(int angle)
    {
        m_iconAngle = angle;
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    Button& Button::SetLinkStyle()
    {
        SetWindowLong(m_hWnd, GWL_STYLE, (GetWindowLong(m_hWnd, GWL_STYLE) & ~BS_TYPEMASK) | BS_COMMANDLINK);

        SetColors(
            Theme::Colors::TextAccented,  Theme::Colors::Transparent,
            Theme::Colors::Text,          Theme::Colors::Transparent,
            Theme::Colors::TextSecondary, Theme::Colors::Transparent
        );
        SetAlign(BS_LEFT);
        SetFlat(true);
        SetSize(GetMinSize());
        InvalidateRect(m_hWnd, NULL, FALSE);
        return *this;
    }

    void Button::SetSize(LONG cx, LONG cy)
    {
        if (cy != -1)
        {
            m_designHeight = m_theme.DpiUnscale(cy);
        }

        if (cx != -1)
        {
            m_designWidth = m_theme.DpiUnscale(cx);
        }
        if (m_anchor)
        {
            FluentControl::SetSize(cx, cy);
        }
        else
        {
            SetWindowPos(m_hWnd, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    Button& Button::SetColors(Theme::Colors textNornal, Theme::Colors backgroundNormal, Theme::Colors textHover, Theme::Colors backgroundHover, Theme::Colors textPressed, Theme::Colors backgroundPressed)
    {
        m_textNormalColor = textNornal == Theme::Colors::Default ? m_textNormalColor : textNornal;
        m_backgroundNormalColor = backgroundNormal == Theme::Colors::Default ? m_backgroundNormalColor : backgroundNormal;
        m_textHoverColor = textHover == Theme::Colors::Default ? m_textHoverColor : textHover;
        m_backgroundHoverColor = backgroundHover == Theme::Colors::Default ? m_backgroundHoverColor : backgroundHover;
        m_textPressedColor = textPressed == Theme::Colors::Default ? m_textPressedColor : textPressed;
        m_backgroundPressedColor = backgroundPressed == Theme::Colors::Default ? m_backgroundPressedColor : backgroundPressed;
        InvalidateRect(m_hWnd, NULL, TRUE);
        return *this;
    }

    void Button::SetMenu(const std::vector<Popup::PopupItem> &items, int width, int align)
    {
        m_menuItems = items;
        m_menuWidth = width;
        m_menuAlingment = align;
    }

    void Button::ShowMenu()
    {
        RECT rect;
        GetWindowRect(m_hWnd, &rect);
        int x = (m_menuAlingment & TPM_RIGHTALIGN) ? rect.right
                : (m_menuAlingment & TPM_CENTERALIGN) ? (rect.left + rect.right) / 2
                : rect.left;
        int y = (m_menuAlingment & TPM_BOTTOMALIGN) ? rect.top
                : (m_menuAlingment & TPM_VCENTERALIGN) ? (rect.top + rect.bottom) / 2
                : rect.bottom;
        m_popup.Show(m_hWnd, x, y, m_menuItems, m_menuWidth, m_menuAlingment);
        m_theme.SwapFocus(m_popup.GetHwnd());
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    void Button::HandleClick()
    {
        if (!m_menuItems.empty())
        {
            m_theme.SwapFocus(m_hWnd);
            ShowMenu();
            return;
        }

        OnButtonDown.Notify();
        OnChanged.Notify();
    }

    SIZE Button::GetMinSize()
    {
        RECT clientRect;
        GetClientRect(m_hWnd, &clientRect);

        HDC hdc = GetWindowDC(m_hWnd);
        Graphics graphics(hdc);

        StringFormat format;
        long align = GetWindowLong(m_hWnd, GWL_STYLE) & BS_CENTER;
        format.SetAlignment(align == BS_LEFT ? StringAlignmentNear : align == BS_RIGHT ? StringAlignmentFar : StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        format.SetTrimming(StringTrimmingNone);
        format.SetFormatFlags(StringFormatFlagsNoWrap);

        Font textFont(hdc, m_theme.GetFont_Text());
        Font iconFont(hdc, m_isSmallIcon ? m_theme.GetFont_Glyph() : m_theme.GetFont_GlyphNormal());

        RectF bounds;
        bounds.Width = 100000;
        bounds.Height = 100000;

        RectF textRect{};
        RectF iconRect{};
        if (!m_text.empty())
        {
            graphics.MeasureString(m_text.c_str(), -1, &textFont, bounds, &format, &textRect);
        }
        if (!m_icon.empty())
        {
            iconRect = MeasureIconLayers(graphics, m_icon, iconFont, format);
        }

        float spacing = (!m_text.empty() && !m_icon.empty()) ? m_theme.DpiScaleF(6.0f) : 0.0f;
        const float progressDiameter = iconRect.Height + m_theme.DpiScaleF(6.0f);
        float iconWidth = m_showProgress ? (std::max)(iconRect.Width, progressDiameter) : iconRect.Width;
        float width = textRect.Width + iconWidth + spacing;
        float height = (std::max)(textRect.Height, m_showProgress ? progressDiameter : iconRect.Height);
        ReleaseDC(m_hWnd, hdc);

        return SIZE{(long)width, (long)height};
    }

    Button::~Button()
    {
        if (m_hWnd)
        {
            m_popup.Hide();
            DestroyWindow(m_hWnd);
            m_hWnd = nullptr;
        }
    }

    void Button::Animate(int startAngle, int stopAngle, int duration, bool bInfinite)
    {
        ClearProgress();
        bool active = m_animationTimer;

        m_startAngle = startAngle;
        m_endAngle = stopAngle;
        m_stepAngle = (stopAngle - startAngle) * 10 / duration;
        if (!m_stepAngle)
        {
            m_stepAngle = m_endAngle > m_startAngle ? 1 : -1;
        }
        m_animationLoop = bInfinite;
        if (!active)
        {
            m_animationTimer = SetTimer(m_hWnd, 1, 10, NULL) ? 1 : 0;
            SetAngle(m_startAngle);
        }
    }

    void Button::CompleteAnimation()
    {
        m_animationLoop = false;
    }

    void Button::CancelAnimation(int endAngle)
    {
        if (m_animationTimer)
            KillTimer(m_hWnd, m_animationTimer);
        m_animationTimer = 0;
        SetAngle(endAngle);
    }

    Button& Button::SetProgress(double progress)
    {
        progress = progress < 0.0 ? -1.0 : std::clamp(progress, 0.0, 1.0);
        if (m_showProgress && m_progress == progress)
            return *this;

        if (m_animationTimer || m_iconAngle)
            CancelAnimation();
        if (m_progressTimer)
        {
            KillTimer(m_hWnd, m_progressTimer);
            m_progressTimer = 0;
        }
        m_showProgress = true;
        m_progress = progress;
        if (progress < 0.0)
        {
            m_progressStarted = GetTickCount64();
            m_progressTimer = SetTimer(m_hWnd, 2, 16, NULL) ? 2 : 0;
        }
        InvalidateRect(m_hWnd, NULL, FALSE);
        return *this;
    }

    Button& Button::ClearProgress()
    {
        if (!m_showProgress)
            return *this;

        if (m_progressTimer)
        {
            KillTimer(m_hWnd, m_progressTimer);
            m_progressTimer = 0;
        }
        m_showProgress = false;
        InvalidateRect(m_hWnd, NULL, FALSE);
        return *this;
    }

    LRESULT Button::OnTimer(UINT timerId)
    {
        if (timerId == 2)
        {
            if (m_progressTimer)
                InvalidateRect(m_hWnd, NULL, FALSE);
            return 0;
        }
        if (timerId != 1 || !m_animationTimer)
            return 0;

        int nextAngle = m_iconAngle + m_stepAngle;
        if ( (m_endAngle > m_startAngle && nextAngle >= m_endAngle)
            || (m_endAngle < m_startAngle && nextAngle <= m_endAngle))
        {
            if (!m_animationLoop)
            {
                CancelAnimation(m_endAngle);
                return 0;
            }
            else
            {
                nextAngle = m_startAngle;
            }
        }
        SetAngle(nextAngle);
        return 0;
    }

    LRESULT Button::ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {

        Button *This = reinterpret_cast<Button *>(dwRefData);
        This->HandleMouseEvents(hWnd, uMsg, lParam);
        switch (uMsg)
        {
            case WM_SETCURSOR:
                if ((GetWindowLong(hWnd, GWL_STYLE) & BS_TYPEMASK) == BS_COMMANDLINK)
                {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                    return TRUE;
                }
                break;
            case WM_TIMER:
                return This->OnTimer((UINT)wParam);
            case WM_MOUSEMOVE:
            case WM_MOUSELEAVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
                This->HandleMouse(hWnd, uMsg);
                return 0;

            case WM_GETDLGCODE:
                if ( wParam == VK_SPACE || wParam ==VK_RETURN || wParam ==VK_GAMEPAD_A
                    || wParam == VK_LEFT  || wParam == VK_UP || wParam == VK_RIGHT || wParam == VK_DOWN)
                {
                    return DLGC_WANTALLKEYS;
                }
                break;

            case WM_KEYDOWN:
                if (This->m_theme.IsKeyboardFocused() &&
                    (wParam == VK_SPACE || wParam == VK_RETURN || wParam == VK_GAMEPAD_A))
                {
                    This->OnClick.Notify();
                    return 0;
                }
                else if (wParam == VK_LEFT  || wParam == VK_UP
                        || wParam == VK_RIGHT || wParam == VK_DOWN)
                {
                    This->m_theme.KeyboardNavigate((WORD)wParam);
                    return 0;
                }
                break;

            case WM_PAINT:
            case WM_PRINTCLIENT:
                {
                    FluentDesign::DoubleBuferedPaint paint(hWnd, uMsg == WM_PRINTCLIENT ? (HDC)wParam : nullptr);

                    RECT rect = paint.ClientRect();
                    This->DrawButton(hWnd, paint.MemDC(), rect);
                }
                return 0;

            case WM_PRINT:
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
            case WM_ERASEBKGND:
                return 1;

            case WM_NCDESTROY:
                This->m_animationTimer = 0;
                This->m_progressTimer = 0;
                RemoveWindowSubclass(hWnd, ButtonSubclassProc, uIdSubclass);
                break;
        }

        // Call original window procedure for default handling
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    void Button::HandleMouse(HWND hWnd, UINT uMsg)
    {
        switch (uMsg)
        {
        case WM_MOUSEMOVE:
            if (!m_buttonMouseOver)
            {
                m_buttonMouseOver = true;
                InvalidateRect(hWnd, NULL, FALSE);

                // Track mouse leave
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT)};
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
            }
            break;

        case WM_MOUSELEAVE:
            m_buttonMouseOver = false;
            InvalidateRect(hWnd, NULL, FALSE);
            SendMessage(GetParent(hWnd), WM_NCMOUSELEAVE, 0, 0);
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
        bool pressed = m_mousePressed || m_popup.IsVisible();

        // Convert RECT to GDI+ RectF
        RectF gdiRect((REAL)rect.left, (REAL)rect.top,
                    (REAL)(rect.right - rect.left),
                    (REAL)(rect.bottom - rect.top));

        // Calculate button rectangle using GDI+ RectF
        RectF br = gdiRect;
        br.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));

        // Determine colors based on state
        Color borderColor(m_theme.GetColor(
                pressed ? Theme::Colors::ButtonBorderPressed
            : m_buttonMouseOver ? Theme::Colors::ButtonBorderHover
                                : Theme::Colors::ButtonBorder
        ));

        Color backColor(m_theme.GetColor(
                pressed ? m_backgroundPressedColor
            : m_buttonMouseOver ? m_backgroundHoverColor
                                : m_backgroundNormalColor
        ));

        if (m_bFlat)
        {
            borderColor = backColor;
        }

        if ( m_bFlat && focused && m_theme.IsKeyboardFocused() && m_buttonMouseOver)
        {
            br.Inflate(m_theme.DpiScaleF(-1), m_theme.DpiScaleF(-1));
        }

        if (!m_bFlat || pressed || m_buttonMouseOver)
        {
            // Draw outer rounded rectangle (track)
            Pen borderPen(borderColor, (REAL)m_theme.DpiScale(1));
            SolidBrush backBrush(backColor);

            // For rounded rectangles in GDI+, we need to use GraphicsPath
            if (m_bSquare && !(m_bFlat && focused && m_buttonMouseOver && m_theme.IsKeyboardFocused()))
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

        Font textFont(hdc, m_theme.GetFont_Text());
        Font iconFont(hdc, m_isSmallIcon ? m_theme.GetFont_Glyph() : m_theme.GetFont_GlyphNormal());

        Color textColor(m_theme.GetColor(
            ! enabled           ? Theme::Colors::TextDisabled
            : pressed           ? m_textPressedColor
            : m_buttonMouseOver ? m_textHoverColor
                                : m_textNormalColor
        ));

        SolidBrush textBrush(textColor);
        br = gdiRect;
        br.Inflate(-m_theme.DpiScaleF(1), -m_theme.DpiScaleF(1));
        StringFormat format;
        long align = GetWindowLong(m_hWnd, GWL_STYLE) & BS_CENTER;
        format.SetAlignment(align == BS_LEFT ? StringAlignmentNear : align == BS_RIGHT ? StringAlignmentFar : StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        format.SetTrimming(StringTrimmingNone);
        format.SetFormatFlags(StringFormatFlagsNoWrap);

        RectF measureBounds;
        measureBounds.Width = 100000;
        measureBounds.Height = 100000;

        RectF textRect{};
        RectF iconRect{};
        if (!m_text.empty())
        {
            graphics.MeasureString(m_text.c_str(), -1, &textFont, measureBounds, &format, &textRect);
        }
        if (!m_icon.empty())
        {
            iconRect = MeasureIconLayers(graphics, m_icon, iconFont, format);
        }

        const float spacing = (!m_text.empty() && !m_icon.empty()) ? m_theme.DpiScaleF(6.0f) : 0.0f;
        const float progressDiameter = iconRect.Height + m_theme.DpiScaleF(6.0f);
        const float iconWidth = m_showProgress ? (std::max)(iconRect.Width, progressDiameter) : iconRect.Width;
        const float contentWidth = iconWidth + spacing + textRect.Width;
        float x = br.X;
        if (align == BS_CENTER)
        {
            x = br.X + (br.Width - contentWidth) / 2.0f;
        }
        else if (align == BS_RIGHT)
        {
            x = br.GetRight() - contentWidth;
        }

        if (!m_icon.empty())
        {
            RectF iconDraw = br;
            iconDraw.X = x + (iconWidth - iconRect.Width) / 2.0f;
            iconDraw.Width = iconRect.Width;

            if (m_showProgress)
            {
                const float thickness = m_theme.DpiScaleF(2.0f);
                const float diameter = progressDiameter;
                RectF progressRect(x + (iconWidth - diameter) / 2.0f, br.Y + (br.Height - diameter) / 2.0f, diameter, diameter);
                progressRect.Inflate(-thickness / 2.0f, -thickness / 2.0f);
                Pen progressPen(Color(m_theme.GetColor(Theme::Colors::TextAccented)), thickness);
                if (m_progress >= 1.0)
                {
                    graphics.DrawEllipse(&progressPen, progressRect);
                }
                else
                {
                    const float startAngle = m_progress < 0.0
                        ? -90.0f + float((GetTickCount64() - m_progressStarted) % 1000) * 360.0f / 1000.0f : -90.0f;
                    const float sweepAngle = m_progress < 0.0 ? 270.0f : float(m_progress * 360.0);
                    if (sweepAngle > 0.0f)
                        graphics.DrawArc(&progressPen, progressRect, startAngle, sweepAngle);
                }
            }

            if (m_iconAngle && !m_showProgress)
            {
                const float cx = iconDraw.X + iconDraw.Width / 2.0f;
                const float cy = iconDraw.Y + iconDraw.Height / 2.0f;
                graphics.TranslateTransform(cx, cy);
                graphics.RotateTransform((REAL)m_iconAngle);
                iconDraw.X -= cx;
                iconDraw.Y -= cy;
            }
            SolidBrush iconBrush(m_showProgress || m_animationTimer ? Color(m_theme.GetColor(m_textNormalColor)) : textColor);
            for (const wchar_t glyph : m_icon)
                graphics.DrawString(&glyph, 1, &iconFont, iconDraw, &format, &iconBrush);
            graphics.ResetTransform();
            x += iconWidth + spacing;
        }

        if (!m_text.empty())
        {
            RectF textDraw = br;
            textDraw.X = x;
            textDraw.Width = textRect.Width;
            graphics.DrawString(m_text.c_str(), -1, &textFont, textDraw, &format, &textBrush);
        }
        graphics.ResetTransform();

        return;
    }
    void Button::UpdateLayout()
    {
        SetWindowPos(m_hWnd, 0, 0, 0,
                     m_theme.DpiScale(m_designWidth),
                     m_theme.DpiScale(m_designHeight),
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    }
}
