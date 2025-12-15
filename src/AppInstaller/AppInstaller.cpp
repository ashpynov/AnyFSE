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
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <string>
#include <vector>
#include <fstream>
#include <thread>

#include "AppInstaller.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/Window.hpp"
#include "ToolsEx/Admin.hpp"
#include "AppControl/GamingExperience.hpp"

#pragma comment(lib, "delayimp.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "ComCtl32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "gdi32.lib")


#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Initialize common controls
    INITCOMMONCONTROLSEX icex = {sizeof(icex)};
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    int result = (int)AnyFSE::AppInstaller().Show(hInstance, !_wcsnicmp(lpCmdLine, L"/autoupdate", 11));

    return result;
}
namespace AnyFSE
{
    Logger log = LogManager::GetLogger("Installer");

    INT_PTR AppInstaller::Show(HINSTANCE hInstance, bool bAutoUpdate)
    {
        m_isUpdate = bAutoUpdate;

        if (bAutoUpdate)
        {
            AutoDeleteSelf();
        }

        size_t size = sizeof(DLGTEMPLATE) + sizeof(WORD) * 3; // menu, class, title
        HGLOBAL hGlobal = GlobalAlloc(GHND, size);

        LPDLGTEMPLATE dlgTemplate = (LPDLGTEMPLATE)GlobalLock(hGlobal);

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

        INT_PTR res = DialogBoxIndirectParam(hInstance, dlgTemplate, NULL, DialogProc, (LPARAM)this);

        GlobalFree(hGlobal);
        if (res == -1)
        {
            log.Error(log.APIError(),"Cant create settings dialog)");
        }
        return res;
    }

    void AppInstaller::CenterDialog(HWND hwnd)
    {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        int cx = m_theme.DpiScale(Layout_DialogWidth);
        int cy = m_theme.DpiScale(Layout_DialogHeight);

        // Calculate center position
        int x = (screenWidth - cx) / 2;
        int y = (screenHeight - cy) / 2;

        // Move dialog to center
        SetWindowPos(hwnd, NULL, x, y, cx, cy, SWP_NOZORDER);
    }

    INT_PTR AppInstaller::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        AppInstaller *This = nullptr;

        if (msg == WM_INITDIALOG)
        {
            // Store 'this' pointer in window user data
            This = (AppInstaller *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)This);
            This->m_hDialog = hwnd;
        }
        else
        {
            // Get 'this' pointer from window user data
            This = (AppInstaller *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (This)
        {
            return This->InstanceDialogProc(hwnd, msg, wParam, lParam);
        }

        return FALSE;
    }

    INT_PTR AppInstaller::InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            {
                m_theme.AttachDlg(hwnd);
                CenterDialog(hwnd);
                OnInitDialog(hwnd);
                m_theme.DpiUnscaleChilds(hwnd, m_designedPositions);
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                OnCancel();
                return TRUE;
            }
            break;
        case WM_DPICHANGED:
            m_theme.DpiScaleChilds(hwnd, m_designedPositions);
            RedrawWindow(m_hDialog, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE);
            break;
        case WM_PAINT:
            OnPaint(hwnd);
            return TRUE;
        case WM_ERASEBKGND:
            OnErase((HDC)wParam, (HWND)lParam);
            return TRUE;
        }
        return FALSE;
    }

    void AppInstaller::OnInitDialog(HWND hwnd)
    {
        std::wstring title = std::wstring(L"AnyFSE ")
            + (m_isUpdate ? L"Updater v" : L"Installer v")
            + Unicode::to_wstring(APP_VERSION);
        SetWindowText(hwnd, title.c_str());

        CreatePage();

        if (!GamingExperience::ApiIsAvailable)
        {
            ShowErrorPage(
                L"Not supported",
                L"Installation is interrupted as soon as "
                L"Fullscreen Experiense API "
                L"was not detected on your system.\n\n"
                L"It is supported since Windows 25H2 version for Handheld Devices.",
                Icon_Error
            );
        }
        else if (!ToolsEx::Admin::IsRunningAsAdministrator() && !ToolsEx::Admin::RequestAdminElevation()
        )
        {
            ShowErrorPage(
                L"Insufficient permissions",
                L"Escalated privileges is required to install AnyFSE "
                L"for schedulle autorun task.\n\n"
                L"Please run installer as Administrator "
                L"and allow it in User Account Control (UAC) prompt.",
                Icon_Permission);
        }
        else
        {
            ShowWelcomePage();
        }
    }

    void AppInstaller::OnPaint(HWND hwnd)
    {
        FluentDesign::DoubleBuferedPaint paint(hwnd);
        DrawDialog(paint.MemDC(), paint.ClientRect());
    }

    void AppInstaller::OnErase(HDC hdc, HWND child)
    {
        RECT dialogRect;
        GetClientRect(m_hDialog, &dialogRect);

        if(child)
        {
            RECT clientRect;
            GetClientRect(child, &clientRect);

            RECT childRect = clientRect;
            Window::GetChildRect(child, &childRect);

            float panelHeight = dialogRect.bottom - m_theme.DpiScaleF(Layout_Margins + Layout_ButtonHeight + Layout_ButtonPadding);

            HBRUSH hBrush = CreateSolidBrush(m_theme.GetColorRef(
                childRect.top >= panelHeight ? Theme::Colors::Footer : Theme::Colors::Dialog
            ));

            FillRect(hdc, &clientRect, hBrush);
            DeleteObject(hBrush);

            if ( child == GetFocus())
            {
                m_theme.DrawChildFocus(hdc, child, child);
            }
        }
        SetWindowLong(m_hDialog, DWLP_MSGRESULT, 1);
    }

    void AppInstaller::DrawDialog(HDC hdc, RECT clientRect)
    {
        using namespace Gdiplus;
        using namespace FluentDesign;

        Graphics graphics(hdc);
        RectF rect = ToRectF(clientRect);
        rect.Inflate(1, 1);

        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        float footerHeight = m_theme.DpiScaleF(Layout_Margins + Layout_ButtonHeight + Layout_ButtonPadding);

        RectF panelRect = rect;
        panelRect.Height = rect.Height - footerHeight + 1;

        SolidBrush panelBrush(m_theme.GetColor(Theme::Colors::Dialog));
        graphics.FillRectangle(&panelBrush, panelRect);

        RectF footerRect = rect;
        footerRect.Y = panelRect.GetBottom() - 1;
        footerRect.Height = footerHeight;

        SolidBrush footerBrush(m_theme.GetColor(Theme::Colors::Footer));
        graphics.FillRectangle(&footerBrush, footerRect);

        for (auto &a : m_designedPositions)
        {
            if (IsWindowVisible(a.first))
            {
                m_theme.DrawChildFocus(hdc, m_hDialog, a.first);
            }
        }
    }

    void AppInstaller::OnCancel()
    {
        EndDialog(m_hDialog, IDCANCEL);
    }
}