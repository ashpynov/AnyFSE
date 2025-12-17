#pragma once
#include <windows.h>
#include "Tools/GdiPlus.hpp"
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/Align.hpp"

namespace FluentDesign
{
    class FluentControl
    {
        public:
            typedef BOOL (*GetParentRectFunc)(Theme&, HWND, LPRECT);

        protected:
            static BOOL GetParentRect(Theme &, HWND hWnd, LPRECT prc) { return GetClientRect(hWnd, prc); }

        protected:

            GetParentRectFunc m_getParentRect = GetParentRect;
            Align::Margins m_designMargins;
            Align::Anchor m_anchor = Align::None;

        protected:
            Theme &m_theme;
            HWND m_hWnd;

            FluentControl(Theme &theme)
                : m_theme(theme)
                , m_hWnd(nullptr)
            {}


            FluentControl(Theme &theme, Align::Anchor align, GetParentRectFunc getParentRect = GetParentRect)
                : FluentControl(theme)
            {
                SetAnchor(align, getParentRect);
            }

            virtual ~FluentControl() {}  // for dynamic_cast

            void AnchoredSizePos(HWND hParent, int &x, int &y, int &cx, int &cy);

        public:
            HWND GetHwnd() const { return m_hWnd; }

            void SetAnchor(Align::Anchor anchor, GetParentRectFunc getParentRect = GetParentRect);
            void SetSize(LONG cx, LONG cy);
            void StoreDesign();
            HDWP ReflowControl(HDWP hdwp = nullptr);
    };
}
