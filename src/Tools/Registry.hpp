#include <windows.h>
#include <string>

namespace AnyFSE::Tools
{
    class Registry
    {
    public:
        static std::wstring ReadString(const std::wstring &subKey, const std::wstring &valueName, const std::wstring &defaultValue = L"");
        static DWORD ReadDWORD(const std::wstring &subKey, const std::wstring &valueName, DWORD defaultValue = 0);
        static bool ReadBool(const std::wstring &subKey, const std::wstring &valueName, bool defaultValue = false);
    };
}
using namespace AnyFSE::Tools;