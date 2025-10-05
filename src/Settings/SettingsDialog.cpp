// SettingsDialog.cpp
#include "SettingsDialog.hpp"
#include "Resource.h"
#include <commdlg.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include "Logging/LogManager.hpp"
#include "SettingsDialog.hpp"
#include "Configuration/Config.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace AnyFSE::Settings
{
    Logger log = LogManager::GetLogger("Settings");

    INT_PTR SettingsDialog::Show(HINSTANCE hInstance)
    {
        INT_PTR res = DialogBoxParam(hInstance,
                                     MAKEINTRESOURCE(IDD_SETTINGS),
                                     NULL,
                                     DialogProc,
                                     (LPARAM)this);
        if (res == -1)
        {
            log.Error(log.APIError(),"Cant create settings dialog)");
        }
        return res;
    }

    void SettingsDialog::CenterDialog(HWND hwnd)
    {
        RECT rcDialog, rcScreen;
        GetWindowRect(hwnd, &rcDialog);

        // Get screen dimensions
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        // Calculate center position
        int x = (screenWidth - (rcDialog.right - rcDialog.left)) / 2;
        int y = (screenHeight - (rcDialog.bottom - rcDialog.top)) / 2;

        // Move dialog to center
        SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }

    // Static dialog procedure - routes to instance method
    INT_PTR CALLBACK SettingsDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        SettingsDialog *pThis = nullptr;

        if (msg == WM_INITDIALOG)
        {
            // Store 'this' pointer in window user data
            pThis = (SettingsDialog *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hDialog = hwnd;
        }
        else
        {
            // Get 'this' pointer from window user data
            pThis = (SettingsDialog *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (pThis)
        {
            return pThis->InstanceDialogProc(hwnd, msg, wParam, lParam);
        }

        return FALSE;
    }

    // Instance dialog procedure - has access to all member variables
    INT_PTR SettingsDialog::InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            OnInitDialog(hwnd);
            CenterDialog(hwnd);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDC_BROWSE:
                OnBrowseLauncher(hwnd, IDC_LAUNCHER);
                break;
            case IDOK:
                OnOk(hwnd);
                EndDialog(hwnd, IDOK);
                return TRUE;
            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
            break;
        }

        return FALSE;
    }

    void SettingsDialog::OnInitDialog(HWND hwnd)
    {
        // Set initial values
        SetDlgItemText(hwnd, IDC_LAUNCHER, Config::LauncherStartCommand.c_str());
        SetDlgItemText(hwnd, IDC_ARGUMENTS, Config::LauncherStartCommandArgs.c_str());
        SetDlgItemText(hwnd, IDC_PROCESS_NAME, Config::LauncherProcessName.c_str());
        SetDlgItemText(hwnd, IDC_PROCESS_NAME_ALT, L"");

        SetDlgItemText(hwnd, IDC_TITLE, Config::LauncherWindowName.c_str());
        SetDlgItemText(hwnd, IDC_TITLE_ALT, L"");

        CheckDlgButton(hwnd, IDC_AGGRESSIVE_MODE, Config::AggressiveMode ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_RUN_ON_STARTUP, true ? BST_CHECKED : BST_UNCHECKED);
    }

    void SettingsDialog::OnBrowseLauncher(HWND hwnd, int editId)
    {
        OPENFILENAME ofn = {};
        WCHAR szFile[260] = {};

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L"All Files\0*.*\0\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn))
        {
            SetDlgItemText(hwnd, editId, szFile);
        }
    }

    void SettingsDialog::OnOk(HWND hwnd)
    {
        // Save values from controls
        // WCHAR buffer[MAX_PATH];

        // GetDlgItemText(hwnd, IDC_EDIT1, buffer, MAX_PATH);
        // m_settings.filePath1 = buffer;

        // GetDlgItemText(hwnd, IDC_EDIT2, buffer, MAX_PATH);
        // m_settings.filePath2 = buffer;

        // GetDlgItemText(hwnd, IDC_EDIT3, buffer, MAX_PATH);
        // m_settings.filePath3 = buffer;

        // m_settings.checkbox1 = IsDlgButtonChecked(hwnd, IDC_CHECKBOX1) == BST_CHECKED;
        // m_settings.checkbox2 = IsDlgButtonChecked(hwnd, IDC_CHECKBOX2) == BST_CHECKED;
    }
}