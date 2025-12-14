#include "Dialog.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/Window.hpp"

namespace FluentDesign
{
    void Dialog::CenterDialog(HWND hwnd)
    {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        SIZE sz = GetDialogDesignSize();
        sz.cx = m_theme.DpiScale(sz.cx);
        sz.cy = m_theme.DpiScale(sz.cy);

        // Calculate center position
        int x = (screenWidth - sz.cx) / 2;
        int y = (screenHeight - sz.cy) / 2;

        // Move dialog to center
        SetWindowPos(hwnd, NULL, x, y, sz.cx, sz.cy, SWP_NOZORDER);
    }

    BOOL Dialog::OnInitDialog(HWND hwnd)
    {
        m_theme.AttachDlg(hwnd);
        CenterDialog(hwnd);
        Create();

        m_theme.DpiUnscaleChilds(hwnd, m_designedPositions);
        return FALSE;
    }

    BOOL Dialog::OnDpiChanged(HWND hwnd)
    {
        m_theme.DpiScaleChilds(hwnd, m_designedPositions);
        RedrawWindow(m_hDialog, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE);
        return FALSE;
    }

    BOOL Dialog::OnPaint(HWND hwnd)
    {
        using namespace Gdiplus;
        using namespace FluentDesign;

        FluentDesign::DoubleBuferedPaint paint(hwnd);
        Graphics graphics(paint.MemDC());
        RectF rect = ToRectF(paint.ClientRect());
        rect.Inflate(1, 1);

        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        RectF panelRect = rect;
        panelRect.Height = rect.Height - m_theme.DpiScaleF(GetFooterDesignHeight()) + 1;

        SolidBrush panelBrush(m_theme.GetColor(GetDialogColor()));
        graphics.FillRectangle(&panelBrush, panelRect);

        RectF footerRect = rect;
        footerRect.Y = panelRect.GetBottom() - 1;
        footerRect.Height = m_theme.DpiScaleF(GetFooterDesignHeight());

        SolidBrush footerBrush(m_theme.GetColor(GetFooterColor()));
        graphics.FillRectangle(&footerBrush, footerRect);

        for (auto &a : m_designedPositions)
        {
            if (IsWindowVisible(a.first))
            {
                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, a.first);
            }
        }
        return TRUE;
    }

    BOOL Dialog::OnErase(HDC hdc, HWND child)
    {
        RECT dialogRect;
        GetClientRect(m_hDialog, &dialogRect);

        if(child)
        {
            RECT clientRect;
            GetClientRect(child, &clientRect);

            RECT childRect = clientRect;
            Window::GetChildRect(child, &childRect);

            float panelHeight = dialogRect.bottom - m_theme.DpiScaleF(GetFooterDesignHeight());

            HBRUSH hBrush = CreateSolidBrush(m_theme.GetColorRef(
                childRect.top >= panelHeight ? GetFooterColor() : GetDialogColor()
            ));

            FillRect(hdc, &clientRect, hBrush);
            DeleteObject(hBrush);

            if ( child == GetFocus())
            {
                m_theme.DrawChildFocus(hdc, child, child);
            }
        }
        SetWindowLong(m_hDialog, DWLP_MSGRESULT, 1);
        return TRUE;
    }

    BOOL Dialog::OnNcCalcSize(HWND hwnd, NCCALCSIZE_PARAMS *params)
    {
        InflateRect(&params->rgrc[0], -1, -1);
        SetWindowLong(hwnd, DWLP_MSGRESULT, 0);
        return TRUE;
    }

    INT_PTR Dialog::InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            return OnInitDialog(hwnd);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                OnCancel();
                return TRUE;
            }
            break;
        case WM_NCCALCSIZE:
            return OnNcCalcSize(hwnd, (NCCALCSIZE_PARAMS*) lParam);
        case WM_DPICHANGED:
            return OnDpiChanged(hwnd);
        case WM_PAINT:
            return OnPaint(hwnd);
        case WM_ERASEBKGND:
            return OnErase((HDC)wParam, (HWND)lParam);
        case WM_NCHITTEST:
            SetWindowLong(hwnd, DWLP_MSGRESULT, HTCAPTION);
            return TRUE;
        }
        return FALSE;
    }

    INT_PTR Dialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        Dialog *This = nullptr;

        if (msg == WM_INITDIALOG)
        {
            // Store 'this' pointer in window user data
            This = (Dialog *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)This);
            This->m_hDialog = hwnd;
        }
        else
        {
            // Get 'this' pointer from window user data
            This = (Dialog *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (This)
        {
            return This->InstanceDialogProc(hwnd, msg, wParam, lParam);
        }

        return FALSE;
    }

    INT_PTR Dialog::Show(HWND hParent)
    {
        size_t size = sizeof(DLGTEMPLATE) + sizeof(WORD) * 3; // menu, class, title
        HGLOBAL hGlobal = GlobalAlloc(GHND, size);
        if (!hGlobal) return -1;

        LPDLGTEMPLATE dlgTemplate = (LPDLGTEMPLATE)GlobalLock(hGlobal);
        if (!dlgTemplate) return -1;

        dlgTemplate->style = DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION;
        dlgTemplate->dwExtendedStyle = WS_EX_WINDOWEDGE;
        dlgTemplate->cdit = 0;  // No controls
        dlgTemplate->x = 0;
        dlgTemplate->y = 0;
        dlgTemplate->cx = 0;
        dlgTemplate->cy = 0;

        // No menu, class, or title
        WORD* ptr = (WORD*)(dlgTemplate + 1);
        *ptr++ = 0; // No menu
        *ptr++ = 0; // Default dialog class
        *ptr++ = 0; // No title

        GlobalUnlock(hGlobal);

        INT_PTR res = DialogBoxIndirectParam(GetModuleHandle(NULL), dlgTemplate, hParent, DialogProc, (LPARAM)this);

        GlobalFree(hGlobal);
        return res;
    }

    void Dialog::OnCancel()
    {
        EndDialog(m_hDialog, IDCANCEL);
    }

    void Dialog::OnOK()
    {
        EndDialog(m_hDialog, IDOK);
    }
}