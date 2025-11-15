// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>

#include <list>
#include <map>
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/Button.hpp"
#include "FluentDesign/TextBox.hpp"
#include "FluentDesign/Static.hpp"


namespace AnyFSE
{
#ifndef VER_VERSION_STR
#define VER_VERSION_STR "1.2.3"
#endif

#define IDR_EMBEDDED_ZIP 3

#ifndef APP_VERSION
#define APP_VERSION VER_VERSION_STR
#endif

    using namespace FluentDesign;

    class AppInstaller
    {
        private:

        public:
            INT_PTR Show(HINSTANCE hInstance);

            enum Pages : int
            {
                None = -1,
                Welcome = 0,
                Path,
                Progress,
                Complete,
                Error,
                Count
            };

            AppInstaller()
                : m_theme()
                , m_captionStatic(m_theme)
                , m_textStatic(m_theme)
                , m_pathEdit(m_theme)
                , m_imageStatic(m_theme)
                , m_browseButton(m_theme)
                , m_leftButton(m_theme)
                , m_rightButton(m_theme)
            {}

        private:
            HINSTANCE m_hInstance;
            HWND m_hDialog;

            const int Layout_DialogWidth = 480;
            const int Layout_DialogHeight = 280;
            const int Layout_Margins = 16;
            const int Layout_ImageWidth = 48;
            const int Layout_ImageTop = 6;
            const int Layout_CaptionHeight = 48;
            const int Layout_TextHeight = 48;
            const int Layout_ButtonWidth = 100;
            const int Layout_BrowseButtonWidth = 80;
            const int Layout_ButtonHeight = 32;
            const int Layout_EditHeight = 36;
            const int Layout_ButtonPadding = 16;

            const wchar_t * Icon_EULA = L"C:\\Windows\\system32\\imageres.dll,-81";
            const wchar_t * Icon_Browse = L"C:\\Windows\\system32\\imageres.dll,-1025";
            const wchar_t * Icon_Error = L"C:\\Windows\\system32\\imageres.dll,-98";
            const wchar_t * Icon_Permission = L"C:\\Windows\\system32\\imageres.dll,-105";
            const wchar_t * Icon_Done = L"C:\\Windows\\system32\\imageres.dll,-1400";
            const wchar_t * Icon_Progress = L"C:\\Windows\\system32\\imageres.dll,-5357";

            Theme m_theme;

            std::map<HWND, Gdiplus::RectF> m_designedPositions;
            std::list<HWND> m_controls;

            Static m_imageStatic;
            Static m_captionStatic;
            Static m_textStatic;
            TextBox m_pathEdit;
            Button m_browseButton;
            Button m_leftButton;
            Button m_rightButton;

        private:
            void CenterDialog(HWND hwnd);
            static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
            INT_PTR InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

            std::list<HWND> CreatePage();
            void ShowPage(
                const std::wstring &icon,
                const std::wstring &caption,
                const std::wstring &text,
                const std::wstring &buttonRight,
                const std::function<void()> &callbackRight,
                const std::wstring &buttonLeft=L"",
                const std::function<void()> &callbackLeft=[](){},
                bool showBrowse=false
            );
            void ShowWelcomePage();
            void ShowLicensePage();
            void ShowPathPage();
            void ShowProgressPage();
            void ShowCompletePage();
            void ShowErrorPage(const std::wstring& caption, const std::wstring& text, const std::wstring& icon = L"");


            void OnInitDialog(HWND hwnd);
            void OnPaint(HWND hwnd);
            void OnErase(HDC hdc, HWND child);

            void DrawDialog(HDC hdc, RECT clientRect);
            void CheckPath();
            bool IsValidPath(const std::wstring &pathStr);
            void SetCurrentProgress(const std::wstring &status);

            void OnSelectPath();
            void OnBrowse();
            void OnCancel();
            void OnInstall();
            void OnSettings();
            void OnDone();
            void StartAnyFSE(bool bSettings);
            bool TerminateAnyFSE();
            bool ExtractEmbeddedZip(const std::wstring &path);
    };
}