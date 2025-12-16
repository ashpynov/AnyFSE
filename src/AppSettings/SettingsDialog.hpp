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

#include "AppSettings/SettingsLayout.hpp"
#include "AppSettings/SettingsPages/LauncherPage.hpp"
#include "AppSettings/SettingsPages/UpdatePage.hpp"
#include "AppSettings/SettingsPages/TroubleshootPage.hpp"
#include "AppSettings/SettingsPages/SplashPage.hpp"
#include "AppSettings/SettingsPages/StartupPage.hpp"
#include "AppSettings/SettingsPages/SupportPage.hpp"

#include "Updater/Updater.hpp"


#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


namespace AnyFSE::App::AppSettings::Settings
{
    using namespace FluentDesign;
    class SettingsDialog
    {
    public:

        // SettingsDialog_Update
        static const UINT WM_UPDATE_NOTIFICATION = WM_USER + 2;

        HWND GetHwnd() { return m_hDialog; }

        template <class T>
        SettingsLine& AddSettingsLine(
            std::list<SettingsLine>& pageList,
            ULONG &top, const std::wstring &name, const std::wstring &desc,
            T &control, int height, int padding, int contentMargin,
            int contentWidth = Layout::CustomSettingsWidth,
            int contentHeight = Layout::CustomSettingsHeight);

        SettingsLine& AddSettingsLine(
            std::list<SettingsLine>& pageList,
            ULONG &top, const std::wstring &name, const std::wstring &desc,
            int height, int padding, int contentMargin,
            int contentWidth = Layout::CustomSettingsWidth,
            int contentHeight = Layout::CustomSettingsHeight);

        void SwitchActivePage(const std::wstring& pageName, std::list<SettingsLine> *pPageList, bool back = false);
        void UpdateLayout();
        void UpdateLine(SettingsLine *line);

    public:
        INT_PTR Show(HINSTANCE hInstance);
        void CenterDialog(HWND hwnd);
        void StoreWindowPlacement();
        void RestoreWindowPlacement();
        SettingsDialog()
            : m_theme()
            , m_captionStatic(m_theme)
            , m_captionBackButton(m_theme)
            , m_captionCloseButton(m_theme)
            , m_captionMinimizeButton(m_theme)
            , m_captionMaximizeButton(m_theme)


            , m_okButton(m_theme)
            , m_closeButton(m_theme)

            , m_pActivePageList(nullptr)

            // SettingsDialog_Update
            , m_updateCheckButton(m_theme)
            , m_updateCurrentText(m_theme)
            , m_updateButton(m_theme)
            , m_scrollView(m_theme)
        {
            m_pages.push_back(new Page::LauncherPage(m_theme, *this));
            m_pages.push_back(new Page::SplashPage(m_theme, *this));
            m_pages.push_back(new Page::UpdatePage(m_theme, *this));
            m_pages.push_back(new Page::TroubleshootPage(m_theme, *this));
            m_pages.push_back(new Page::StartupPage(m_theme, *this));
            m_pages.push_back(new Page::SupportPage(m_theme, *this));
        }

        ~SettingsDialog()
        {
            for (auto& page : m_pages)
            {
                delete(page);
            }
        }

    private:
        static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        INT_PTR InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void GetDialogRect(RECT *prc);

        void SaveSettings();

        void OnInitDialog(HWND hwnd);

        void OnOk();
        void OnMinimize();
        void OnMaximize();
        void UpdateMaximizeButtonIcon();
        void OnClose();
        void OpenSettingsPage();

        HINSTANCE m_hInstance = nullptr;
        HWND m_hDialog = nullptr;


        std::wstring m_pageName = L"";
        RECT m_breadCrumbRect = { 0 };
        bool m_buttonMouseOver = false;
        bool m_buttonPressed = false;


        Theme m_theme;

        Static m_captionStatic;
        Button m_captionBackButton;
        Button m_captionMinimizeButton;
        Button m_captionMaximizeButton;
        Button m_captionCloseButton;

        ScrollView m_scrollView;

        Button m_okButton;
        Button m_closeButton;

        HWND m_hScrollView = nullptr;

        std::list<SettingsLine> m_settingPageList;
        std::list<SettingsLine> *m_pActivePageList = nullptr;


        void UpdateDpiLayout();
        void ArrangeControls();
        HWND GetMainWindow();

        void AddCaption();
        void AddScrollPanel();
        void AddMinimizeButton();
        void AddMaximizeButton();
        void AddCloseButton();

        Button m_updateCheckButton;
        Static m_updateCurrentText;
        Button m_updateButton;

        void AddUpdateControls();
        void MoveUpdateControls();
        bool SetVersionStatus(const Updater::UpdateInfo& uiInfo);
        void OnCheckUpdate();
        void OnUpdateNetworkRestore();
        void OnUpdateNotification();
        void UpdateVersionStatusDelay(UINT delay);
        void OnShowVersion();
        void OnUpdate();

        std::list<Page::SettingsPage *> m_pages;
    };
}

#include "SettingsDialog.tpp"

using namespace AnyFSE::App::AppSettings::Settings;