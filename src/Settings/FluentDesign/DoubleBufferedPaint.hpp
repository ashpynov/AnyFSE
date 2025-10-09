
#include <windows.h>

namespace FluentDesign
{
    class DoubleBuferedPaint
    {
        PAINTSTRUCT ps;
        RECT rect;
        HDC hdc;
        HDC hdcMem;
        HBITMAP hbmMem;
        HBITMAP hOldBitmap;
        int width;
        int height;
        HWND hWnd;

    public:
        DoubleBuferedPaint(HWND hwnd)
            : hWnd(hwnd)
        {
            hdc = BeginPaint(hWnd, &ps);
            GetClientRect(hWnd, &rect);

            width = rect.right - rect.left;
            height = rect.bottom - rect.top;

            // === DOUBLE BUFFERING START ===
            hdcMem = CreateCompatibleDC(hdc);
            hbmMem = CreateCompatibleBitmap(hdc, width, height);
            hOldBitmap = (HBITMAP)SelectObject(hdcMem, hbmMem);
        }

        HDC MemDC()
        {
            return hdcMem;
        }

        RECT ClientRect()
        {
            return rect;
        }

        ~DoubleBuferedPaint()
        {
            // === DOUBLE BUFFERING - COPY TO SCREEN ===
            BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

            // === CLEANUP DOUBLE BUFFERING ===
            SelectObject(hdcMem, hOldBitmap);
            SelectClipRgn(hdc, NULL);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
        }
    };
}