// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


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