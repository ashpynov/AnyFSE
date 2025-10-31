// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include "SettingsDialog.hpp"
#include "Resource.h"
#include <commdlg.h>
#include <uxtheme.h>
#include "Tools/Tools.hpp"
#include "Tools/Unicode.hpp"
#include "Logging/LogManager.hpp"
#include "SettingsDialog.hpp"
#include "Configuration/Config.hpp"
#include "FluentDesign/Theme.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "TaskManager.hpp"
#include "Registry.hpp"

#define byte ::byte

#include "Tools/Gdiplus.hpp"


#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace AnyFSE::App::AppSettings::Settings
{
    Logger log = LogManager::GetLogger("Settings");

    INT_PTR SettingsDialog::Show(HINSTANCE hInstance)
    {
        size_t size = sizeof(DLGTEMPLATE) + sizeof(WORD) * 3; // menu, class, title
        HGLOBAL hGlobal = GlobalAlloc(GHND, size);

        LPDLGTEMPLATE dlgTemplate = (LPDLGTEMPLATE)GlobalLock(hGlobal);

        dlgTemplate->style = DS_MODALFRAME | WS_POPUP | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU;
        dlgTemplate->dwExtendedStyle = WS_EX_TOPMOST | WS_EX_WINDOWEDGE;
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


    void SettingsDialog::CenterDialog(HWND hwnd)
    {
        // Get screen dimensions
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        int cx = m_theme.DpiScale(700);
        int cy = m_theme.DpiScale(720);

        // Calculate center position
        int x = (screenWidth - cx) / 2;
        int y = (screenHeight - cy) / 2;

        // Move dialog to center
        SetWindowPos(hwnd, NULL, x, y, cx, cy, SWP_NOZORDER);
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
            {   // To enable immersive dark mode (for a dark title bar)
                m_theme.Attach(hwnd);
                CenterDialog(hwnd);
                OnInitDialog(hwnd);
            }
            return TRUE;

        case WM_PAINT:
            {
                FluentDesign::DoubleBuferedPaint paint(hwnd);
                HBRUSH back = CreateSolidBrush(m_theme.GetColorRef(FluentDesign::Theme::Colors::Dialog));
                RECT r = paint.ClientRect();
                FillRect(paint.MemDC(), &r, back);
                DeleteObject(back);

                r.left += m_theme.DpiScale(Layout_MarginLeft);

                WCHAR *text1 = L"Gaming > ";
                WCHAR *text2 = L"Any Full Screen Experience";
                SetTextColor(paint.MemDC(), m_theme.GetColorRef(FluentDesign::Theme::Colors::TextSecondary));
                SetBkMode(paint.MemDC(), TRANSPARENT);
                HGDIOBJ oldFont = SelectFont(paint.MemDC(), m_theme.GetFont_Title());
                ::DrawText(paint.MemDC(), text1, -1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT);
                ::DrawText(paint.MemDC(), text1, -1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);

                r.left = r.right;
                r.right = paint.ClientRect().right;
                SetTextColor(paint.MemDC(), m_theme.GetColorRef(FluentDesign::Theme::Colors::Text));
                ::DrawText(paint.MemDC(), text2, -1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);

                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_hButtonOk);
                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_hButtonClose);
                m_theme.DrawChildFocus(paint.MemDC(), m_hDialog, m_hButtonRemove);

                SelectObject(paint.MemDC(), oldFont);
            }
            return FALSE;


        case WM_DESTROY:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK:
                OnOk();
                EndDialog(hwnd, IDOK);
                return TRUE;
            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            case IDABORT:
                EndDialog(hwnd, IDABORT);
                return TRUE;
            }
            break;
        }

        return FALSE;
    }

    template<class T>
    FluentDesign::SettingsLine& SettingsDialog::AddSettingsLine(
        ULONG &top, const std::wstring& name, const std::wstring& desc, T& control,
        int height, int padding, int contentMargin, int contentWidth, int contentHeight )
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);
        int width = rect.right - rect.left
            - m_theme.DpiScale(Layout_MarginLeft)
            - m_theme.DpiScale(Layout_MarginRight);

        m_settingLinesList.emplace_back(
            m_theme, m_hScrollView, name, desc,
            m_theme.DpiScale(Layout_MarginLeft),
            top, width,
            m_theme.DpiScale(height),
            [&](HWND parent) { return control.Create(parent, 0, 0,
                m_theme.DpiScale(contentWidth), m_theme.DpiScale(contentHeight)); });

        top += m_theme.DpiScale(height + padding);
        if (contentMargin)
        {
            m_settingLinesList.back().SetLeftMargin(contentMargin);
        }

        return m_settingLinesList.back();
    }

    void SettingsDialog::OnInitDialog(HWND hwnd)
    {


        RECT rect;
        GetClientRect(m_hDialog, &rect);

        m_hScrollView = m_scrollView.Create(m_hDialog,
            0,
            rect.top + m_theme.DpiScale(Layout_MarginTop),
            rect.right,
            rect.bottom
                - m_theme.DpiScale(Layout_MarginBottom)
                - m_theme.DpiScale(Layout_MarginTop)
                - m_theme.DpiScale(Layout_ButtonHeight)
                - m_theme.DpiScale(Layout_ButtonPadding)
        );

        ULONG top = 0;

        FluentDesign::SettingsLine &launcher = AddSettingsLine(top,
            L"Choose home app",
            L"Choose home application for full screen experience",
            m_launcherCombo,
            Layout_LineHeight, Layout_LauncherBrowsePadding, 0,
            Layout_LauncherComboWidth );

        launcher.SetFrame(Gdiplus::FrameFlags::SIDE_NO_BOTTOM | Gdiplus::FrameFlags::CORNER_TOP);

        FluentDesign::SettingsLine &browse = AddSettingsLine(top,
            L"",
            L"",
            m_browseButton,
            Layout_LauncherBrowseLineHeight, Layout_LinePadding, 0,
            Layout_BrowseWidth, Layout_BrowseHeight);

        browse.SetFrame(Gdiplus::FrameFlags::SIDE_NO_TOP | Gdiplus::FrameFlags::CORNER_BOTTOM);

        m_pFseOnStartupLine = &AddSettingsLine(top,
            L"Enter full screen experience on startup",
            L"",
            m_fseOnStartupToggle,
            Layout_LineHeight, Layout_LinePadding, 0);

        m_pCustomSettingsLine = &AddSettingsLine(top,
            L"Use custom settings",
            L"Change monitoring and startups settings for selected home application",
            m_customSettingsToggle,
            Layout_LineHeight, Layout_LinePaddingSmall, 0);

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Additional arguments",
            L"Command line arguments passed to application",
            m_additionalArgumentsEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Primary process name",
            L"Name of home application process",
            m_processNameEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Primary window title",
            L"Title of app window when it have been activated",
            m_titleEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Secondary process name",
            L"Name of app process for alternative mode",
            m_processNameAltEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        m_pCustomSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Secondary window title",
            L"Title of app window when it alternative mode have been activated",
            m_titleAltEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));



        GetClientRect(m_hDialog, &rect);
        rect.top = rect.bottom
            - m_theme.DpiScale(Layout_MarginBottom)
            - m_theme.DpiScale(Layout_ButtonHeight);

        rect.right -= m_theme.DpiScale(Layout_MarginRight);
        rect.left  += m_theme.DpiScale(Layout_MarginLeft);


        m_hButtonRemove = m_removeButton.Create(m_hDialog,
            rect.left,
            rect.top,
            m_theme.DpiScale(Layout_UninstallWidth),
            m_theme.DpiScale(Layout_ButtonHeight));

        m_removeButton.SetText(L"Uninstall");

        m_removeButton.OnChanged += [This = this]()
        {
            This->OnUninstall();
        };


        m_hButtonOk = m_okButton.Create(m_hDialog,
            rect.right
                - m_theme.DpiScale(Layout_OKWidth)
                - m_theme.DpiScale(Layout_ButtonPadding)
                - m_theme.DpiScale(Layout_CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout_OKWidth),
            m_theme.DpiScale(Layout_ButtonHeight));

        m_okButton.SetText(L"Save");

        m_okButton.OnChanged += [This = this]()
        {
            This->OnOk();
        };

        m_hButtonClose = m_closeButton.Create(m_hDialog,
            rect.right - m_theme.DpiScale(Layout_CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout_CloseWidth),
            m_theme.DpiScale(Layout_ButtonHeight));

        m_closeButton.OnChanged += [This = this]()
        {
            EndDialog(This->m_hDialog, IDCANCEL);
        };

        m_closeButton.SetText(L"Discard and Close");

        m_launcherCombo.OnChanged += [This = this]()
        {
            This->OnLauncherChanged(This->m_hDialog);
        };

        m_customSettingsToggle.OnChanged += [This = this]()
        {
            This->OnCustomChanged(This->m_hDialog);
        };

        m_pCustomSettingsLine->OnChanged += [This = this]()
        {
            This->m_customSettingsState = This->m_pCustomSettingsLine->GetState();
        };

        m_browseButton.SetText(L"Browse");
        m_browseButton.OnChanged += [This = this]()
        {
            This->OnBrowseLauncher(This->m_hDialog, 0);
        };

        m_currentLauncherPath = Config::Launcher.StartCommand;
        Config::FindLaunchers(m_launchersList);
        UpdateCombo();
        Config::LoadLauncherSettings(m_currentLauncherPath, m_config);

        bool customSettings = Config::CustomSettings;
        m_customSettingsState = customSettings ? FluentDesign::SettingsLine::Opened: FluentDesign::SettingsLine::Closed;

        m_isCustom = (customSettings || m_config.IsCustom) && m_config.Type != LauncherType::Xbox || m_config.Type == LauncherType::Custom;
        m_isAgressive = Config::AggressiveMode && m_config.Type != LauncherType::Xbox;

        UpdateControls();
        UpdateCustomSettings();
        m_customSettingsState = m_pCustomSettingsLine->GetState() != FluentDesign::SettingsLine::Normal
                                ? m_pCustomSettingsLine->GetState()
                                : FluentDesign::SettingsLine::Opened;
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
            if (m_currentLauncherPath != szFile)
            {
                m_currentLauncherPath = szFile;
                Config::LoadLauncherSettings(m_currentLauncherPath, m_config);
                UpdateCombo();
                UpdateControls();
                UpdateCustomSettings();
            }
        }
    }

    void SettingsDialog::OnOk()
    {
        SaveSettings();
        EndDialog(m_hDialog, IDOK);
    }

    void SettingsDialog::InitCustomControls()
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_STANDARD_CLASSES;
        ::InitCommonControlsEx(&icex);
    }

    void SettingsDialog::OnUninstall()
    {
        // uninstall
        InitCustomControls();
        int nButtonClicked = 0;
        if (SUCCEEDED(TaskDialog(m_hDialog, GetModuleHandle(NULL),
                       L"Uninstall",
                       L"Uninstall Any Fullscreen Experience?",
                       L"This will remove system task and all settings\n",
                       TDCBF_YES_BUTTON | TDCBF_CANCEL_BUTTON, TD_INFORMATION_ICON, &nButtonClicked))
            && nButtonClicked == IDYES)

        {
            //Registry::DeleteKey(L"HKCU\\Software\\AnyFSE");
            TaskManager::RemoveTask();
            EndDialog(m_hDialog, IDABORT);
        }
    }

    void SettingsDialog::OnCustomChanged(HWND hwnd)
    {
        m_isCustom = m_customSettingsToggle.GetCheck();
        UpdateControls();
    }

    void SettingsDialog::OnAggressiveChanged(HWND hwnd)
    {
        // m_isAgressive = BST_CHECKED != SendDlgItemMessage(m_hDialog, IDC_NOT_AGGRESSIVE_MODE, BM_GETCHECK, 0, 0);
        // UpdateCustom();
    }

    void SettingsDialog::UpdateControls()
    {
        LauncherConfig defaults;
        Config::GetLauncherDefaults(m_currentLauncherPath, defaults);

        if (defaults.Type == LauncherType::None)
        {
            m_pFseOnStartupLine->Enable(false);
            m_fseOnStartupToggle.SetCheck(false);
        }
        else
        {
            m_pFseOnStartupLine->Enable();
            m_fseOnStartupToggle.SetCheck(Config::FseOnStartup);
        }

        bool alwaysSettings = defaults.Type==LauncherType::Custom;
        bool noSettings = defaults.Type == LauncherType::Xbox || defaults.Type == LauncherType::None;

        bool haveSettings = m_isCustom && !noSettings || alwaysSettings;
        bool enableCheck = !alwaysSettings && !noSettings;

        m_customSettingsToggle.SetCheck(haveSettings);

        FluentDesign::SettingsLine::State state = haveSettings
                ? alwaysSettings
                    ? FluentDesign::SettingsLine::Opened
                    : m_customSettingsState
                : FluentDesign::SettingsLine::Normal;

        m_pCustomSettingsLine->SetState(state);
        m_pCustomSettingsLine->Enable(enableCheck);

        if (!haveSettings)
        {
            m_config = defaults;
            UpdateCustomSettings();
        }
    }

    void SettingsDialog::OnLauncherChanged(HWND hwnd)
    {
        m_currentLauncherPath = m_launcherCombo.GetCurentValue();;
        Config::LoadLauncherSettings(m_currentLauncherPath, m_config);
        UpdateControls();
        UpdateCustomSettings();
    }


    void SettingsDialog::UpdateCombo()
    {
        m_launcherCombo.Reset();

        for ( auto& launcher: m_launchersList)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(launcher, info);
            Config::UpdatePortableLauncher(info);
            m_launcherCombo.AddItem(info.Name, info.IconFile, info.StartCommand);
        }
        size_t index = Tools::index_of(m_launchersList, m_currentLauncherPath);

        if (index == Tools::npos)
        {
            LauncherConfig info;
            Config::GetLauncherDefaults(m_currentLauncherPath, info);
            Config::UpdatePortableLauncher(info);
            m_launcherCombo.AddItem(info.Name, info.IconFile, info.StartCommand, 1);
            index = 1;
        }

        m_launcherCombo.SelectItem((int)index);
    }

    void SettingsDialog::UpdateCustomSettings()
    {
        // Set values
        m_additionalArgumentsEdit.SetText(m_config.StartArg);
        m_processNameEdit.SetText(m_config.ProcessName);
        m_processNameAltEdit.SetText(m_config.ProcessNameAlt);
        m_titleEdit.SetText(m_config.WindowTitle);
        m_titleAltEdit.SetText(m_config.WindowTitleAlt);

        // SendDlgItemMessage(m_hDialog, IDC_NOT_AGGRESSIVE_MODE, BM_SETCHECK, (WPARAM)(m_isAgressive && config.Type != LauncherType::Xbox) ? BST_UNCHECKED : BST_CHECKED, 0 );
        // EnableWindow(GetDlgItem(m_hDialog, IDC_NOT_AGGRESSIVE_MODE), config.Type != LauncherType::Xbox);
    }
    void SettingsDialog::SaveSettings()
    {
        const std::wstring gamingConfiguration = L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration";
        const std::wstring startupToGamingHome = L"StartupToGamingHome";
        const std::wstring gamingHomeApp = L"GamingHomeApp";
        const std::wstring xboxApp = L"Microsoft.GamingApp_8wekyb3d8bbwe!Microsoft.Xbox.App";


        bool removeAnyFSE = false;

        if (m_config.Type == LauncherType::None)
        {
            Registry::DeleteValue(gamingConfiguration, gamingHomeApp);
            Registry::WriteBool(gamingConfiguration, startupToGamingHome, false);
            removeAnyFSE = true;
        }
        else
        {
            Registry::WriteBool(gamingConfiguration, startupToGamingHome, m_fseOnStartupToggle.GetCheck());
            Registry::WriteString(gamingConfiguration, gamingHomeApp, xboxApp);

            if (m_config.Type == LauncherType::Xbox)
            {
                removeAnyFSE = true;
            }
            else
            {
                // save FSE
            }
        }

        Config::Launcher.StartCommand = m_config.StartCommand;
        Config::CustomSettings = m_customSettingsToggle.GetCheck();
        Config::CustomSettings = m_customSettingsToggle.GetCheck();
        Config::Launcher.StartArg = m_additionalArgumentsEdit.GetText();
        Config::Launcher.ProcessName = m_processNameEdit.GetText();
        Config::Launcher.WindowTitle = m_titleEdit.GetText();
        Config::Launcher.ProcessNameAlt = m_processNameAltEdit.GetText();
        Config::Launcher.WindowTitleAlt = m_titleAltEdit.GetText();
        Config::Launcher.IconFile = m_config.IconFile;

        Config::Save();

        if (removeAnyFSE)
        {
            TaskManager::RemoveTask();
        }
        else
        {
            //create app start
            TaskManager::CreateTask();
        }
    }
}