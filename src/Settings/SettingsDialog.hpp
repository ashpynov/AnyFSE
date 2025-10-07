// ModernSettingsDialog.h
#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

#include "Configuration/Config.hpp"

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


namespace AnyFSE::Settings
{
    class SettingsDialog
    {
        struct ComboItem
        {
            std::wstring name;
            std::wstring iconPath;
            std::wstring Path;
            int iconIndex;
        };

        struct ControlsGroup
        {
            bool visible;
            int height;
            std::vector<std::pair<int, int>> items;
        };

        HIMAGELIST g_hImageList = NULL;
        std::list<ComboItem> comboItems;

        std::vector<ControlsGroup> groups;

        int designHeight;
        void InitGroups();
        void ShowGroup(int groupIdx, bool show);
        void MoveGroupItem(int id, int original, int offset);

    public:
        INT_PTR Show(HINSTANCE hInstance);
        void CenterDialog(HWND hwnd);

    private:
        static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        INT_PTR InstanceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        void UpdateCombo();
        void UpdateCustomSettings();
        void SaveSettings();

        void AddComboItem(const wstring &name, const wstring &path, int pos =-1);

        void OnInitDialog(HWND hwnd);
        void OnBrowseLauncher(HWND hwnd, int editId);
        void OnLauncherChanged(HWND hwnd);
        void OnOk(HWND hwnd);
        void OnCustomChanged(HWND hwnd);
        void UpdateCustom();

        HINSTANCE m_hInstance;
        HWND m_hDialog;

        bool m_isCustom;
        LauncherConfig config;
        std::wstring current;
        std::list<std::wstring> launchers;


    };
}

using namespace AnyFSE::Settings;