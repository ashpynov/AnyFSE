#include <windows.h>
#include <string>


namespace AnyFSE::Tools::Admin
{
    BOOL IsRunningAsAdministrator();
    BOOL RequestAdminElevation(const std::wstring& args = L"");
}