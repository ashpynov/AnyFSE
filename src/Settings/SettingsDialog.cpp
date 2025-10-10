// SettingsDialog.cpp
#include "SettingsDialog.hpp"
#include "Resource.h"
#include <commdlg.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include "Tools/Tools.hpp"
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
        RECT rcDialog;
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
        SettingsDialog *This = nullptr;

        if (msg == WM_INITDIALOG)
        {
            // Store 'this' pointer in window user data
            This = (SettingsDialog *)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)This);
            This->m_hDialog = hwnd;
        }
        else
        {
            // Get 'this' pointer from window user data
            This = (SettingsDialog *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (This)
        {
            return This->InstanceDialogProc(hwnd, msg, wParam, lParam);
        }

        return FALSE;
    }

    // Instance dialog procedure - has access to all member variables
    INT_PTR SettingsDialog::InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
            SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            m_theme.Attach(hwnd);
            OnInitDialog(hwnd);
            CenterDialog(hwnd);
            return TRUE;

        case WM_CTLCOLORDLG:
        {
            // Return a black brush for dialog background
            static HBRUSH hBlackBrush = CreateSolidBrush(RGB(32, 32, 32));
            return (LRESULT)hBlackBrush;
        }
        case WM_DESTROY:

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDC_CUSTOM:
                if (HIWORD(wParam) == BN_CLICKED)
                {
                    OnCustomChanged(hwnd);
                }
                break;
            case IDC_NOT_AGGRESSIVE_MODE:
                if (HIWORD(wParam) == BN_CLICKED)
                {
                    OnAggressiveChanged(hwnd);
                }
                break;
            case IDC_BROWSE:
                OnBrowseLauncher(hwnd, 0);
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

    template<class T>
    void SettingsDialog::AddSettingsLine(
        ULONG &top, const wstring& name, const wstring& desc, T& control,
        int height, int padding, int contentMargin, int contentWidth, int contentHeight )
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);
        int width = rect.right - rect.left - m_theme.DpiScale(64);

        settingLines.emplace_back(
            m_theme, m_hDialog, name, desc, m_theme.DpiScale(32), top, width, m_theme.DpiScale(height),
            [&](HWND parent) { return control.Create(parent, 0, 0,
                m_theme.DpiScale(contentWidth), m_theme.DpiScale(contentHeight)); });

        top += m_theme.DpiScale(height + padding);
        if (contentMargin)
        {
            settingLines.back().SetLeftMargin(m_theme.DpiScale(contentMargin));
        }
    }

    void SettingsDialog::OnInitDialog(HWND hwnd)
    {
        ULONG top = m_theme.DpiScale(45);

        AddSettingsLine(top,
            L"Choose home app",
            L"Choose home application for full screen expirience",
            launcherCombo,
            67, -12, 0, 280 );

        AddSettingsLine(top,
            L"",
            L"",
            browse,
            57, 8, 0, 130, 32);

        AddSettingsLine(top,
            L"Enter full screen expirience on startup",
            L"",
            enterFullscreen,
            67, 8, 0);

        AddSettingsLine(top,
            L"Use custom settings",
            L"Change monitoring and startups settings for selected home application",
            customSettings,
            67, 2, 0);

        AddSettingsLine(top,
            L"Additional arguments",
            L"Command line arguments passed to application",
            additionalArguments,
            48, 2, 20);

        AddSettingsLine(top,
            L"Primary process name",
            L"Name of home application process",
            processName,
            48, 2, 20);

        AddSettingsLine(top,
            L"Primary window title",
            L"Title of app window when it have been activated",
            title,
            48, 2, 20);

        AddSettingsLine(top,
            L"Secondary process name",
            L"Name of app process for alternative mode",
            processNameAlt,
            48, 2, 20);

        AddSettingsLine(top,
            L"Secondary window title",
            L"Title of app window when it alternative mode have been activated",
            titleAlt,
            48, 2, 20);

        launcherCombo.OnChanged += [This = this]()
        {
            This->OnLauncherChanged(This->m_hDialog);
        };

        customSettings.OnChanged += [This = this]()
        {
            This->OnCustomChanged(This->m_hDialog);
        };

        browse.SetText(L"Browse");
        browse.OnChanged += [This = this]()
        {
            This->OnBrowseLauncher(This->m_hDialog, 0);
        };

        CheckDlgButton(m_hDialog, IDC_RUN_ON_STARTUP, true ? BST_CHECKED : BST_UNCHECKED);
        current = Config::GetCurrentLauncher();
        Config::FindLaunchers(launchers);
        UpdateCombo();
        Config::GetLauncherSettings(current, config);
        m_isCustom = config.isCustom && config.Type != LauncherType::Xbox || config.Type == LauncherType::Custom;
        m_isAgressive = Config::AggressiveMode && config.Type != LauncherType::Xbox;
        UpdateCustom();
        UpdateCustomSettings();
    }

    void SettingsDialog::ShowGroup(int groupIdx, bool show)
    {
        // To do
    }

    void SettingsDialog::OnBrowseLauncher(HWND hwnd, int editId)
    {
        OPENFILENAME ofn = {};
        WCHAR szFile[260] = {};

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = L"Launchers (*.exe)\0*.exe\0\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn))
        {
            if (current != szFile)
            {
                current = szFile;
                Config::GetLauncherSettings(current, config);
                UpdateCombo();
                UpdateCustom();
                UpdateCustomSettings();
            }
        }
    }

    void SettingsDialog::OnOk(HWND hwnd)
    {
        // Save values from controls
    }
    void SettingsDialog::OnCustomChanged(HWND hwnd)
    {
        m_isCustom = customSettings.GetCheck();
        UpdateCustom();
    }

    void SettingsDialog::OnAggressiveChanged(HWND hwnd)
    {
        // m_isAgressive = BST_CHECKED != SendDlgItemMessage(m_hDialog, IDC_NOT_AGGRESSIVE_MODE, BM_GETCHECK, 0, 0);
        // UpdateCustom();
    }

    void SettingsDialog::UpdateCustom()
    {
        LauncherConfig defaults;
        Config::GetLauncherDefaults(current, defaults);
        bool isCustom = defaults.Type==LauncherType::Custom;
        bool isXbox = defaults.Type == LauncherType::Xbox;

        bool setCheck = m_isCustom && !isXbox || isCustom;
        bool enableCheck = !isCustom && !isXbox;

        customSettings.SetCheck(setCheck);
        // customSettings.Enable(enableCheck); // TODO
        // ShowGroup(0, setCheck);
        if (!setCheck)
        {
            config = defaults;
            UpdateCustomSettings();
        }
    }

    void SettingsDialog::OnLauncherChanged(HWND hwnd)
    {
        current = launcherCombo.GetCurentValue();;
        Config::GetLauncherSettings(current, config);
        UpdateCustom();
        UpdateCustomSettings();
    }


    void SettingsDialog::UpdateCombo()
    {
        launcherCombo.Reset();

        launcherCombo.AddItem(L"None", L"", L"");

        for ( auto& launcher: launchers)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(launcher, info);
            launcherCombo.AddItem(info.Name, info.IconFile, info.StartCommand);
        }
        size_t index = Tools::index_of(launchers, current);

        if (index == Tools::npos)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(current, info);
            launcherCombo.AddItem(info.Name, info.IconFile, info.StartCommand, 1);
            index = 1;
        }

        launcherCombo.SelectItem((int)index);
    }

    void SettingsDialog::UpdateCustomSettings()
    {
        // Set values
        additionalArguments.SetText(config.StartArg);
        processName.SetText(config.ProcessName);
        processNameAlt.SetText(config.ProcessNameAlt);
        title.SetText(config.WindowTitle);
        titleAlt.SetText(config.WindowTitleAlt);

        // SendDlgItemMessage(m_hDialog, IDC_NOT_AGGRESSIVE_MODE, BM_SETCHECK, (WPARAM)(m_isAgressive && config.Type != LauncherType::Xbox) ? BST_UNCHECKED : BST_CHECKED, 0 );
        // EnableWindow(GetDlgItem(m_hDialog, IDC_NOT_AGGRESSIVE_MODE), config.Type != LauncherType::Xbox);
    }
    void SettingsDialog::SaveSettings()
    {}
}