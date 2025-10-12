#include "SettingsLine.hpp"
#include "Logging/LogManager.hpp"
#include <commctrl.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <vssym32.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace FluentDesign
{
    static Logger log = LogManager::GetLogger("SettingsLine");

    // Window class registration
    static const wchar_t *SETTINGS_LINE_CLASS = L"SettingsLineClass";
    static bool s_classRegistered = false;

    SettingsLine::SettingsLine(FluentDesign::Theme& theme)
        : m_theme(theme)
        , m_enabled(true)
        , m_hovered(false)
        , m_hWnd(NULL)
        , linePadding(4)
        , leftMargin(16)
        , m_state(State::Normal)
    {
    }

    SettingsLine::SettingsLine(
            FluentDesign::Theme& theme,
            HWND hParent,
            const std::wstring &name,
            const std::wstring &description,
            int x, int y, int width, int height)
        : SettingsLine(theme)
    {
        Create(hParent, name, description, x, y, width, height);
    }

    SettingsLine::SettingsLine(
            FluentDesign::Theme& theme,
            HWND hParent,
            const std::wstring &name,
            const std::wstring &description,
            int x, int y, int width, int height,
            std::function<HWND(HWND)> createChild)
        : SettingsLine(theme, hParent, name, description, x,y, width, height)
    {
        SetChildControl(createChild(GetHWnd()));
    }

    SettingsLine::~SettingsLine()
    {
        log.Debug("SetingsLineDestroy");
    }

    HWND SettingsLine::Create(
        HWND hParent,
        const std::wstring &name,
        const std::wstring &description,
        int x, int y,
        int width, int height
    )
    {

        m_hParent = hParent;
        m_name = name;
        m_description = description;
        m_left = x;
        m_top = y;
        m_width = width;
        m_height = height;

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
            return NULL;
        }

        return m_hWnd;
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

        case WM_NCMOUSELEAVE:
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
            {
                if(lParam)
                {
                    RECT childRect;
                    GetClientRect((HWND)lParam, &childRect);
                    DrawBackground((HDC)wParam, childRect);
                }
            }
            return 1; // We handle background in WM_PAINT

        case WM_COMMAND:
            return OnCommand(m_hChildControl, message, wParam, lParam);
            break;
        case WM_DRAWITEM:
            return OnDrawItem(m_hChildControl, (LPDRAWITEMSTRUCT)lParam);

        }

        return DefWindowProc(m_hWnd, message, wParam, lParam);
    }

    void SettingsLine::Invalidate(BOOL bErase)
    {
        InvalidateRect(m_hWnd, NULL, bErase);
        if (m_hChildControl)
        {
            InvalidateRect(m_hChildControl, NULL, bErase);
        }
    }

    void SettingsLine::OnPaint()
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hWnd, &ps);

        // Double buffering to prevent flicker
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, m_width, m_height);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
        RECT rect = {0, 0, m_width, m_height};
        // Draw background
        DrawBackground(hdcMem, rect);

        // Draw text
        DrawText(hdcMem);
        if (m_state != Normal)
        {
            DrawChevron(hdcMem);
        }

        // Copy to screen
        BitBlt(hdc, 0, 0, m_width, m_height, hdcMem, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);

        EndPaint(m_hWnd, &ps);
    }

    void SettingsLine::DrawBackground(HDC hdc, const RECT &rect)
    {
        HBRUSH hBrush = CreateSolidBrush( m_hovered ? m_theme.GetColorRef(Theme::Colors::PanelHover)
                                                    : m_theme.GetColorRef(Theme::Colors::Panel)
        );

        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);
    }

    void SettingsLine::DrawText(HDC hdc)
    {
        RECT rect;
        GetClientRect(m_hWnd, &rect);

        COLORREF textColor = m_enabled ? m_theme.GetColorRef(Theme::Colors::Text)
                                       : m_theme.GetColorRef(Theme::Colors::TextDisabled);

        COLORREF descColor = m_enabled ? m_theme.GetColorRef(Theme::Colors::TextSecondary)
                                       : m_theme.GetColorRef(Theme::Colors::TextDisabled);

        SetBkMode(hdc, TRANSPARENT);

        // Draw name
        rect.left += m_theme.DpiScale(leftMargin);   // Margin
        //rect.right -= 160; // Space for child control

        SelectObject(hdc, m_theme.GetFont_Text());
        SetTextColor(hdc, textColor);

        int height = m_theme.GetSize_Text();
        if (!m_description.empty())
        {
            height += m_theme.DpiScale(linePadding) + m_theme.GetSize_TextSecondary();
        }

        RECT nameRect = rect;
        nameRect.top = (rect.bottom - rect.top - height) / 2;
        nameRect.bottom = nameRect.top + m_theme.GetSize_Text();

        ::DrawText(hdc, m_name.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

        // Draw description
        SelectObject(hdc, m_theme.GetFont_TextSecondary());
        SetTextColor(hdc, descColor);

        RECT descRect = rect;
        descRect.top = nameRect.bottom + m_theme.DpiScale(linePadding);
        descRect.bottom = descRect.top + m_theme.GetSize_TextSecondary();

        ::DrawText(hdc, m_description.c_str(), -1, &descRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE| DT_NOCLIP);
    }

    void SettingsLine::DrawChevron(HDC hdc)
    {
        RECT rect;
        GetClientRect(m_hWnd, &rect);

        rect.left = rect.right - m_theme.DpiScale(CHEVRON_SIZE + 20);
        rect.right = rect.left + m_theme.DpiScale(CHEVRON_SIZE);
        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, m_theme.GetFont_GlyphNormal());
        SetTextColor(hdc, m_theme.GetColorRef(m_enabled ? FluentDesign::Theme::Text : FluentDesign::Theme::TextDisabled));

        rect.top = (rect.bottom - rect.top - m_theme.GetSize_Text()) / 2;
        rect.bottom = rect.top + m_theme.GetSize_Text();

        WCHAR *chevron = L"";
        switch (m_state)
        {
            case State::Opened:
                chevron = L"\xE70E";
                break;
            case State::Closed:
                chevron = L"\xE70D";
                break;
            case State::Next:
                chevron = L"\xE76c";
                break;
            case State::Link:
                chevron = L"\xe8a7";
                break;
        }

        ::DrawText(hdc, chevron, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
    }

    void SettingsLine::OnSize(int width, int height)
    {
        m_width = width;
        m_height = height;
        UpdateLayout();
    }

    void SettingsLine::OnMouseMove()
    {
        if (!m_hovered && m_state != State::Normal)
        {
            m_hovered = true;
            Invalidate();

            // Track mouse leave
            TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
            TrackMouseEvent(&tme);
        }
    }

    void SettingsLine::OnMouseLeave()
    {
        if (m_hChildControl && m_state != State::Normal)
        {
            POINT pt;
            GetCursorPos(&pt);
            HWND hWndUnderCursor = WindowFromPoint(pt);

            if (hWndUnderCursor == m_hWnd)
            {
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
                TrackMouseEvent(&tme);
            }
            else if (hWndUnderCursor != m_hChildControl)
            {
                Invalidate();
                m_hovered = false;
            }
        }
        else
        {
            Invalidate();
            m_hovered = false;
        }
    }

    void SettingsLine::OnEnable(bool enabled)
    {
        m_enabled = enabled;
        if (m_hChildControl)
        {
            EnableWindow(m_hChildControl, enabled);
        }
        Invalidate();
    }

    void SettingsLine::OnLButtonDown()
    {
        // Focus the child control when clicked
        if (m_state == State::Opened || m_state == State::Closed )
        {
            SetState(m_state == State::Closed ?  State::Opened : State::Closed);
            OnChanged.Notify();
        }
    }

    void SettingsLine::UpdateLayout()
    {
        PositionChildControl();
        if (m_groupItems.size() > 0)
        {
            bool bShow = m_state == State::Opened;

            for (auto& gr : m_groupItems)
            {
                ShowWindow(gr->m_hWnd, bShow ? SW_SHOW : SW_HIDE);
            }
        }
        Invalidate();
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

        int marginRight = m_theme.DpiScale(20);
        if( m_state != State::Normal )
        {
            marginRight += m_theme.DpiScale(CHEVRON_SIZE * 2);
        }

        SetWindowPos(m_hChildControl, NULL,
                     m_width - controlWidth - marginRight,
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
        EnableWindow(m_hChildControl, m_enabled);
        ShowWindow(m_hChildControl, SW_SHOWNOACTIVATE);
    }

    void SettingsLine::SetName(const std::wstring &name)
    {
        m_name = name;
        Invalidate();
    }

    void SettingsLine::SetDescription(const std::wstring &description)
    {
        m_description = description;
        Invalidate();
    }

    void SettingsLine::Enable(bool enable)
    {
        m_enabled = enable;
        EnableWindow(m_hWnd, enable);
        if (m_hChildControl)
        {
            EnableWindow(m_hChildControl, enable);
        }
        Invalidate();
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
    void SettingsLine::SetLeftMargin(int margin)
    {
        leftMargin = margin;
        Invalidate();
    }
    void SettingsLine::SetState(State state)
    {
        m_state = state;
        UpdateLayout();
    }
    void SettingsLine::AddGroupItem(SettingsLine *groupItem)
    {
        m_groupItems.push_back(groupItem);
    }
}