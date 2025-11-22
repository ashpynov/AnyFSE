#include <string>
#include "Theme.hpp"

namespace FluentDesign
{

    class IDialog
    {
        public:
            virtual void Create() = 0;
            virtual float GetFooterDesignHeight() const = 0;
            virtual void OnOK() = 0;
            virtual void OnCancel() = 0;

            virtual INT_PTR InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
            virtual BOOL OnInitDialog(HWND hwnd) = 0;
            virtual BOOL OnDpiChanged(HWND hwnd) = 0;
            virtual BOOL OnPaint(HWND hwnd) = 0;
            virtual BOOL OnErase(HDC hdc, HWND child) = 0;

            virtual SIZE GetDialogDesignSize() = 0;
    };

    class Dialog: public IDialog
    {
        protected:
            Theme m_theme;
            HWND m_hDialog;

            std::map<HWND, Gdiplus::RectF> m_designedPositions;

            void CenterDialog(HWND hwnd);

            BOOL OnInitDialog(HWND hwnd);
            BOOL OnDpiChanged(HWND hwnd);
            BOOL OnPaint(HWND hwnd);
            BOOL OnErase(HDC hdc, HWND child);
            BOOL OnNcCalcSize(HWND hwnd, NCCALCSIZE_PARAMS *params);

            INT_PTR InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
            static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

            INT_PTR Show(HWND hParent);

            Dialog(){}

            /* default implementations */
            float GetFooterDesignHeight() const { return 0.f; };
            void OnCancel();
            void OnOK();
            virtual Theme::Colors GetDialogColor() { return Theme::Colors::Dialog; };
            virtual Theme::Colors GetFooterColor() { return Theme::Colors::Footer; };
    };
}