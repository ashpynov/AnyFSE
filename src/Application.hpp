#include <windows.h>

namespace AnyFSE
{
    class Application
    {
        private:
            static BOOL RequestAdminElevation(bool configure = false);


        public:
            static BOOL ShouldRunAsApplication(LPSTR lpCmdLine);
            static BOOL ShouldRunAsConfiguration(LPSTR lpCmdLine);
            static void InitCustomControls();
            static BOOL IsRunningAsAdministrator(bool elevate = false, bool configure = false);
            static int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
    };
}