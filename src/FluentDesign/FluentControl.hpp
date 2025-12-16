#pragma once
#include <windows.h>
#include "FluentDesign/Theme.hpp"

namespace FluentDesign
{
    class FluentControl
    {
        protected:
            Theme &m_theme;
            HWND m_hWnd;

            FluentControl(Theme& theme)
                : m_theme(theme)
                , m_hWnd(nullptr)
            {}

        public:
            HWND GetHwnd() const { return m_hWnd; }
    };
}
