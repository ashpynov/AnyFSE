// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>

#pragma once

#include <windows.h>

namespace AnyFSE::App::AppControl
{
    class AppControl
    {
        private:
            static BOOL RequestAdminElevation(bool configure = false);
        public:
            static int ShowSettings();
            static BOOL ShouldRunAsApplication(LPSTR lpCmdLine);
            static BOOL ShouldRunAsConfiguration(LPSTR lpCmdLine);
            static void InitCustomControls();
            static BOOL IsRunningAsAdministrator(bool elevate = false, bool configure = false);
            static bool AsControl(LPSTR lpCmdLine);
            static bool IsServiceAvailable();
            static bool AsSettings(LPSTR lpCmdLine);
            static bool NeedAdmin(LPSTR lpCmdLine);
            static int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
    };
}