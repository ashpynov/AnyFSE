// SettingsDialog.cpp
#include "SettingsDialog.hpp"
#include "Resource.h"
#include <commdlg.h>

#include <uxtheme.h>
#include "Tools/Tools.hpp"
#include "Tools/Registry.hpp"
#include "Logging/LogManager.hpp"
#include "SettingsDialog.hpp"
#include "Configuration/Config.hpp"
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/DoubleBufferedPaint.hpp"
#include "TaskManager.hpp"

#define byte ::byte

#include "FluentDesign/Gdiplus.hpp"


#pragma comment(lib, "comctl32.lib")
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
            {   // To enable immersive dark mode (for a dark title bar)
                m_theme.Attach(hwnd);
                OnInitDialog(hwnd);
                CenterDialog(hwnd);
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

                SelectObject(paint.MemDC(), oldFont);
            }
            return FALSE;


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
                OnOk();
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
    FluentDesign::SettingsLine& SettingsDialog::AddSettingsLine(
        ULONG &top, const wstring& name, const wstring& desc, T& control,
        int height, int padding, int contentMargin, int contentWidth, int contentHeight )
    {
        RECT rect;
        GetClientRect(m_hDialog, &rect);
        int width = rect.right - rect.left
            - m_theme.DpiScale(Layout_MarginLeft)
            - m_theme.DpiScale(Layout_MarginRight);

        settingLines.emplace_back(
            m_theme, m_hDialog, name, desc,
                m_theme.DpiScale(Layout_MarginLeft),
                top, width,
                m_theme.DpiScale(height),
            [&](HWND parent) { return control.Create(parent, 0, 0,
                m_theme.DpiScale(contentWidth), m_theme.DpiScale(contentHeight)); });

        top += m_theme.DpiScale(height + padding);
        if (contentMargin)
        {
            settingLines.back().SetLeftMargin(contentMargin);
        }

        return settingLines.back();
    }

    void SettingsDialog::OnInitDialog(HWND hwnd)
    {
        ULONG top = m_theme.DpiScale(Layout_MarginTop);

        FluentDesign::SettingsLine &launcher = AddSettingsLine(top,
            L"Choose home app",
            L"Choose home application for full screen expirience",
            launcherCombo,
            Layout_LineHeight, Layout_LauncherBrowsePadding, 0,
            Layout_LauncherComboWidth );

        launcher.SetFrame(Gdiplus::FrameFlags::SIDE_NO_BOTTOM | Gdiplus::FrameFlags::CORNER_TOP);

        FluentDesign::SettingsLine &browse = AddSettingsLine(top,
            L"",
            L"",
            browseButton,
            Layout_LauncherBrowseLineHeight, Layout_LinePadding, 0,
            Layout_BrowseWidth, Layout_BrowseHeight);

        browse.SetFrame(Gdiplus::FrameFlags::SIDE_NO_TOP | Gdiplus::FrameFlags::CORNER_BOTTOM);

        fseOnStartupLine = &AddSettingsLine(top,
            L"Enter full screen expirience on startup",
            L"",
            fseOnStartupToggle,
            Layout_LineHeight, Layout_LinePadding, 0);

        customSettingsLine = &AddSettingsLine(top,
            L"Use custom settings",
            L"Change monitoring and startups settings for selected home application",
            customSettingsToggle,
            Layout_LineHeight, Layout_LinePaddingSmall, 0);

        customSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Additional arguments",
            L"Command line arguments passed to application",
            additionalArgumentsEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        customSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Primary process name",
            L"Name of home application process",
            processNameEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        customSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Primary window title",
            L"Title of app window when it have been activated",
            titleEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        customSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Secondary process name",
            L"Name of app process for alternative mode",
            processNameAltEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        customSettingsLine->AddGroupItem(&AddSettingsLine(top,
            L"Secondary window title",
            L"Title of app window when it alternative mode have been activated",
            titleAltEdit,
            Layout_LineHeightSmall, Layout_LinePaddingSmall, Layout_LineSmallMargin));

        RECT rect;



        GetClientRect(m_hDialog, &rect);
        rect.top = rect.bottom
            - m_theme.DpiScale(Layout_MarginBottom)
            - m_theme.DpiScale(Layout_ButtonHeight);

        rect.right -= m_theme.DpiScale(Layout_MarginRight);

        m_hButtonOk = okButton.Create(m_hDialog,
            rect.right
                - m_theme.DpiScale(Layout_OKWidth)
                - m_theme.DpiScale(Layout_ButtonPadding)
                - m_theme.DpiScale(Layout_CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout_OKWidth),
            m_theme.DpiScale(Layout_ButtonHeight));

        okButton.SetText(L"Save");

        m_hButtonClose = closeButton.Create(m_hDialog,
            rect.right - m_theme.DpiScale(Layout_CloseWidth),
            rect.top,
            m_theme.DpiScale(Layout_CloseWidth),
            m_theme.DpiScale(Layout_ButtonHeight));

        closeButton.OnChanged += [This = this]()
        {
            EndDialog(This->m_hDialog, IDCANCEL);
        };

        okButton.OnChanged += [This = this]()
        {
            This->OnOk();
        };


        closeButton.SetText(L"Discard and Close");

        launcherCombo.OnChanged += [This = this]()
        {
            This->OnLauncherChanged(This->m_hDialog);
        };

        customSettingsToggle.OnChanged += [This = this]()
        {
            This->OnCustomChanged(This->m_hDialog);
        };

        customSettingsLine->OnChanged += [This = this]()
        {
            This->m_customSettingsState = This->customSettingsLine->GetState();
        };

        browseButton.SetText(L"Browse");
        browseButton.OnChanged += [This = this]()
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
        UpdateControls();
        UpdateCustomSettings();
        m_customSettingsState = customSettingsLine->GetState() != FluentDesign::SettingsLine::Normal
                                ? customSettingsLine->GetState()
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
            if (current != szFile)
            {
                current = szFile;
                Config::GetLauncherSettings(current, config);
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

    void SettingsDialog::OnCustomChanged(HWND hwnd)
    {
        m_isCustom = customSettingsToggle.GetCheck();
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
        Config::GetLauncherDefaults(current, defaults);

        if (defaults.Type == LauncherType::None)
        {
            fseOnStartupLine->Enable(false);
            fseOnStartupToggle.SetCheck(false);
        }
        else
        {
            fseOnStartupLine->Enable();
            fseOnStartupToggle.SetCheck(Config::FseOnStartup);
        }

        bool alwaysSettings = defaults.Type==LauncherType::Custom;
        bool noSettings = defaults.Type == LauncherType::Xbox || defaults.Type == LauncherType::None;

        bool haveSettings = m_isCustom && !noSettings || alwaysSettings;
        bool enableCheck = !alwaysSettings && !noSettings;

        customSettingsToggle.SetCheck(haveSettings);

        FluentDesign::SettingsLine::State state = haveSettings
                ? alwaysSettings
                    ? FluentDesign::SettingsLine::Opened
                    : m_customSettingsState
                : FluentDesign::SettingsLine::Normal;

        customSettingsLine->SetState(state);
        customSettingsLine->Enable(enableCheck);

        if (!haveSettings)
        {
            config = defaults;
            UpdateCustomSettings();
        }
    }

    void SettingsDialog::OnLauncherChanged(HWND hwnd)
    {
        current = launcherCombo.GetCurentValue();;
        Config::GetLauncherSettings(current, config);
        UpdateControls();
        UpdateCustomSettings();
    }


    void SettingsDialog::UpdateCombo()
    {
        launcherCombo.Reset();

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
        additionalArgumentsEdit.SetText(config.StartArg);
        processNameEdit.SetText(config.ProcessName);
        processNameAltEdit.SetText(config.ProcessNameAlt);
        titleEdit.SetText(config.WindowTitle);
        titleAltEdit.SetText(config.WindowTitleAlt);

        // SendDlgItemMessage(m_hDialog, IDC_NOT_AGGRESSIVE_MODE, BM_SETCHECK, (WPARAM)(m_isAgressive && config.Type != LauncherType::Xbox) ? BST_UNCHECKED : BST_CHECKED, 0 );
        // EnableWindow(GetDlgItem(m_hDialog, IDC_NOT_AGGRESSIVE_MODE), config.Type != LauncherType::Xbox);
    }
    void SettingsDialog::SaveSettings()
    {
        const std::wstring gamingConfiguration = L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration";
        const std::wstring startupToGamingHome = L"StartupToGamingHome";
        const std::wstring gamingHomeApp = L"GamingHomeApp";
        const std::wstring xboxApp = L"Microsoft.GamingApp_8wekyb3d8bbwe!Microsoft.Xbox.App";
        const std::wstring anyFSERoot = L"HKCU\\Software\\AnyFSE\\Settings";

        bool removeAnyFSE = false;

        if (config.Type == LauncherType::None)
        {
            Registry::DeleteValue(gamingConfiguration, gamingHomeApp);
            Registry::WriteBool(gamingConfiguration, startupToGamingHome, false);
            removeAnyFSE = true;
        }
        else
        {
            Registry::WriteBool(gamingConfiguration, startupToGamingHome, fseOnStartupToggle.GetCheck());
            Registry::WriteString(gamingConfiguration, gamingHomeApp, xboxApp);

            if (config.Type == LauncherType::Xbox)
            {
                removeAnyFSE = true;
            }
            else
            {
                // save FSE
                Registry::WriteString(anyFSERoot, L"LauncherPath", config.StartCommand);

                Registry::WriteBool(anyFSERoot, L"CustomSettings", customSettingsToggle.GetCheck());
                Registry::WriteString(anyFSERoot, L"StartCommand", config.StartCommand);
                Registry::WriteString(anyFSERoot, L"StartArg", additionalArgumentsEdit.GetText());
                Registry::WriteString(anyFSERoot, L"ProcessName", processNameEdit.GetText());
                Registry::WriteString(anyFSERoot, L"WindowTitle", titleEdit.GetText());
                Registry::WriteString(anyFSERoot, L"ProcessNameAlt", processNameAltEdit.GetText());
                Registry::WriteString(anyFSERoot, L"WindowTitleAlt", titleAltEdit.GetText());
                Registry::WriteString(anyFSERoot, L"IconFile", config.IconFile);
            }
        }
        if (removeAnyFSE)
        {
            Registry::DeleteKey(anyFSERoot);
            // remove appstart
        }
        else
        {
            //create app start
            TaskManager::CreateTask();
        }
    }
}