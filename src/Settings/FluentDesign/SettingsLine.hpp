#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include "Theme.hpp"

namespace FluentDesign
{
    class SettingsLine
    {
        int linePadding;
        int leftMargin;

    protected:
        HWND m_hWnd;
        HWND m_hParent;
        HWND m_hChildControl;

        std::wstring m_name;
        std::wstring m_description;

        int m_left, m_top;
        int m_width, m_height;
        bool m_enabled;
        bool m_hovered;

    private:
        virtual LPARAM OnCommand(HWND hwnd, int msg, WPARAM wParam, LPARAM lParam) { return 0; };
        virtual LPARAM OnDrawItem(HWND hwnd, LPDRAWITEMSTRUCT dis) { return 0; };

        FluentDesign::Theme &m_theme;
    public:

        explicit SettingsLine(FluentDesign::Theme &theme);

        explicit SettingsLine(FluentDesign::Theme &theme, HWND hParent,
            const std::wstring &name, const std::wstring &description,
            int x, int y, int width, int height);

        explicit SettingsLine(FluentDesign::Theme &theme, HWND hParent,
            const std::wstring &name, const std::wstring &description,
            int x, int y, int width, int height,
             std::function<HWND(HWND)> createChild);

        ~SettingsLine();

        HWND Create(HWND hParent, const std::wstring &name,
            const std::wstring &description,
            int x, int y, int width, int height);

        HWND GetHWnd() const { return m_hWnd; }

        // Child control management
        void SetChildControl(HWND hChildControl);
        HWND GetChildControl() const { return m_hChildControl; }

        // Text management
        void SetName(const std::wstring &name);
        void SetDescription(const std::wstring &description);
        const std::wstring &GetName() const { return m_name; }
        const std::wstring &GetDescription() const { return m_description; }

        // State management
        void Enable(bool enable = true);
        void Disable();
        bool IsEnabled() const { return m_enabled; }

        // Sizing
        void SetSize(int width, int height);
        void SetLeftMargin(int margin);

    protected:
        // Window procedure and message handlers
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

        // Message handlers
        void OnPaint();
        void OnSize(int width, int height);
        void OnMouseMove();
        void OnMouseLeave();
        void OnEnable(bool enabled);
        void OnLButtonDown();

        // Drawing methods
        void DrawBackground(HDC hdc);
        void DrawText(HDC hdc);
        void UpdateColors();
        void CreateFonts();
        void UpdateBrushes();

        // Layout management
        void UpdateLayout();
        void PositionChildControl();
    };
}