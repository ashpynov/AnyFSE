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
#include "AppInstaller/Admin.hpp"
#include "Tools/Process.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Paths.hpp"
#include "Tools/Packages.hpp"
#include "Tools/Localization.hpp"
#include "App/AppConstants.hpp"
#include "Ally/Services.hpp"
#include "AppInstaller/Certificate.hpp"


#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "ComCtl32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "gdi32.lib")


#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace
{
    struct UninstallOptions
    {
        bool silent = false;
        bool update = false;
    };

    UninstallOptions ParseUninstallOptions(LPWSTR lpCmdLine)
    {
        const std::wstring commandLine = lpCmdLine ? lpCmdLine : L"";
        UninstallOptions options;
        options.silent = commandLine.find(L"/s") != std::wstring::npos || commandLine.find(L"-s") != std::wstring::npos;
        options.update = commandLine.find(L"/u") != std::wstring::npos || commandLine.find(L"-u") != std::wstring::npos;
        return options;
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    const UninstallOptions options = ParseUninstallOptions(lpCmdLine);
    if (options.silent)
    {
        try
        {
            AnyFSE::AppUninstaller().Uninstall(options.update);
        }
        catch (...)
        {
            return 1;
        }
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

        Tools::Localization::InitializeFromLocales();


        size_t size = sizeof(DLGTEMPLATE) + sizeof(WORD) * 3; // menu, class, title
        HGLOBAL hGlobal = GlobalAlloc(GHND, size);

        LPDLGTEMPLATE dlgTemplate = (LPDLGTEMPLATE)GlobalLock(hGlobal);

        dlgTemplate->style = DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_MINIMIZEBOX;
        dlgTemplate->dwExtendedStyle = WS_EX_WINDOWEDGE | WS_EX_APPWINDOW;
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
        CreatePage();
        UpdateDialogTitle();

        if (!ToolsEx::Admin::IsRunningAsAdministrator() && !ToolsEx::Admin::RequestAdminElevation()
        )
        {
            ShowErrorPage(
                Translate(L"uninstallerInsufficientPermissionsCaption"),
                Translate(L"uninstallerInsufficientPermissionsDescription"),
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
        ).GetHwnd());

        page.push_back( m_rightButton.Create(m_hDialog,
            rc.right - m_theme.DpiScale(Layout_ButtonWidth),
            rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
            m_theme.DpiScale(Layout_ButtonWidth),
            m_theme.DpiScale(Layout_ButtonHeight)
        ).GetHwnd());

        page.push_back(m_languageButton
            .Create(m_hDialog,
                rc.left - m_theme.DpiScale(Layout_ImageWidth + Layout_Margins * 2),
                rc.bottom - m_theme.DpiScale(Layout_ButtonHeight),
                m_theme.DpiScale(Layout_ButtonHeight * 2),
                m_theme.DpiScale(Layout_ButtonHeight))
            .SetIcon(L"\xE164")
            .SetFlat(true)
            .GetHwnd()
        );

        PopulateLanguageMenu();
        m_languageButton.Show(false);

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
        m_imageStatic.LoadIcon(icon.empty() ? Tools::Paths::GetExeFileName() : icon, 128);

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
        m_languageButton.Show(true);
        ShowPage(Icon_Delete,
            Translate(L"uninstallerWelcomeCaption"),
            Translate(L"uninstallerWelcomeDescription"),
            Translate(L"cancelBtn"), delegate(OnCancel),
            Translate(L"uninstallBtn"), delegate(OnUninstall)
        );
    }

    void AppUninstaller::ShowCompletePage()
    {
        m_languageButton.Show(false);
        ShowPage(Icon_Done,
            Translate(L"doneBtn"),
            Translate(L"uninstallerDoneDescription"),
            Translate(L"doneBtn"), delegate(OnDone)
        );
    }

    bool AppUninstaller::DeleteFiles(const std::wstring& dir)
    {
        std::wstring moduleName = Unicode::to_lower(Paths::GetExeFileName());
        fs::path path(dir);

        std::list<std::wstring> to_delete;
        if (fs::is_directory(path))
        {
            for (const auto & entry : fs::directory_iterator(path))
            {
                std::wstring p = entry.path().wstring();
                if (p == L"." || p == L".." || Unicode::to_lower(p) == moduleName)
                {
                    continue;
                }
                to_delete.push_back(entry.path().wstring());
            }
        }
        for (auto& p : to_delete)
        {
            fs::is_directory(p) ? fs::remove_all(p) : fs::remove(p);
        }
        return true;
    }

    void AppUninstaller::ShowErrorPage(const std::wstring &caption, const std::wstring &text, const std::wstring &icon)
    {
        m_languageButton.Show(false);
        ShowPage(icon.empty() ? Icon_Error : icon,
                 caption,
                 text,
                 Translate(L"closeBtn"), delegate(OnCancel));
    }

    void AppUninstaller::UpdateDialogTitle()
    {
        SetWindowText(m_hDialog, Translate(L"uninstallerWindowTitle").c_str());
    }

    void AppUninstaller::PopulateLanguageMenu()
    {
        std::vector<Popup::PopupItem> items;
        for (const auto &locale : Tools::Localization::EnumerateResourceLocales())
        {
            items.emplace_back(
                Unicode::to_upper(locale.code) == Unicode::to_upper(Tools::Localization::GetCurrentLocale())
                    ? L"\xE1D2"
                    : L"\xEA3F",
                locale.language,
                [this, code = locale.code]() { OnSelectLanguage(code); }
            );
        }

        if (!items.empty())
        {
            m_languageButton.SetMenu(items, m_theme.DpiScale(120), TPM_LEFTALIGN);
            m_languageButton.OnChanged = [this]() { m_languageButton.ShowMenu(); };
        }
        m_languageButton.SetText(Unicode::to_upper(Tools::Localization::GetCurrentLocale()).substr(0, 2));
    }

    void AppUninstaller::OnSelectLanguage(const std::wstring &localeCode)
    {
        Tools::Localization::InitializeFromLocales(localeCode);

        PopulateLanguageMenu();
        UpdateDialogTitle();
        ShowWelcomePage();
    }

    void AppUninstaller::Uninstall(bool update)
    {
        Ally::Services::DisableInjectorService();

        TerminateAnyFSE();

        bool removeDir = DeleteFiles(Tools::Paths::GetInstallPath());

        if (!update)
        {
            Tools::Packages::RemovePackage(AppConstants::PackageFamilyName);
        }
        ToolsEx::Certificate::RemoveRootCertificate(Unicode::to_wstring(VER_PUBLISHER_CN));

        Registry::DeleteKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AnyFSE");

        AutoDeleteSelf(Tools::Paths::GetInstallPath(), removeDir);
    }

    void AppUninstaller::OnUninstall()
    {
        try
        {
            Uninstall();
        }
        catch(const std::exception& e)
        {
            ShowErrorPage(Translate(L"uninstallationErrorCaption"), Unicode::to_wstring(e.what()));
            return;
        }
        ShowCompletePage();
    }

    bool AppUninstaller::AutoDeleteSelf(const std::wstring& path, bool deleteFolder)
    {
        std::wstring batchPath = fs::temp_directory_path().wstring() + L"\\unins000_anyfse_cleanup.bat";

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
        std::set<DWORD> ids;
        Process::FindAllByName(L"AnyFSE.exe", ids);
        for (auto id : ids)
        {
            Process::Kill(id);
        }
        return true;
    }

    void AppUninstaller::OnDone()
    {
        EndDialog(m_hDialog, IDOK);
    }
}
