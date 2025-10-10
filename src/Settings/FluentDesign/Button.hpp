#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <functional>
#include "Tools/Event.hpp"
#include "Theme.hpp"

namespace FluentDesign
{
    class Button
    {

    private:
        int m_designWidth;
        int m_designHeight;

        HWND hButton;

        bool buttonPressed;
        bool buttonMouseOver;
        int m_cornerRadius = 8;
        std::wstring m_text;

        static LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void HandleMouse(HWND hWnd, UINT uMsg, LPARAM lParam);
        void DrawButton(HWND hWnd, HDC hdc, RECT rc);

        void UpdateLayout();

        FluentDesign::Theme &m_theme;

    public:
        Button(FluentDesign::Theme& theme);
        Button(
            FluentDesign::Theme& theme,
            HWND hParent,
            int x, int y,
            int width, int height);

        HWND Create(HWND hParent, int x, int y, int width, int height);

        void SetText(const std::wstring& text);

        ~Button();

        Event OnChanged;
    };
}