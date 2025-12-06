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
#include <filesystem>

#include "AppUninstaller.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/Window.hpp"
#include "ToolsEx/Admin.hpp"
#include "Tools/Process.hpp"
#include "ToolsEx/ProcessEx.hpp"
#include "ToolsEx/TaskManager.hpp"
#include "Tools/Registry.hpp"


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

    if (lpCmdLine[0])
    {
        AnyFSE::AppUninstaller().Uninstall();
        return 0;
    }
    else
    {
        // Initialize common controls
        INITCOMMONCONTROLSEX icex = {sizeof(icex)};
        icex.dwICC = ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icex);
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        return (int)AnyFSE::AppUninstaller().Show(hInstance);
    }
}
namespace AnyFSE
{
    Logger log = LogManager::GetLogger("Uninstaller");

    INT_PTR AppUninstaller::Show(HINSTANCE hInstance)
    {
        LogManager::Initialize("AnyFSE/Uninstaller");

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
            log.Error(log.APIError(),"Cant create uninstaller dialog)");
        }
        return res;
    }

    void AppUninstaller::CenterDialog(HWND hwnd)
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

    INT_PTR AppUninstaller::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        AppUninstaller *This = nullptr;

        if (msg == WM_INITDIALOG)
        {
            // Store 'this' pointer in window user data
            This = (AppUninstaller *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)This);
            This->m_hDialog = hwnd;
        }
        else
        {
            // Get 'this' pointer from window user data
            This = (AppUninstaller *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (This)
        {
            return This->InstanceDialogProc(hwnd, msg, wParam, lParam);
        }

        return FALSE;
    }

    INT_PTR AppUninstaller::InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

    void AppUninstaller::OnInitDialog(HWND hwnd)
    {
        std::wstring title = L"AnyFSE Uninstaller";
        SetWindowText(hwnd, title.c_str());

        CreatePage();

        if (!ToolsEx::Admin::IsRunningAsAdministrator() && !ToolsEx::Admin::RequestAdminElevation()
        )
        {
            ShowErrorPage(
                L"Insufficient permissions",
                L"Escalated privileges is required to remove AnyFSE "
                L"for removing schedulled autorun task.\n\n"
                L"Please run uninstaller as Administrator "
                L"and allow it in User Account Control (UAC) prompt.",
                Icon_Permission);
        }
        else
        {
            ShowWelcomePage();
        }
    }

    void AppUninstaller::OnPaint(HWND hwnd)
    {
        FluentDesign::DoubleBuferedPaint paint(hwnd);
        DrawDialog(paint.MemDC(), paint.ClientRect());
    }

    void AppUninstaller::OnErase(HDC hdc, HWND child)
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

    void AppUninstaller::DrawDialog(HDC hdc, RECT clientRect)
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

    void AppUninstaller::OnCancel()
    {
        EndDialog(m_hDialog, IDCANCEL);
    }

    namespace fs = std::filesystem;

    std::list<HWND> AppUninstaller::CreatePage()
    {
        RECT rc;
        GetClientRect(m_hDialog, &rc);

        rc.left += m_theme.DpiScale(Layout_ImageWidth + Layout_Margins * 2);
        InflateRect(&rc, -m_theme.DpiScale(Layout_Margins), -m_theme.DpiScale(Layout_Margins));
        int width = rc.right - rc.left;

        std::list<HWND> page;

        page.push_back(m_imageStatic.Create(m_hDialog,
            rc.left - m_theme.DpiScale(Layout_ImageWidth + Layout_Margins),
            rc.top + m_theme.DpiScale(Layout_ImageTop),
            m_theme.DpiScale(Layout_ImageWidth),
            m_theme.DpiScale(Layout_ImageWidth)
        ));

        page.push_back( m_captionStatic.Create(m_hDialog,
            rc.left,
            rc.top,
            width,
            m_theme.DpiScale(Layout_CaptionHeight)
        ));

        page.push_back( m_textStatic.Create(m_hDialog,
            rc.left,
            rc.top + m_theme.DpiScale(Layout_CaptionHeight),
            width,
            rc.bottom - rc.top - m_theme.DpiScale(Layout_CaptionHeight + Layout_ButtonHeight + Layout_ButtonPadding)
        ));

        page.push_back( m_leftButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth * 2 + Layout_ButtonPadding),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        page.push_back( m_rightButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ));

        m_captionStatic.SetLarge(true);
        m_captionStatic.SetColor(Theme::Colors::TextAccented);

        return page;
    }

    void AppUninstaller::ShowPage(
        const std::wstring &icon,
        const std::wstring &caption,
        const std::wstring &text,
        const std::wstring &buttonRight,
        const std::function<void()> &callbackRight,
        const std::wstring &buttonLeft,
        const std::function<void()> &callbackLeft)
    {
        if (icon.empty())
        {
            wchar_t modulePath[MAX_PATH];
            GetModuleFileName(NULL, modulePath, MAX_PATH);
            m_imageStatic.LoadIcon(modulePath, 128);
        }
        else
        {
            m_imageStatic.LoadIcon(icon, 128);
        }

        m_captionStatic.SetText(caption);
        m_textStatic.SetText(text);
        m_rightButton.SetText(buttonRight);
        m_rightButton.OnChanged = callbackRight;

        if( !buttonLeft.empty())
        {
            m_leftButton.SetText(buttonLeft);
            m_leftButton.OnChanged = callbackLeft;
            m_leftButton.Show(true);
        }
        else
        {
            m_leftButton.Show(false);
        }

        RedrawWindow(m_hDialog, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
    }

    void AppUninstaller::ShowWelcomePage()
    {
        ShowPage(Icon_Delete,
            L"Remove AnyFSE",
            std::wstring(
                L"This will remove AnyFSE from your system."
                L"\n\nClick Uninstall to proceed or Cancel to exit."
            ),
            L"Cancel", delegate(OnCancel),
            L"Uninstall", delegate(OnUninstall)
        );
    }

    void AppUninstaller::ShowCompletePage()
    {
        ShowPage(Icon_Done,
            L"Done",
            L"AnyFSE was removed from your system",
            L"Done", delegate(OnDone)
        );
    }

    void AppUninstaller::ShowErrorPage(const std::wstring &caption, const std::wstring &text, const std::wstring &icon)
    {
        ShowPage(icon.empty() ? Icon_Error : icon,
                 caption,
                 text,
                 L"Close", delegate(OnCancel));
    }

    #define TRY(hr) if (FAILED((hr))) break;
    #define FREE(f) if (f) f->Release();

    void AppUninstaller::Uninstall()
    {
        wchar_t moduleName[MAX_PATH + 1];
        GetModuleFileName(GetModuleHandle(NULL), moduleName, MAX_PATH);
        std::wstring path = fs::path(moduleName).parent_path().wstring();

        TerminateAnyFSE();
        ToolsEx::TaskManager::RemoveTask();

        bool removeDir = DeleteFiles(path);

        Registry::DeleteKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AnyFSE");

        AutoDeleteSelf(path, removeDir);
    }

    void AppUninstaller::OnUninstall()
    {
        try
        {
            Uninstall();
        }
        catch(const std::exception& e)
        {
            ShowErrorPage(L"Uninstallation Error", Unicode::to_wstring(e.what()));
            return;
        }
        ShowCompletePage();
    }

    std::list<std::wstring> AppUninstaller::ListDir(const std::wstring& path, const std::wstring& mask)
    {
        std::list<std::wstring> result;
        std::wstring searchPath = path + L"\\" + mask;

        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            do {
                if (!(findData.cFileName[0] == L'.'
                    && findData.cFileName[1] == L'\0'
                    || findData.cFileName[1] == L'.'
                    && findData.cFileName[2] == L'\0'))
                {
                    result.push_back(path + L"\\" + findData.cFileName);
                }
            } while (FindNextFile(hFind, &findData) != 0);

            FindClose(hFind);
        }
        return result;
    }

    bool AppUninstaller::DeleteFiles(const std::wstring& path)
    {
        for (auto& path : ListDir(path, L"AnyFSE.*"))
        {
            fs::remove_all(path);
        }

        for (auto& path : ListDir(path, L"*.pdb"))
        {
            fs::remove_all(path);
        }

        if (fs::exists(path + L"\\splash") && fs::is_empty(path + L"\\splash"))
        {
            fs::remove(path + L"\\splash");
        }

        return ListDir(path, L"*").size() <= 1;
    }

    bool AppUninstaller::AutoDeleteSelf(const std::wstring& path, bool deleteFolder)
    {
        std::wstring batchPath = fs::temp_directory_path().wstring() + L"\\unins000_anyfse_clenup.bat";

        std::wofstream batch(batchPath);
        if (!batch.is_open()) return false;

        batch << L"@echo off\n";
        batch << L"chcp 65001 >nul\n";
        batch << L"echo Cleaning up...\n";
        batch << L"timeout /t 2 /nobreak >nul\n\n";

        batch << L":waitloop\n";
        batch << L"tasklist /fi \"PID eq " << GetCurrentProcessId() << L"\" | find \"" << GetCurrentProcessId() << L"\" >nul\n";
        batch << L"if not errorlevel 1 (\n";
        batch << L"  timeout /t 1 /nobreak >nul\n";
        batch << L"  goto waitloop\n";
        batch << L")\n\n";

        // Delete this batch file
        batch << L"del /f /q \"" << path << L"\\unins000.exe\"\n";


        if (deleteFolder)
        {
            // Delete installation directory
            batch << L"if exist \"" << path << L"\" (\n";
            batch << L"  echo Deleting: " << path << L"\n";
            batch << L"  rd /q \"" << path << L"\"\n";
            batch << L")\n\n";
        }

        batch << L"del /f /q \"" << batchPath << L"\"\n";

        batch << L"echo Uninstallation complete!\n";
        batch << L"timeout /t 3\n";

        batch.close();

        // Execute batch
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpFile = batchPath.c_str();
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (ShellExecuteExW(&sei))
        {
            WaitForSingleObject(sei.hProcess, 1000);
            CloseHandle(sei.hProcess);
        }

        return true;
    }


    bool AppUninstaller::TerminateAnyFSE()
    {
        std::set<DWORD> handles;
        if (Process::FindAllByName(L"AnyFSE.exe", handles))
        {
            for (auto& handle: handles)
            {
                ProcessEx::Kill(handle);
            }
        }
        if (Process::FindAllByName(L"AnyFSE.Service.exe", handles))
        {
            for (auto& handle: handles)
            {
                ProcessEx::Kill(handle);
            }
        }
        return true;
    }
    void AppUninstaller::OnDone()
    {
        EndDialog(m_hDialog, IDOK);
    }
}