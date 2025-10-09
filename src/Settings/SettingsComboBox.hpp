#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <functional>
#include "SettingsLine.hpp"
#include "Tools/Event.hpp"

namespace AnyFSE::Settings::Controls
{
    class SettingsComboBox:private SettingsLine
    {
        struct ComboItem
        {
            std::wstring name;
            std::wstring icon;
            std::wstring value;
            int iconIndex;
        };

    private:
        HWND hCombo;
        HIMAGELIST g_hImageList = NULL;
        std::list<ComboItem> comboItems;
        virtual HWND CreateControl(HWND hWnd);
        virtual bool ApplyTheme(bool isDark);


        static LRESULT CALLBACK ComboBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                              UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        BOOL OnMeasureItem(MEASUREITEMSTRUCT *mis);

        BOOL OnDrawItem(DRAWITEMSTRUCT *dis);

        void DrawComboBox(DRAWITEMSTRUCT *dis);

    public:
        SettingsComboBox(HWND hParent,
            const std::wstring &name,
            const std::wstring &description,
            int x, int y, int width, int height);

        ~SettingsComboBox();

        int AddItem(const std::wstring &name, const std::wstring &icon, const std::wstring &value, int pos = -1);
        int Reset();
        void SelectItem(int index);
        std::wstring GetCurentValue();

        Event OnChanged;

    };
}