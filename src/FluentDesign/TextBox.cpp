// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>



#include "TextBox.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

using namespace Gdiplus;
namespace FluentDesign
{

    TextBox::TextBox(FluentDesign::Theme& theme)
    : m_theme(theme)
    {
        theme.OnDPIChanged += [This = this]() { This->UpdateLayout(); };
    }

    TextBox::TextBox(FluentDesign::Theme &theme, HWND hParent, int x, int y, int width, int height)
    :TextBox(theme)
    {
        Create(hParent, x, y, width, height);
    }

    TextBox::~TextBox()
    {
        Destroy();
    }

    HWND TextBox::Create(HWND hParent, int x, int y, int width, int height)
    {
        m_hParent = hParent;
        m_designedWidth = m_theme.DpiUnscale(width);

        m_hContainer = CreateWindowEx(
            WS_EX_CONTROLPARENT,
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE,
            x, y,
            width, height,
            hParent,
            NULL,
            GetModuleHandle(NULL),
            this);

        if (!m_hContainer)
        {
            return NULL;
        }

        m_theme.RegisterChild(m_hContainer);
        SetWindowSubclass(m_hContainer, ContainerSubclassProc, 0, (DWORD_PTR)this);

        // Create the actual TextBox control
        m_hTextBox = CreateWindowEx(
            0,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
            m_theme.DpiScale(m_marginLeft), m_theme.DpiScale(m_marginTop),
            width - m_theme.DpiScale(m_marginLeft + m_marginRight),
            height - m_theme.DpiScale(m_marginTop + m_marginBottom),
            m_hContainer,
            NULL,
            GetModuleHandle(NULL),
            NULL);

        if (!m_hTextBox)
        {
            DestroyWindow(m_hContainer);
            m_hContainer = nullptr;
            return NULL;
        }

        UpdateLayout();

        SetWindowSubclass(m_hTextBox, TextBoxSubclassProc, 0, (DWORD_PTR)this);
        UpdateTextBoxColors();

        return m_hContainer;
    }

    void TextBox::Destroy()
    {
        if (m_hTextBox)
        {
            RemoveWindowSubclass(m_hTextBox, TextBoxSubclassProc, 0);
            DestroyWindow(m_hTextBox);
            m_hTextBox = nullptr;
        }
        if (m_hContainer)
        {
            RemoveWindowSubclass(m_hContainer, ContainerSubclassProc, 0);
            DestroyWindow(m_hContainer);
            m_hContainer = nullptr;
        }
    }

    std::wstring TextBox::GetText() const
    {
        if (!m_hTextBox)
            return L"";

        int length = GetWindowTextLength(m_hTextBox);
        if (length == 0)
            return L"";

        std::wstring text;
        text.resize(length + 1);
        GetWindowText(m_hTextBox, &text[0], length + 1);
        text.resize(length);

        return text;
    }

    void TextBox::SetText(const std::wstring &text)
    {
        if (m_hTextBox)
        {
            SetWindowText(m_hTextBox, text.c_str());
        }
    }

    void TextBox::UpdateLayout()
    {
        SendMessage(m_hTextBox, WM_SETFONT, (WPARAM)m_theme.GetFont_Text(), TRUE);

        SetWindowPos(m_hTextBox, 0, 0, 0,
            m_theme.DpiScale(m_designedWidth),
            m_theme.GetSize_Text() * 2 + m_theme.DpiScale(
                m_marginTop + m_marginBottom
            ),
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);

        RECT tbRect;
        SendMessage(m_hTextBox, EM_GETRECT, 0, (LPARAM) &tbRect);

        int tbHeight = tbRect.bottom - tbRect.top;

        SetWindowPos(m_hContainer, 0, 0, 0,
                    m_theme.DpiScale(m_designedWidth),
                    tbHeight + m_theme.DpiScale(
                        m_marginTop + m_marginBottom
                    ),
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        OnSize();
    }

    void TextBox::OnSize()
    {
        if (!m_hContainer || !m_hTextBox)
            return;

        RECT rect;
        GetClientRect(m_hContainer, &rect);

        SetWindowPos(
            m_hTextBox,
            NULL,
            m_theme.DpiScale(m_marginLeft),
            m_theme.DpiScale(m_marginTop),
            rect.right - m_theme.DpiScale(m_marginRight + m_marginLeft),
            rect.bottom - m_theme.DpiScale(m_marginTop + m_borderWidth * 5),
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void TextBox::UpdateTextBoxColors()
    {
        if (!m_hTextBox)
            return;

        // Force redraw to apply new colors
        InvalidateRect(m_hTextBox, NULL, TRUE);
    }

    void TextBox::Invalidate()
    {
        if (m_hContainer)
        {
            InvalidateRect(m_hContainer, NULL, TRUE);
        }
    }

    // Container window procedure
    LRESULT CALLBACK TextBox::ContainerSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        TextBox *This = reinterpret_cast<TextBox *>(dwRefData);

        if (This)
        {
            switch (msg)
            {
            case WM_PAINT:
            {
                DoubleBuferedPaint paint(hWnd);
                This->DrawContainer(paint.MemDC());
                return 0;
            }

            case WM_ERASEBKGND:
                return 1; // We handle background in WM_PAINT

            case WM_SETFOCUS:
                if (This->m_hTextBox)
                {
                    SetFocus(This->m_hTextBox);
                }
                break;
            case WM_MOUSEACTIVATE:
                SetFocus(This->m_hTextBox);
                return MA_ACTIVATE;

            case WM_SIZE:
                This->OnSize();
                break;

            case WM_COMMAND:
                // Forward notifications to parent
                if (HIWORD(wParam) == EN_SETFOCUS)
                {
                    This->m_hasFocus = true;
                    This->Invalidate();
                }
                else if (HIWORD(wParam) == EN_KILLFOCUS)
                {
                    This->m_hasFocus = false;
                    This->Invalidate();
                }
                else if (HIWORD(wParam) == EN_CHANGE)
                {
                    This->OnChanged.Notify();
                    break;
                }
                return SendMessage(This->m_hParent, msg, wParam, lParam);
            case WM_CTLCOLOREDIT:
            case WM_CTLCOLORSTATIC:
                {
                    HDC hdc = (HDC)wParam;

                    COLORREF back = This->m_hasFocus  ? This->m_theme.GetColorRef(Theme::Colors::EditFocus)
                                  : This->m_mouseOver ? This->m_theme.GetColorRef(Theme::Colors::EditHover)
                                                      : This->m_theme.GetColorRef(Theme::Colors::Edit);
                    SetBkColor(hdc, back);
                    ::SetTextColor(hdc, This->m_theme.GetColorRef(Theme::Colors::Text));

                    static HBRUSH hBrush = nullptr;
                    if (hBrush)
                        DeleteObject(hBrush);
                    hBrush = CreateSolidBrush(back);
                    return (LRESULT)hBrush;
                }

            case WM_DESTROY:
                RemoveWindowSubclass(hWnd, TextBoxSubclassProc, uIdSubclass);
                break;
            }
        }

        return DefSubclassProc(hWnd, msg, wParam, lParam);
    }

    // TextBox subclass procedure
    LRESULT CALLBACK TextBox::TextBoxSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        TextBox *pTextBox = reinterpret_cast<TextBox *>(dwRefData);

        switch (msg)
        {
        case WM_ERASEBKGND:
        {
            // HDC hdc = (HDC)wParam;
            // RECT rect;
            // GetClientRect(hWnd, &rect);

            // HBRUSH hBrush = CreateSolidBrush(pTextBox->m_backColor);
            // FillRect(hdc, &rect, hBrush);
            // DeleteObject(hBrush);

            return 1;
        }
        case WM_DESTROY:
            RemoveWindowSubclass(hWnd, TextBoxSubclassProc, uIdSubclass);
            break;
        }

        return DefSubclassProc(hWnd, msg, wParam, lParam);
    }

    void TextBox::DrawContainer(HDC memDC)
    {
        RECT rect;
        GetClientRect(m_hContainer, &rect);

        // Use GDI+ for rounded rectangles
        Gdiplus::Graphics graphics(memDC);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        Color accent(m_hasFocus ? m_theme.GetColor(Theme::Colors::EditAccentFocus)
                                : m_theme.GetColor(Theme::Colors::EditAccent)
        );


        Color back(m_hasFocus ? m_theme.GetColor(Theme::Colors::EditFocus)
                : m_mouseOver ? m_theme.GetColor(Theme::Colors::EditHover)
                              : m_theme.GetColor(Theme::Colors::Edit)
        );

        Color border(m_theme.GetColor(m_hasFocus ? Theme::Colors::EditBorderFocus : Theme::Colors::EditBorder));

        RectF accentRect((REAL)rect.left, (REAL)rect.top,
                ( REAL)(rect.right - rect.left),
                ( REAL)(rect.bottom - rect.top));

        accentRect.Inflate(-m_theme.DpiScaleF(m_borderWidth), -m_theme.DpiScaleF(2 * m_borderWidth));

        RectF clientRect = accentRect;
        clientRect.Height = clientRect.Height - m_theme.DpiScale(m_borderWidth);

        clientRect.Height -= m_theme.DpiScaleF(m_hasFocus ? 1 : 0);
        accentRect.Inflate(-m_theme.DpiScaleF(m_borderWidth), 0);

        Pen accentPen(accent, 0*m_theme.DpiScaleF(m_borderWidth));
        SolidBrush accentBrush(accent);
        RoundRect(graphics, accentRect, m_theme.DpiScaleF(m_cornerRadius), &accentBrush, accentPen);

        Pen borderPen(border, m_theme.DpiScaleF(m_borderWidth));
        SolidBrush controlBrush(back);
        RoundRect(graphics, clientRect, m_theme.DpiScaleF(m_cornerRadius), &controlBrush, borderPen);

    }

} // namespace FluentDesign