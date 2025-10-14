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

        static std::wstring Registry::GetInstallPath(const std::wstring &displayName);

        static bool WriteString(const std::wstring &subKey, const std::wstring &valueName, const std::wstring &value);
        static bool WriteDWORD(const std::wstring &subKey, const std::wstring &valueName, DWORD value);
        static bool WriteBool(const std::wstring &subKey, const std::wstring &valueName, bool value);
        static bool WriteBinary(const std::wstring &subKey, const std::wstring &valueName, const BYTE* data, DWORD size);

        // Utility methods
        static bool DeleteValue(const std::wstring &subKey, const std::wstring &valueName);
        static bool DeleteKey(const std::wstring &subKey);

    private:
        static HKEY GetRootKey(const std::wstring& subKey, std::wstring& actualPath);
    };
}
using namespace AnyFSE::Tools;