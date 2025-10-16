#include <windows.h>

namespace AnyFSE
{
    class Application
    {
        private:
            static BOOL RequestAdminElevation();


        public:
            static BOOL ShouldRunAsApplication(LPSTR lpCmdLine);
            static void InitCustomControls();
            static BOOL IsRunningAsAdministrator(bool elevate = false);
            static int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
    };
}