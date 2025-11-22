#include <windows.h>

#include "Tools/Gdiplus.hpp"
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/Dialog.hpp"
#include "FluentDesign/Static.hpp"
#include "FluentDesign/Button.hpp"
#include "FluentDesign/TextBox.hpp"

namespace AnyFSE::App::AppSettings::Settings
{
    class StartupAppEditor : protected FluentDesign::Dialog
    {
        private:
            const int Layout_DialogWidth = 480;
            const int Layout_DialogHeight = 280;
            const int Layout_Margins = 16;
            const int Layout_CaptionHeight = 56;
            const int Layout_TextHeight = 30;
            const int Layout_ButtonWidth = 216;
            const int Layout_BrowseButtonWidth = 80;
            const int Layout_ButtonHeight = 32;
            const int Layout_EditHeight = 36;
            const int Layout_ButtonPadding = 16;

            std::wstring &m_refPath;
            std::wstring &m_refArgs;

            FluentDesign::Button m_okButton;
            FluentDesign::Button m_cancelButton;
            FluentDesign::Button m_browseButton;

            FluentDesign::TextBox m_pathEdit;
            FluentDesign::TextBox m_argsEdit;

            FluentDesign::Static m_captionStatic;
            FluentDesign::Static m_pathStatic;
            FluentDesign::Static m_argsStatic;

            StartupAppEditor(std::wstring& refPath, std::wstring &refArgs )
                : FluentDesign::Dialog()
                , m_refPath(refPath)
                , m_refArgs(refArgs)
                , m_okButton(m_theme)
                , m_cancelButton(m_theme)
                , m_browseButton(m_theme)
                , m_pathEdit(m_theme)
                , m_argsEdit(m_theme)
                , m_captionStatic(m_theme)
                , m_pathStatic(m_theme)
                , m_argsStatic(m_theme)
            {}

            void OnOk();
            void OnCancel();
            void OnBrowse();

            void ValidatePath();

        public:
            static INT_PTR EditApp(HWND hParent, std::wstring &refPath, std::wstring &refArgs);

            /** Interface **/
            void Create();
            float GetFooterDesignHeight() const { return float(Layout_Margins + Layout_ButtonHeight + Layout_ButtonPadding); }
            SIZE GetDialogDesignSize() { return SIZE{Layout_DialogWidth, Layout_DialogHeight}; }
            FluentDesign::Theme::Colors GetDialogColor() { return FluentDesign::Theme::Colors::Panel; };
            FluentDesign::Theme::Colors GetFooterColor() { return FluentDesign::Theme::Colors::Footer; };
    };
}