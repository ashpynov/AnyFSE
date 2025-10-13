// ModernSettingsDialog.h
#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

#include "Configuration/Config.hpp"
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/ComboBox.hpp"
#include "FluentDesign/Toggle.hpp"
#include "FluentDesign/TextBox.hpp"
#include "FluentDesign/Button.hpp"
#include "FluentDesign/SettingsLine.hpp"


#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


namespace AnyFSE::Settings
{
    class SettingsDialog
    {
        static const int Layout_OKWidth = 100;
        static const int Layout_CloseWidth = 150;

        static const int Layout_BrowseWidth = 130;
        static const int Layout_BrowseHeight = 32;

        static const int Layout_MarginLeft = 32;
        static const int Layout_MarginTop = 60;
        static const int Layout_MarginRight = 32;
        static const int Layout_MarginBottom = 32;
        static const int Layout_ButtonPadding = 16;
        static const int Layout_ButtonHeight = 32;

        static const int Layout_LineHeight = 67;
        static const int Layout_LinePadding = 8;
        static const int Layout_LineHeightSmall = 45;
        static const int Layout_LinePaddingSmall = 0;
        static const int Layout_LineSmallMargin = 40;

        static const int Layout_CustomSettingsWidth = 220;
        static const int Layout_Layout_CustomSettingsHeight = 40;

        static const int Layout_LauncherComboWidth = 280;
        static const int Layout_LauncherBrowsePadding = 0;
        static const int Layout_LauncherBrowseLineHeight = 57;

        void ShowGroup(int groupIdx, bool show);

    public:
        INT_PTR Show(HINSTANCE hInstance);
        void CenterDialog(HWND hwnd);
        SettingsDialog()
            : m_theme()
            , launcherCombo(m_theme)
            , enterFullscreen(m_theme)
            , customSettings(m_theme)
            , additionalArguments(m_theme)
            , processName(m_theme)
            , title(m_theme)
            , processNameAlt(m_theme)
            , titleAlt(m_theme)
            , browseButton(m_theme)
            , buttonOK(m_theme)
            , buttonClose(m_theme)
        {}

    private:
        static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        INT_PTR InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        void UpdateCombo();
        void UpdateCustomSettings();
        void SaveSettings();

        void OnInitDialog(HWND hwnd);
        void OnBrowseLauncher(HWND hwnd, int editId);
        void OnLauncherChanged(HWND hwnd);
        void OnOk();
        void OnCustomChanged(HWND hwnd);
        void OnAggressiveChanged(HWND hwnd);
        void UpdateControls();

        HINSTANCE m_hInstance;
        HWND m_hDialog;

        bool m_isCustom;
        bool m_isAgressive;
        bool m_enterOnStartup;

        LauncherConfig config;
        std::wstring current;
        std::list<std::wstring> launchers;

        FluentDesign::Theme m_theme;

        FluentDesign::ComboBox launcherCombo;
        FluentDesign::Toggle enterFullscreen;
        FluentDesign::Toggle customSettings;
        FluentDesign::TextBox additionalArguments;
        FluentDesign::TextBox processName;
        FluentDesign::TextBox title;
        FluentDesign::TextBox processNameAlt;
        FluentDesign::TextBox titleAlt;
        FluentDesign::Button browseButton;
        FluentDesign::Button buttonOK;
        FluentDesign::Button buttonClose;

        HWND m_hButtonOk;
        HWND m_hButtonClose;

        std::list<FluentDesign::SettingsLine> settingLines;
        template <class T>
        FluentDesign::SettingsLine& AddSettingsLine(
            ULONG &top, const wstring &name, const wstring &desc,
            T &control, int height, int padding, int contentMargin,
            int contentWidth = Layout_CustomSettingsWidth,
            int contentHeight = Layout_Layout_CustomSettingsHeight);

        FluentDesign::SettingsLine * fseOnStartup;
        FluentDesign::SettingsLine * customSettingsGroup;

        FluentDesign::SettingsLine::State m_customSettingsState;
    };
}

using namespace AnyFSE::Settings;