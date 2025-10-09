#include "SettingsLine.hpp"
#include "Logging/LogManager.hpp"
#include <commctrl.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <vssym32.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace AnyFSE::Settings
{
    static Logger log = LogManager::GetLogger("SettingsLine");
    // Window class registration
    static const wchar_t *SETTINGS_LINE_CLASS = L"SettingsLineClass";
    static bool s_classRegistered = false;

    SettingsLine::SettingsLine(HWND hParent,
                               const std::wstring &name,
                               const std::wstring &description,
                               int x, int y, int width, int height)
        : m_hWnd(NULL)
        , m_hParent(hParent)
        , m_hChildControl(NULL)
        , m_name(name)
        , m_description(description)
        , m_left(x)
        , m_top(y)
        , m_width(width)
        , m_height(height)
        , m_enabled(true)
        , m_hovered(false)
        , m_hNameFont(NULL)
        , m_hDescFont(NULL)
        , m_hBackgroundBrush(NULL)
        , m_hHoverBrush(NULL)
    {

        UpdateColors();
    }

    SettingsLine::~SettingsLine()
    {
        if (m_hNameFont)
            DeleteObject(m_hNameFont);
        if (m_hDescFont)
            DeleteObject(m_hDescFont);
        if (m_hBackgroundBrush)
            DeleteObject(m_hBackgroundBrush);
        if (m_hHoverBrush)
            DeleteObject(m_hHoverBrush);
    }

    bool SettingsLine::Create()
    {
        // Register window class if needed
        if (!s_classRegistered)
        {
            WNDCLASSEX wc = {};
            wc.cbSize = sizeof(WNDCLASSEX);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = SettingsLine::WndProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = sizeof(SettingsLine *);
            wc.hInstance = GetModuleHandle(NULL);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = NULL; // We'll handle background drawing
            wc.lpszClassName = SETTINGS_LINE_CLASS;

            if (!RegisterClassEx(&wc))
            {
                return false;
            }
            s_classRegistered = true;
        }

        // Calculate position and size
        RECT parentRect;
        GetClientRect(m_hParent, &parentRect);

        // Create the window
        m_hWnd = CreateWindowEx(
            0,
            SETTINGS_LINE_CLASS,
            L"",
            WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN,
            m_left, m_top, m_width, m_height,
            m_hParent,
            NULL,
            GetModuleHandle(NULL),
            this);

        if (!m_hWnd)
        {
            return false;
        }

        // Create fonts and brushes
        CreateFonts();
        UpdateBrushes();

        SetChildControl(CreateControl(m_hWnd));

        return true;
    }

    void SettingsLine::CreateFonts()
    {
        // Get system font
        NONCLIENTMETRICS ncm = {};
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);

        // Create name font (slightly larger/bolder)
        m_hNameFont = CreateFontIndirect(&ncm.lfMessageFont);
        LOGFONT nameFont = ncm.lfMessageFont;
        nameFont.lfHeight = (LONG)(ncm.lfMessageFont.lfHeight * 1.1);
        nameFont.lfWeight = FW_NORMAL;
        m_hNameFont = CreateFontIndirect(&nameFont);

        // Create description font (slightly smaller, normal weight)
        LOGFONT descFont = ncm.lfMessageFont;
        descFont.lfHeight = (LONG)(ncm.lfMessageFont.lfHeight * 0.9);
        m_hDescFont = CreateFontIndirect(&descFont);
    }

    void SettingsLine::UpdateBrushes()
    {
        // Get system colors for modern appearance
        if (m_hBackgroundBrush) DeleteObject(m_hBackgroundBrush);
        m_hBackgroundBrush = CreateSolidBrush(m_backgroundColor);

        if (m_hHoverBrush) DeleteObject(m_hHoverBrush);
        m_hHoverBrush = CreateSolidBrush(m_hoverColor); // Light gray hover
    }

    void SettingsLine::UpdateColors()
    {
        m_nameColor = RGB(240, 240, 240);
        m_descColor = RGB(200, 200, 200);
        m_disabledColor = GetSysColor(COLOR_GRAYTEXT);
        m_backgroundColor = RGB(55, 55, 55);
        m_hoverColor = RGB(55, 55, 55);

        UpdateBrushes();
    }

    // Window procedure
    LRESULT CALLBACK SettingsLine::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        SettingsLine *pThis = nullptr;

        if (message == WM_NCCREATE)
        {
            CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
            pThis = reinterpret_cast<SettingsLine *>(pCreate->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->m_hWnd = hWnd;
        }
        else
        {
            pThis = reinterpret_cast<SettingsLine *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        }

        if (pThis)
        {
            return pThis->HandleMessage(message, wParam, lParam);
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    LRESULT SettingsLine::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_PAINT:
            OnPaint();
            return 0;

        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_MOUSEMOVE:
            OnMouseMove();
            return 0;

        case WM_MOUSELEAVE:
            OnMouseLeave();
            return 0;

        case WM_LBUTTONDOWN:
            OnLButtonDown();
            return 0;

        case WM_ENABLE:
            OnEnable(wParam != 0);
            return 0;

        case WM_ERASEBKGND:
            return 1; // We handle background in WM_PAINT

        case WM_COMMAND:
            return OnCommand(m_hChildControl, message, wParam, lParam);
            break;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            // Forward child control colors to parent for consistent theming
            return (LRESULT)m_hBackgroundBrush;
        }

        return DefWindowProc(m_hWnd, message, wParam, lParam);
    }

    void SettingsLine::OnPaint()
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hWnd, &ps);

        // Double buffering to prevent flicker
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, m_width, m_height);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

        // Draw background
        DrawBackground(hdcMem);

        // Draw text
        DrawText(hdcMem);

        // Copy to screen
        BitBlt(hdc, 0, 0, m_width, m_height, hdcMem, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);

        EndPaint(m_hWnd, &ps);
    }

    void SettingsLine::DrawBackground(HDC hdc)
    {
        HBRUSH hBrush = m_hovered ? m_hHoverBrush : m_hBackgroundBrush;
        RECT rect = {0, 0, m_width, m_height};
        FillRect(hdc, &rect, hBrush);
    }

    void SettingsLine::DrawText(HDC hdc)
    {
        RECT rect;
        GetClientRect(m_hWnd, &rect);

        COLORREF textColor = m_enabled ? m_nameColor : m_disabledColor;
        COLORREF descColor = m_enabled ? m_descColor : m_disabledColor;

        SetBkMode(hdc, TRANSPARENT);

        // Draw name
        rect.left += 10;   // Margin
        rect.right -= 160; // Space for child control

        SelectObject(hdc, m_hNameFont);
        SetTextColor(hdc, textColor);

        RECT nameRect = rect;
        nameRect.top += 8;
        nameRect.bottom = nameRect.top + 20;

        ::DrawText(hdc, m_name.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Draw description
        SelectObject(hdc, m_hDescFont);
        SetTextColor(hdc, descColor);

        RECT descRect = rect;
        descRect.top += 30;
        descRect.bottom = descRect.top + 16;

        ::DrawText(hdc, m_description.c_str(), -1, &descRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    void SettingsLine::OnSize(int width, int height)
    {
        m_width = width;
        m_height = height;
        UpdateLayout();
    }

    void SettingsLine::OnMouseMove()
    {
        if (!m_hovered)
        {
            m_hovered = true;
            InvalidateRect(m_hWnd, NULL, FALSE);

            // Track mouse leave
            TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
            TrackMouseEvent(&tme);
        }
    }

    void SettingsLine::OnMouseLeave()
    {
        m_hovered = false;
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    void SettingsLine::OnEnable(bool enabled)
    {
        m_enabled = enabled;
        if (m_hChildControl)
        {
            EnableWindow(m_hChildControl, enabled);
        }
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    void SettingsLine::OnLButtonDown()
    {
        // Focus the child control when clicked
        if (m_hChildControl && m_enabled)
        {
            SetFocus(m_hChildControl);
        }
    }

    void SettingsLine::UpdateLayout()
    {
        PositionChildControl();
        InvalidateRect(m_hWnd, NULL, TRUE);
    }

    void SettingsLine::PositionChildControl()
    {
        if (!m_hChildControl)
            return;

        RECT rc;
        GetWindowRect(m_hChildControl, &rc);
        MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, 2);

        int controlWidth = rc.right - rc.left;
        int controlHeight = rc.bottom - rc.top;
        int controlY = (m_height - controlHeight) / 2;

        SetWindowPos(m_hChildControl, NULL,
                     m_width - controlWidth - 10,
                     controlY,
                     0, 0,
                     SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);
    }

    // Public methods implementation
    void SettingsLine::SetChildControl(HWND hChildControl)
    {
        m_hChildControl = hChildControl;
        SetParent(m_hChildControl, m_hWnd);
        PositionChildControl();
        ApplyTheme( true );
        EnableWindow(m_hChildControl, m_enabled);
    }

    void SettingsLine::SetName(const std::wstring &name)
    {
        m_name = name;
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    void SettingsLine::SetDescription(const std::wstring &description)
    {
        m_description = description;
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    void SettingsLine::Enable(bool enable)
    {
        m_enabled = enable;
        EnableWindow(m_hWnd, enable);
        if (m_hChildControl)
        {
            EnableWindow(m_hChildControl, enable);
        }
        InvalidateRect(m_hWnd, NULL, FALSE);
    }

    void SettingsLine::Disable()
    {
        Enable(false);
    }

    void SettingsLine::SetSize(int width, int height)
    {
        m_width = width;
        m_height = height;
        SetWindowPos(m_hWnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }
}