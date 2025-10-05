// ModernSettingsDialog.h
#pragma once

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
// SettingsDialog.h
#pragma once
#include <windows.h>

namespace AnyFSE::Settings
{
    class SettingsDialog
    {
    public:
        INT_PTR Show(HINSTANCE hInstance);
        void CenterDialog(HWND hwnd);

    private:
        static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        INT_PTR InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        void OnInitDialog(HWND hwnd);
        void OnBrowseLauncher(HWND hwnd, int editId);
        void OnOk(HWND hwnd);

        HINSTANCE m_hInstance;
        int m_dialogId;
        HWND m_hDialog;
    };
}

using namespace AnyFSE::Settings;