
#include "ScrollView.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/GdiPlus.hpp"

namespace FluentDesign
{
    LRESULT ScrollView::ScrollViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        ScrollView *This = reinterpret_cast<ScrollView *>(dwRefData);
        switch (uMsg)
        {
            case WM_SETFOCUS:
            {
                SetFocus(GetWindow(hWnd, GW_CHILD));
            }
            break;

            case WM_SIZE:
            {
                HWND hChild = GetWindow(hWnd, GW_CHILD);
                while (hChild)
                {
                    RECT rc;
                    GetClientRect(hChild, &rc);
                    SendMessage(hChild, WM_SIZE, 0, MAKELONG(rc.right - rc.left, rc.bottom - rc.top));
                    hChild = GetWindow(hChild, GW_HWNDNEXT);
                }
            }
            break;

            case WM_NCPAINT:
            {
                HDC hdc = GetWindowDC(hWnd);
                using namespace Gdiplus;
                Graphics graphics(hdc);
                graphics.SetSmoothingMode(SmoothingModeAntiAlias);

                RECT rcWindow;
                GetWindowRect(hWnd, &rcWindow);

                // Convert to client-relative coordinates for non-client area
                int width = rcWindow.right - rcWindow.left;
                int height = rcWindow.bottom - rcWindow.top;

                // Calculate scrollbar rectangle (right side)
                RECT rcScrollbar = {
                    width - GetSystemMetrics(SM_CXVSCROLL),
                    0,
                    width,
                    height
                };

                RectF rect = ToRectF(rcScrollbar);
                rect.Inflate(0.5, 0.5);
                SolidBrush backBrush(This->m_theme.GetColor(Theme::Colors::Dialog));
                graphics.FillRectangle(&backBrush, rect);

                if (0)
                {
                    SolidBrush thumbBrush(This->m_theme.GetColor(Theme::Colors::ScrollThumb));
                    Pen thumbPen(This->m_theme.GetColor(Theme::Colors::ScrollThumb), 0.5);

                    rect.X = rect.GetRight() - 13.0f;
                    rect.Width = 7;
                    rect.Height = 400;
                    RoundRect(graphics, rect, rect.Width, &thumbBrush, thumbPen);
                }
                ReleaseDC(hWnd, hdc);
                return 0;
            }

            case WM_PAINT:
                {
                    FluentDesign::DoubleBuferedPaint paint(hWnd);
                }
                return 0;

            case WM_ERASEBKGND:
                {
                    if(lParam)
                    {
                        HWND parent = GetParent(hWnd);
                        if (parent)
                        {
                            SendMessage(parent, WM_ERASEBKGND, (WPARAM)wParam, (LPARAM)hWnd);
                        }
                    }
                }
                return 1;
            case WM_NCDESTROY:
                RemoveWindowSubclass(hWnd, ScrollViewSubclassProc, uIdSubclass);
                break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
    void ScrollView::SetOffset(int newOffset)
    {
    }

    ScrollView::ScrollView(Theme &theme)
        : m_theme(theme)
    {}

    ScrollView::ScrollView(Theme &theme, HWND hParent, int x, int y, int width, int height)
        : ScrollView(theme)
    {
        Create(hParent, x, y, width, height);
    }

    HWND ScrollView::Create(HWND hParent, int x, int y, int width, int height)
    {
        m_hScrollView = CreateWindowEx(
            WS_EX_CONTROLPARENT,
            L"STATIC",
            L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VSCROLL,
            x, y, width, height,
            hParent, NULL, GetModuleHandle(NULL), NULL);

        SetWindowSubclass(m_hScrollView, ScrollViewSubclassProc, 0, (DWORD_PTR)this);
        return m_hScrollView;
    }

    void ScrollView::SetContentHeight(int newHeight)
    {
        m_contentHeight = newHeight;
    }

    ScrollView::~ScrollView()
    {
    }
}