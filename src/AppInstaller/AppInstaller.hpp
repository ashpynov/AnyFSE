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
                , m_currentPage(Pages::None)

                , m_welcomeImageStatic(m_theme)
                , m_welcomeCaptionStatic(m_theme)
                , m_welcomeTextStatic(m_theme)
                , m_welcomeNextButton(m_theme)
                , m_welcomeCancelButton(m_theme)

                , m_pathCaptionStatic(m_theme)
                , m_pathTextStatic(m_theme)
                , m_pathEdit(m_theme)
                , m_pathImageStatic(m_theme)
                , m_pathBrowseButton(m_theme)
                , m_pathNextButton(m_theme)
                , m_pathCancelButton(m_theme)

                , m_progressImageStatic(m_theme)
                , m_progressTextStatic(m_theme)
                , m_progressCaptionStatic(m_theme)
                , m_progressCancelButton(m_theme)

                , m_completeImageStatic(m_theme)
                , m_completeCaptionStatic(m_theme)
                , m_completeTextStatic(m_theme)
                , m_completeDoneButton(m_theme)
                , m_completeSettingsButton(m_theme)

                , m_errorImageStatic(m_theme)
                , m_errorCaptionStatic(m_theme)
                , m_errorTextStatic(m_theme)
                , m_errorCloseButton(m_theme)
            {}

        private:
            HINSTANCE m_hInstance;
            HWND m_hDialog;
            Pages m_currentPage;

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

            const wchar_t * Icon_Browse = L"C:\\Windows\\system32\\imageres.dll,-1025";
            const wchar_t * Icon_Error = L"C:\\Windows\\system32\\imageres.dll,-98";
            const wchar_t * Icon_Permission = L"C:\\Windows\\system32\\imageres.dll,-105";
            const wchar_t * Icon_Done = L"C:\\Windows\\system32\\imageres.dll,-1400";
            const wchar_t * Icon_Progress = L"C:\\Windows\\system32\\imageres.dll,-5357";

            Theme m_theme;
            std::vector<std::list<HWND>> m_dialogPages;

            std::map<HWND, Gdiplus::RectF> m_designedPositions;

            Static m_welcomeImageStatic;
            Static m_welcomeCaptionStatic;
            Static m_welcomeTextStatic;
            Button m_welcomeNextButton;
            Button m_welcomeCancelButton;

            Static m_pathImageStatic;
            Static m_pathCaptionStatic;
            Static m_pathTextStatic;
            TextBox m_pathEdit;
            Button m_pathBrowseButton;
            Button m_pathNextButton;
            Button m_pathCancelButton;

            Static m_progressImageStatic;
            Static m_progressCaptionStatic;
            Static m_progressTextStatic;
            Button m_progressCancelButton;

            Static m_completeImageStatic;
            Static m_completeCaptionStatic;
            Static m_completeTextStatic;
            Button m_completeDoneButton;
            Button m_completeSettingsButton;

            Static m_errorImageStatic;
            Static m_errorCaptionStatic;
            Static m_errorTextStatic;
            Button m_errorCloseButton;

        private:
            void CenterDialog(HWND hwnd);
            static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
            INT_PTR InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
            void CreatePages();
            std::list<HWND> CreateWelcomePage();
            std::list<HWND> CreatePathPage();
            std::list<HWND> CreateProgressPage();
            std::list<HWND> CreateCompletePage();
            std::list<HWND> CreateErrorPage();
            void GoToPage(Pages pageId);
            void SetCurrentProgress(const std::wstring &status);

            void ShowError(const std::wstring& caption, const std::wstring& text, const std::wstring& icon = L"");

            void OnInitDialog(HWND hwnd);
            void OnPaint(HWND hwnd);
            void OnErase(HDC hdc, HWND child);
            void DrawDialog(HDC hdc, RECT clientRect);
            void CheckPath();
            bool IsValidPath(const std::wstring &pathStr);

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