// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include <windows.h>

namespace AnyFSE::App::AppSettings
{
    class AppSettings
    {
        private:
            static BOOL RequestAdminElevation(bool configure = false);

        public:
            static void InitCustomControls();
            static BOOL IsRunningAsAdministrator(bool elevate = false, bool configure = false);
            static int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
    };
}