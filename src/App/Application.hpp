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

#pragma once
#include <windows.h>

namespace AnyFSE::App
{
    struct UserMessage
    {
        static UINT WM_TRAY;
        static UINT WM_TASKBARCREATED;
        static UINT WM_SHOWSETTINGS;
        static UINT WM_RESTARTTASK;
    };

    static const wchar_t * AnyFSESplashWndClassName = L"AnyFSE";
    static const wchar_t * AnyFSEAdminWndClassName = L"AnyFSEAdmin";

    typedef INT_PTR (*MainFunc)(HINSTANCE, HINSTANCE, LPSTR, int);

    static int CallLibrary(const WCHAR * library, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
        HMODULE hModuleDll = NULL;
        static MainFunc *Main = nullptr;

        int result = INT_MIN;
        do
        {
            hModuleDll = LoadLibrary(library);
            if (!hModuleDll)
            {
                break;
            }

            MainFunc Main = (MainFunc)GetProcAddress(hModuleDll, "Main");
            if( !Main )
            {
                break;
            }
            result = (int)Main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

        } while (false);

        if (result == INT_MIN)
        {
            LPSTR messageBuffer = nullptr;
            DWORD size = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&messageBuffer,
                0,
                NULL);
            MessageBoxA(NULL, messageBuffer, "Call module error", MB_OK | MB_ICONERROR);
            LocalFree(messageBuffer);
        }

        if (hModuleDll)
            FreeLibrary(hModuleDll);

        return result;
    }
}