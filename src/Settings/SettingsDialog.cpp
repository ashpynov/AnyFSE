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
#include "SettingsComboBox.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace AnyFSE::Settings
{
    Logger log = LogManager::GetLogger("Settings");

    UINT SettingsDialog::DPI = 96;

    static int DpiScale(int value)
    {
        return MulDiv(value, SettingsDialog::DPI, 96);
    }

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
            DPI = GetDpiForWindow(hwnd);
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
            delete pLauncherCombo;
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

    void SettingsDialog::OnInitDialog(HWND hwnd)
    {

        RECT rect;
        GetClientRect(hwnd, &rect);

        ULONG top = DpiScale(45);
        pLauncherCombo = new Controls::SettingsComboBox(
            m_theme,
            hwnd,
            L"Choose home app",
            L"Choose home application for full screen expirience",
            DpiScale(32), top, rect.right - rect.left - DpiScale(64), DpiScale(67));

        pLauncherCombo->OnChanged += [This = this]()
        {
            This->OnLauncherChanged(This->m_hDialog);
        };

        top += DpiScale(67 + 8);

        pEnterFullscreen = new Controls::SettingsComboBox(
            m_theme,
            hwnd,
            L"Enter full screen expitience on startup",
            L"",
            DpiScale(32), top, rect.right - rect.left - DpiScale(64), DpiScale(67));

        InitGroups();
        CheckDlgButton(m_hDialog, IDC_RUN_ON_STARTUP, true ? BST_CHECKED : BST_UNCHECKED);
        current = Config::GetCurrentLauncher();
        Config::FindLaunchers(launchers);
        UpdateCombo();
        Config::GetLauncherSettings(current, config);
        m_isCustom = config.isCustom && config.Type != LauncherType::Xbox || config.Type == LauncherType::Custom;
        m_isAgressive = Config::AggressiveMode && config.Type != LauncherType::Xbox;
        UpdateCustom();
        UpdateCustomSettings();

        // ShowGroup(0, config.isCustom);
    }

    void SettingsDialog::InitGroups()
    {
        groups = std::vector<ControlsGroup>{
            {
                true, 0, {
                    { IDC_GROUP_SIZER, 0 },
                    { IDC_GROUP_FRAME, 0 },
                    { IDC_ARGUMENTS_STATIC ,0 },
                    { IDC_ARGUMENTS ,0 },
                    { IDC_PROCESS_NAME_STATIC, 0 },
                    { IDC_PROCESS_NAME, 0 },
                    { IDC_TITLE_STATIC, 0 },
                    { IDC_TITLE, 0 },
                    { IDC_PROCESS_NAME_ALT_STATIC, 0 },
                    { IDC_PROCESS_NAME_ALT, 0 },
                    { IDC_TITLE_ALT_STATIC, 0 },
                    { IDC_TITLE_ALT, 0 }
                }
            },
            {
                true, 0, {
                    {IDC_NOT_AGGRESSIVE_MODE, 0},
                    {IDC_RUN_ON_STARTUP, 0},
                    {IDOK, 0},
                    {IDCANCEL, 0}
                }
            }
        };

        for (auto& group : groups )
        {
            group.visible = true;
            for (auto& g : group.items)
            {
                RECT rect;
                GetWindowRect(GetDlgItem(m_hDialog, g.first), &rect);
                MapWindowPoints(HWND_DESKTOP, m_hDialog, (LPPOINT)&rect, 2);
                g.second = rect.top;

                if (g.first == group.items[0].first)
                {
                    group.height = rect.bottom - rect.top;
                }
            }
        }
        RECT rect;
        GetWindowRect(m_hDialog, &rect);
        designHeight = rect.bottom - rect.top;
    }

    void SettingsDialog::MoveGroupItem(int id, int original, int offset )
    {
        RECT rect;
        GetWindowRect(GetDlgItem(m_hDialog, id), &rect);
        MapWindowPoints(HWND_DESKTOP, m_hDialog, (LPPOINT)&rect, 2);
        int h = rect.bottom-rect.top;

        MoveWindow(GetDlgItem(m_hDialog, id), rect.left, original - offset, rect.right - rect.left, h, FALSE);
    }

    void SettingsDialog::ShowGroup(int groupIdx, bool show)
    {
        int offset = 0;
        for (int i = 0; i < groupIdx; ++i)
        {
            if (!groups[i].visible)
            {
                offset += groups[i].height;
            }
        }
        ControlsGroup &group = groups[groupIdx];
        for (auto& g : group.items)
        {
            ShowWindow(GetDlgItem(m_hDialog, g.first), show ? SW_SHOW : SW_HIDE);
            if (show)
            {
                MoveGroupItem(g.first, g.second, offset);
            }
        }
        group.visible = show;
        if (!show)
        {
            offset += group.height;
        }
        for (int i = groupIdx + 1; i < groups.size(); ++i)
        {
            if(groups[i].visible)
            {
                for (auto& g: groups[i].items)
                {
                    MoveGroupItem(g.first, g.second, offset);
                }
            }
            else
            {
                offset += groups[i].height;
            }
        }
        RECT rect;
        GetWindowRect(m_hDialog, &rect);
        int h = rect.bottom-rect.top;

        SetWindowPos(m_hDialog, NULL, rect.left, rect.top, rect.right - rect.left, designHeight - offset, SWP_NOACTIVATE | SWP_NOZORDER);
        //MoveWindow(m_hDialog, rect.left, rect.top, rect.right - rect.left, rect.top + designHeight - offset, TRUE);
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
    void SettingsDialog::OnCustomChanged(HWND hwnd)
    {
        m_isCustom = BST_CHECKED == SendDlgItemMessage(m_hDialog, IDC_CUSTOM, BM_GETCHECK, 0, 0);
        UpdateCustom();
    }

    void SettingsDialog::OnAggressiveChanged(HWND hwnd)
    {
        m_isAgressive = BST_CHECKED != SendDlgItemMessage(m_hDialog, IDC_NOT_AGGRESSIVE_MODE, BM_GETCHECK, 0, 0);
        UpdateCustom();
    }

    void SettingsDialog::UpdateCustom()
    {
        LauncherConfig defaults;
        Config::GetLauncherDefaults(current, defaults);
        bool isCustom = defaults.Type==LauncherType::Custom;
        bool isXbox = defaults.Type == LauncherType::Xbox;

        bool setCheck = m_isCustom && !isXbox || isCustom;
        bool enableCheck = !isCustom && !isXbox;

        SendDlgItemMessage(m_hDialog, IDC_CUSTOM, BM_SETCHECK, (WPARAM)setCheck ? BST_CHECKED : BST_UNCHECKED, 0 );
        EnableWindow(GetDlgItem(m_hDialog, IDC_CUSTOM), enableCheck);

        ShowGroup(0, setCheck);
        if (!setCheck)
        {
            config = defaults;
            UpdateCustomSettings();
        }
    }

    void SettingsDialog::OnLauncherChanged(HWND hwnd)
    {
        current = pLauncherCombo->GetCurentValue();
        Config::GetLauncherSettings(current, config);
        UpdateCustom();
        UpdateCustomSettings();
    }

    void SettingsDialog::AddComboItem(const wstring& name, const wstring& path, int pos)
    {
        pLauncherCombo->AddItem(name, path, path, pos);
    }

    void SettingsDialog::UpdateCombo()
    {
        pLauncherCombo->Reset();

        for ( auto& launcher: launchers)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(launcher, info);
            AddComboItem(info.Name, info.StartCommand);
        }
        size_t index = Tools::index_of(launchers, current);

        if (index == Tools::npos)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(current, info);
            AddComboItem(info.Name, info.StartCommand, 0);
            index = 0;
        }

        pLauncherCombo->SelectItem((int)index);
    }

    void SettingsDialog::UpdateCustomSettings()
    {
        // Set values
        SetDlgItemText(m_hDialog, IDC_ARGUMENTS,        config.StartArg.c_str());
        SetDlgItemText(m_hDialog, IDC_PROCESS_NAME,     config.ProcessName.c_str());
        SetDlgItemText(m_hDialog, IDC_PROCESS_NAME_ALT, config.ProcessNameAlt.c_str());
        SetDlgItemText(m_hDialog, IDC_TITLE,            config.WindowTitle.c_str());
        SetDlgItemText(m_hDialog, IDC_TITLE_ALT,        config.WindowTitleAlt.c_str());

        SendDlgItemMessage(m_hDialog, IDC_NOT_AGGRESSIVE_MODE, BM_SETCHECK, (WPARAM)(m_isAgressive && config.Type != LauncherType::Xbox) ? BST_UNCHECKED : BST_CHECKED, 0 );
        EnableWindow(GetDlgItem(m_hDialog, IDC_NOT_AGGRESSIVE_MODE), config.Type != LauncherType::Xbox);
    }
    void SettingsDialog::SaveSettings()
    {}
}