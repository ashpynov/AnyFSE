#pragma once

#include <list>
#include <windows.h>
#include "FluentDesign/SettingsLine.hpp"

namespace AnyFSE::App::AppSettings::Settings
{
    class SettingsDialog;

    namespace Page
    {
        class SettingsPage
        {
            public:
                virtual ~SettingsPage() = default;

                virtual std::list<FluentDesign::SettingsLine> &GetSettingsLines() = 0;

                virtual void AddPage(std::list<FluentDesign::SettingsLine>& settingPageList, ULONG &top) = 0;
                virtual void LoadControls() = 0;
                virtual void SaveControls() = 0;
        };
    }

};
