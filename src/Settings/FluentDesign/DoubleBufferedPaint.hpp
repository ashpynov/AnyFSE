// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include <windows.h>

namespace FluentDesign
{
    class DoubleBuferedPaint
    {
        PAINTSTRUCT m_ps;
        RECT m_rect;
        HDC m_hdc;
        HDC m_hdcMem;
        HBITMAP m_hbmMem;
        HBITMAP m_hOldBitmap;
        int m_width;
        int m_height;
        HWND m_hWnd;

    public:
        DoubleBuferedPaint(HWND hwnd)
            : m_hWnd(hwnd)
        {
            m_hdc = BeginPaint(m_hWnd, &m_ps);
            GetClientRect(m_hWnd, &m_rect);
            m_width = m_rect.right - m_rect.left;
            m_height = m_rect.bottom - m_rect.top;

            // === DOUBLE BUFFERING START ===
            m_hdcMem = CreateCompatibleDC(m_hdc);
            m_hbmMem = CreateCompatibleBitmap(m_hdc, m_width, m_height);
            m_hOldBitmap = (HBITMAP)SelectObject(m_hdcMem, m_hbmMem);

            HWND parent = GetParent(m_hWnd);
            if (parent)
            {
                SendMessage(parent, WM_ERASEBKGND, (WPARAM)m_hdcMem, (LPARAM)m_hWnd);
            }
        }

        HDC MemDC()
        {
            return m_hdcMem;
        }

        RECT ClientRect()
        {
            return m_rect;
        }

        ~DoubleBuferedPaint()
        {
            // === DOUBLE BUFFERING - COPY TO SCREEN ===
            BitBlt(m_hdc, 0, 0, m_width, m_height, m_hdcMem, 0, 0, SRCCOPY);

            // === CLEANUP DOUBLE BUFFERING ===
            SelectObject(m_hdcMem, m_hOldBitmap);
            SelectClipRgn(m_hdc, NULL);
            DeleteObject(m_hbmMem);
            DeleteDC(m_hdcMem);

            EndPaint(m_hWnd, &m_ps);
        }
    };
}