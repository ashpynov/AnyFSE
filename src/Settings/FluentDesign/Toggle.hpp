#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <functional>
#include "Tools/Event.hpp"
#include "Theme.hpp"

namespace FluentDesign
{
    class Toggle
    {

    private:
        static const int itemHeight = 20;
        static const int itemWidth = 40;
        static const int textWidth = 32;


        HWND hToggle;

        bool buttonPressed;
        bool buttonMouseOver;
        int  pressedPos;
        int  thumbShift;
        int pressedPath;

        bool isChecked;

        static LRESULT CALLBACK ToggleSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void HandleMouse(HWND hWnd, UINT uMsg, LPARAM lParam);
        void DrawButton(HWND hWnd, HDC hdc, RECT rc);

        FluentDesign::Theme &m_theme;

    public:
        Toggle(FluentDesign::Theme& theme);
        Toggle(
            FluentDesign::Theme& theme,
            HWND hParent,
            int x, int y,
            int width, int height);

        HWND Create(HWND hParent, int x, int y, int width, int height);

        ~Toggle();

        void SetCheck(bool isChecked);
        bool GetCheck();
        Event OnChanged;
    };
}