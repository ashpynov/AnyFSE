#pragma once
#include <windows.h>

namespace AnyFSE::App
{
    typedef INT_PTR (*MainFunc)(HINSTANCE, HINSTANCE, LPSTR, int);
    int CallLibrary(const WCHAR * library, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
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