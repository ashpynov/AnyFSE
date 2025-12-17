#include "Tools/Window.hpp"
#include "FluentDesign/FluentControl.hpp"
#include "FluentControl.hpp"

namespace FluentDesign
{
    void FluentControl::SetAnchor(Align::Anchor anchor, GetParentRectFunc getParentRect)
    {

        m_anchor = anchor;
        m_getParentRect = getParentRect;

        if (GetParent(m_hWnd))
        {
            StoreDesign();
        }
    }

    void FluentControl::StoreDesign()
    {
        if (m_anchor==Align::None)
        {
            return;
        }

        RECT parentRECT;
        m_getParentRect(m_theme, GetParent(m_hWnd), &parentRECT);
        Gdiplus::RectF parentRect = m_theme.DpiUnscaleF(parentRECT);

        RECT controlRECT;
        Window::GetChildRect(m_hWnd, &controlRECT);
        Gdiplus::RectF controlRect = m_theme.DpiUnscaleF(controlRECT);

        m_designMargins.Left    = controlRect.GetLeft()   - Align::HSide(Align::LeftSide(m_anchor), parentRect, controlRect.GetRight());
        m_designMargins.Top     = controlRect.GetTop()    - Align::VSide(Align::TopSide(m_anchor), parentRect, controlRect.GetBottom());
        m_designMargins.Right   = controlRect.GetRight()  - Align::HSide(Align::RightSide(m_anchor), parentRect, controlRect.GetLeft());
        m_designMargins.Bottom  = controlRect.GetBottom() - Align::VSide(Align::BottomSide(m_anchor), parentRect, controlRect.GetTop());
    }

    HDWP FluentControl::ReflowControl(HDWP hdwp)
    {
        if (m_anchor==Align::None)
        {
            return hdwp;
        }

        RECT parentRECT;
        m_getParentRect(m_theme, GetParent(m_hWnd), &parentRECT);
        Gdiplus::RectF parentRect = m_theme.DpiUnscaleF(parentRECT);

        float l, t, r, b = 0.f;

        l = m_designMargins.Left    + Align::HSide(Align::LeftSide(m_anchor), parentRect, 0 );
        t = m_designMargins.Top     + Align::VSide(Align::TopSide(m_anchor), parentRect, 0 );
        r = m_designMargins.Right   + Align::HSide(Align::RightSide(m_anchor), parentRect, l );
        b = m_designMargins.Bottom  + Align::VSide(Align::BottomSide(m_anchor), parentRect, t );

        Gdiplus::RectF controlRect(l, t, r - l, b - t);
        RECT controlRECT = m_theme.DpiScale(controlRect);
        if (!hdwp)
        {
            Window::MoveWindow(m_hWnd, &controlRECT, FALSE);
        }
        else
        {
            hdwp = DeferWindowPos(hdwp, m_hWnd, nullptr,
                                  controlRECT.left, controlRECT.top,
                                  controlRECT.right - controlRECT.left,
                                  controlRECT.bottom - controlRECT.top,
                                  SWP_NOZORDER);
        }
        return hdwp;
    }

    void FluentControl::AnchoredSizePos(HWND hParent, int &x, int&y, int &cx, int& cy)
    {
        if (m_anchor == Align::None)
        {
            m_designMargins.Left = m_theme.DpiUnscaleF(x);
            m_designMargins.Top = m_theme.DpiUnscaleF(y);
            m_designMargins.Right = m_theme.DpiUnscaleF(cx);
            m_designMargins.Bottom = m_theme.DpiUnscaleF(cy);

            return;
        }

        RECT parentRECT;
        m_getParentRect(m_theme, hParent, &parentRECT);
        Gdiplus::RectF parentRect = Gdiplus::ToRectF(parentRECT);

        m_designMargins.Left = (float)x;
        m_designMargins.Top = (float)y;
        m_designMargins.Right = (float)cx;
        m_designMargins.Bottom = (float)cy;

        x = m_theme.DpiScale(x) + (int)Align::HSide(Align::LeftSide(m_anchor), parentRect, 0);
        y = m_theme.DpiScale(y) + (int)Align::VSide(Align::TopSide(m_anchor), parentRect, 0);

        cx = m_theme.DpiScale(cx);
        cy = m_theme.DpiScale(cy);

        if (Align::RightSide(m_anchor) != Align::None) // Cx is Right margin
        {
            cx += (int)Align::HSide(Align::RightSide(m_anchor), parentRect, 0) - x;
        }

        if (Align::BottomSide(m_anchor) != Align::None) // CY is Bottom margin
        {
            cy += (int)Align::VSide(Align::BottomSide(m_anchor), parentRect, 0) - y;
        }
    }

    void FluentControl::SetSize(LONG cx, LONG cy)
    {
        RECT rc;
        Window::GetChildRect(m_hWnd, &rc);

        if (cx != -1)
        {
            m_designMargins.Right = m_theme.DpiUnscaleF(cx);
        }

        if (cy != -1)
        {
            m_designMargins.Bottom = m_theme.DpiUnscaleF(cy);
        }
        ReflowControl(NULL);
        RedrawWindow(GetParent(m_hWnd), &rc, 0, RDW_INVALIDATE);
    }
}
