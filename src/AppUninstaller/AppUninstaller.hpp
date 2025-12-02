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

    class AppUninstaller
    {
        private:

        public:
            INT_PTR Show(HINSTANCE hInstance);
            void Uninstall();

            enum Pages : int
            {
                None = -1,
                Welcome = 0,
                Complete,
                Error,
                Count
            };

            AppUninstaller()
                : m_theme()
                , m_captionStatic(m_theme)
                , m_textStatic(m_theme)
                , m_imageStatic(m_theme)
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
            const int Layout_ButtonHeight = 32;
            const int Layout_ButtonPadding = 16;

            const wchar_t * Icon_Delete = L"C:\\Windows\\system32\\imageres.dll,-55";
            const wchar_t * Icon_Error = L"C:\\Windows\\system32\\imageres.dll,-98";
            const wchar_t * Icon_Permission = L"C:\\Windows\\system32\\imageres.dll,-105";
            const wchar_t * Icon_Done = L"C:\\Windows\\system32\\imageres.dll,-1400";

            Theme m_theme;

            std::map<HWND, Gdiplus::RectF> m_designedPositions;

            Static m_imageStatic;
            Static m_captionStatic;
            Static m_textStatic;
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
                const std::function<void()> &callbackLeft=[](){}
            );
            void ShowWelcomePage();
            void ShowCompletePage();
            void ShowErrorPage(const std::wstring& caption, const std::wstring& text, const std::wstring& icon = L"");


            void OnInitDialog(HWND hwnd);
            void OnPaint(HWND hwnd);
            void OnErase(HDC hdc, HWND child);

            void DrawDialog(HDC hdc, RECT clientRect);

            void OnCancel();
            void OnUninstall();
            std::list<std::wstring> ListDir(const std::wstring &path, const std::wstring &mask);
            bool DeleteFiles(const std::wstring &path);
            bool AutoDeleteSelf(const std::wstring &path, bool deleteFolder);
            bool TerminateAnyFSE();
            void OnDone();
    };
}