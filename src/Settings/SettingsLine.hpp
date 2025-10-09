#pragma once
#include <windows.h>
#include <string>
#include <functional>

namespace AnyFSE::Settings
{
    class SettingsLine
    {
        static const int nameHeight = 14;
        static const int descHeight = 11;
        static const int linePadding= 4;
        static const int leftMargin = 16;

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
        HFONT m_hNameFont;
        HFONT m_hDescFont;
        HFONT m_hGlyphFont;

        // Drawing resources
        HBRUSH m_hBackgroundBrush;
        HBRUSH m_hHoverBrush;

        HBRUSH m_hControlBrush;
        HBRUSH m_hControlHoveredBrush;
        HBRUSH m_hControlPressedBrush;

        COLORREF m_nameColor;
        COLORREF m_descColor;
        COLORREF m_disabledColor;
        COLORREF m_darkColor;

        COLORREF m_backgroundColor;
        COLORREF m_hoverColor;

        COLORREF m_controlColor;
        COLORREF m_controlHoveredColor;
        COLORREF m_controlPressedColor;

    private:
        virtual HWND CreateControl(HWND hWnd) = 0;
        virtual bool ApplyTheme(bool isDark) = 0;
        virtual LPARAM OnCommand(HWND hwnd, int msg, WPARAM wParam, LPARAM lParam) { return 0; };
        virtual LPARAM OnDrawItem(HWND hwnd, LPDRAWITEMSTRUCT dis) { return 0; };

    public:
        SettingsLine(HWND hParent,
                     const std::wstring &name,
                     const std::wstring &description,
                     int x, int y, int width, int height);

        ~SettingsLine();

        // Window creation
        bool Create();
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
        void GetSize(int &width, int &height) const;

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