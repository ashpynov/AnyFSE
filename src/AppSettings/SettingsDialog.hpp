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
#include "FluentDesign/Static.hpp"
#include "FluentDesign/SettingsLine.hpp"
#include "FluentDesign/ScrollView.hpp"


#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


namespace AnyFSE::App::AppSettings::Settings
{
    using namespace FluentDesign;
    class SettingsDialog
    {

        static const int Layout_OKWidth = 100;
        static const int Layout_CloseWidth = 150;
        static const int Layout_UninstallWidth = 100;

        static const int Layout_BrowseWidth = 130;
        static const int Layout_BrowseHeight = 32;

        static const int Layout_StartupMenuButtonWidth = 24;
        static const int Layout_StartupMenuButtonHeight = 36;

        static const int Layout_StartupAddWidth = 100;
        static const int Layout_StartupAddHeight = 32;

        static const int Layout_MarginLeft = 32;
        static const int Layout_MarginTop = 60;
        static const int Layout_MarginRight = 32;
        static const int Layout_MarginBottom = 32;
        static const int Layout_ButtonPadding = 16;
        static const int Layout_ButtonHeight = 32;

        static const int Layout_LineHeight = 67;
        static const int Layout_LinePadding = 8;
        static const int Layout_LineHeightSmall = 45;
        static const int Layout_LinePaddingSmall = 4;
        static const int Layout_LineSmallMargin = 52;

        static const int Layout_CustomSettingsWidth = 220;
        static const int Layout_Layout_CustomSettingsHeight = 44;

        static const int Layout_LauncherComboWidth = 280;
        static const int Layout_LauncherBrowsePadding = -12;
        static const int Layout_LauncherBrowseLineHeight = 57;

        static const int Layout_BackButtonSize = 34;
        static const int Layout_BackButtonMargin = 4;

        static const int Layout_CaptionButtonWidth = 48;
        static const int Layout_CaptionHeight = 36;
        static const int Layout_CaptionButtonPad = 4;

        void ShowGroup(int groupIdx, bool show);

    public:
        INT_PTR Show(HINSTANCE hInstance);
        void CenterDialog(HWND hwnd);
        SettingsDialog(HWND &hDialog)
            : m_hDialog(hDialog)
            , m_theme()
            , m_captionStatic(m_theme)
            , m_captionBackButton(m_theme)
            , m_captionCloseButton(m_theme)
            , m_captionMinimizeButton(m_theme)
            , m_captionMaximizeButton(m_theme)
            , m_customSettingsState(FluentDesign::SettingsLine::Closed)
            , m_scrollView(m_theme)
            , m_launcherCombo(m_theme)
            , m_troubleLogLevelCombo(m_theme)
            , m_fseOnStartupToggle(m_theme)
            , m_customSettingsToggle(m_theme)
            , m_additionalArgumentsEdit(m_theme)
            , m_processNameEdit(m_theme)
            , m_titleEdit(m_theme)
            , m_classEdit(m_theme)
            , m_processNameAltEdit(m_theme)
            , m_titleAltEdit(m_theme)
            , m_classAltEdit(m_theme)
            , m_browseButton(m_theme)
            , m_okButton(m_theme)
            , m_closeButton(m_theme)
            , m_splashShowAnimationToggle(m_theme)
            , m_splashShowLogoToggle(m_theme)
            , m_splashShowTextToggle(m_theme)
            , m_splashShowVideoToggle(m_theme)
            , m_splashShowVideoLoopToggle(m_theme)
            , m_splashShowVideoMuteToggle(m_theme)
            , m_splashShowVideoPauseToggle(m_theme)
            , m_splashCustomTextEdit(m_theme)
            , m_splashCustomVideoEdit(m_theme)
            , m_troubleAggressiveToggle(m_theme)
            , m_startupAddButton(m_theme)
            , m_customResetButton(m_theme)
        {}

    private:
        static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        INT_PTR InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void InitCustomControls();

        void UpdateCombo();
        void UpdateCustomSettings();
        void SaveSettings();

        void OnInitDialog(HWND hwnd);
        void OnBrowseLauncher();
        void OnLauncherChanged();
        void OnOk();
        void OnMinimize();
        void OnClose();

        void OnShowTextChanged();
        void OnShowLogoChanged();
        void OnShowVideoChanged();
        void OnCustomChanged();
        void OnCustomReset();
        void UpdateCustomResetEnabled();
        void OpenSettingsPage();
        void OpenCustomSettingsPage();
        void OpenSplashSettingsPage();
        void OpenTroubleshootSettingsPage();
        void OpenStartupSettingsPage();
        void OpenMSSettingsStartupApps();
        void UpdateControls();

        HINSTANCE m_hInstance;
        HWND &m_hDialog;

        bool m_isCustom;
        bool m_isAggressive;
        bool m_enterOnStartup;

        std::wstring m_pageName = L"";
        RECT m_breadCrumbRect = { 0 };
        bool m_buttonMouseOver = false;
        bool m_buttonPressed = false;

        LauncherConfig m_config;
        LauncherConfig m_defaultConfig;
        std::wstring m_currentLauncherPath;
        std::list<std::wstring> m_launchersList;

        Theme m_theme;

        Static m_captionStatic;
        Button m_captionBackButton;
        Button m_captionMinimizeButton;
        Button m_captionMaximizeButton;
        Button m_captionCloseButton;

        ScrollView m_scrollView;
        ComboBox m_launcherCombo;
        ComboBox m_troubleLogLevelCombo;
        Toggle m_troubleAggressiveToggle;
        Toggle m_fseOnStartupToggle;
        Toggle m_customSettingsToggle;
        Button m_customResetButton;

        Toggle m_splashShowAnimationToggle;
        Toggle m_splashShowLogoToggle;
        Toggle m_splashShowTextToggle;
        Toggle m_splashShowVideoToggle;
        Toggle m_splashShowVideoLoopToggle;
        Toggle m_splashShowVideoMuteToggle;
        Toggle m_splashShowVideoPauseToggle;

        TextBox m_splashCustomTextEdit;
        TextBox m_splashCustomVideoEdit;
        TextBox m_additionalArgumentsEdit;
        TextBox m_processNameEdit;
        TextBox m_titleEdit;
        TextBox m_classEdit;
        TextBox m_processNameAltEdit;
        TextBox m_titleAltEdit;
        TextBox m_classAltEdit;
        Button m_browseButton;
        Button m_okButton;
        Button m_closeButton;

        HWND m_hScrollView;

        std::list<SettingsLine> m_settingPageList;
        std::list<SettingsLine> m_customSettingPageList;
        std::list<SettingsLine> m_splashSettingPageList;
        std::list<SettingsLine> m_troubleshootSettingPageList;
        std::list<SettingsLine> m_startupSettingPageList;

        std::list<SettingsLine> *m_pActivePageList;

        Button m_startupAddButton;
        std::list<Toggle> m_startupToggles;
        std::list<SettingsLine *> m_pStartupAppLines;

        template <class T>
        SettingsLine& AddSettingsLine(
            std::list<SettingsLine>& pageList,
            ULONG &top, const std::wstring &name, const std::wstring &desc,
            T &control, int height, int padding, int contentMargin,
            int contentWidth = Layout_CustomSettingsWidth,
            int contentHeight = Layout_Layout_CustomSettingsHeight);

        SettingsLine& AddSettingsLine(
            std::list<SettingsLine>& pageList,
            ULONG &top, const std::wstring &name, const std::wstring &desc,
            int height, int padding, int contentMargin,
            int contentWidth = Layout_CustomSettingsWidth,
            int contentHeight = Layout_Layout_CustomSettingsHeight);

        void SwitchActivePage(std::list<SettingsLine> *pPageList, bool back = false);

        void UpdateDpiLayout();

        void UpdateLayout();

        void AddCustomSettingsPage();
        void AddSplashSettingsPage();
        void AddTroubleshootSettingsPage();
        void AddStartupAppLine(const std::wstring &path, const std::wstring &args, bool enabled);
        Toggle *GetStartupLineToggle(SettingsLine *pLine);
        std::wstring GetProductName(const std::wstring &filePath);
        void SetStartupAppLine(SettingsLine *pLine, const std::wstring &path, const std::wstring &args);
        void OnStartupAdd();
        void OnStartupModify(SettingsLine *pLine);
        void OnStartupDelete(SettingsLine *pLine);
        void AddStartupSettingsPage();

        SettingsLine * m_pFseOnStartupLine;
        SettingsLine * m_pCustomSettingsLine;

        SettingsLine * m_pSplashTextLine;
        SettingsLine * m_pSplashLogoLine;
        SettingsLine * m_pSplashVideoLine;

        SettingsLine * m_pSplashPageLine;
        SettingsLine * m_pTroubleshootPageLine;
        SettingsLine * m_pStartupPageLine;
        SettingsLine * m_pStartupPageAppsHeader;

        SettingsLine::State m_customSettingsState;
    };
}

using namespace AnyFSE::App::AppSettings::Settings;