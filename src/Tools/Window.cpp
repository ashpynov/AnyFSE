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


#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include "Logging/LogManager.hpp"
#include "Tools/Window.hpp"
#include <algorithm>
#include <shlobj_core.h>
#include "Packages.hpp"


namespace AnyFSE::Tools::Window
{
    static Logger log = LogManager::GetLogger("Tools/Window");

    BOOL GetChildRect(HWND hwnd, RECT * rect)
    {
        GetWindowRect(hwnd, rect);
        MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)rect, 2);
        return TRUE;
    }

    BOOL MoveWindow(HWND hwnd, int dx, int dy, BOOL bRepaint)
    {
        RECT rect;
        GetChildRect(hwnd, &rect);
        OffsetRect(&rect, dx, dy);
        ::MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bRepaint);
        return TRUE;
    }

    BOOL MoveWindow(HWND hwnd, RECT* rect, BOOL bRepaint)
    {
        ::MoveWindow(hwnd, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, bRepaint);
        return TRUE;
    }

    BOOL MouseInClientRect(HWND hWnd, RECT *rect)
    {
        RECT hitRect;
        if (rect == NULL)
        {
            GetClientRect(hWnd, &hitRect);
        }
        else
        {
            hitRect = *rect;
        }

        POINT pt;
        GetCursorPos(&pt);
        MapWindowPoints(NULL, hWnd, &pt, 1);

        return PtInRect(&hitRect, pt);
    }
}
