#include <windows.h>
#include "Registry.hpp"

namespace AnyFSE::Tools
{
    // Read string from registry
    // static
    std::wstring Registry::ReadString(const std::wstring &subKey, const std::wstring &valueName, const std::wstring &defaultValue = L"")
    {
        HKEY hOpenedKey;
        std::wstring result = defaultValue;

        if (RegOpenKeyEx(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &hOpenedKey) == ERROR_SUCCESS)
        {
            DWORD size = 0;
            // Get required buffer size
            if (RegQueryValueEx(hOpenedKey, valueName.c_str(), NULL, NULL, NULL, &size) == ERROR_SUCCESS)
            {
                wchar_t *buffer = new wchar_t[size / sizeof(wchar_t) + 1];
                if (RegQueryValueEx(hOpenedKey, valueName.c_str(), NULL, NULL, (LPBYTE)buffer, &size) == ERROR_SUCCESS)
                {
                    result = buffer;
                }
                delete[] buffer;
            }
            RegCloseKey(hOpenedKey);
        }
        return result;
    }

    // Read DWORD from registry
    //static 
    DWORD Registry::ReadDWORD(const std::wstring &subKey, const std::wstring &valueName, DWORD defaultValue = 0)
    {
        HKEY hOpenedKey;
        DWORD result = defaultValue;
        DWORD size = sizeof(DWORD);

        if (RegOpenKeyEx(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &hOpenedKey) == ERROR_SUCCESS)
        {
            RegQueryValueEx(hOpenedKey, valueName.c_str(), NULL, NULL, (LPBYTE)&result, &size);
            RegCloseKey(hOpenedKey);
        }
        return result;
    }

    // Read boolean from registry
    //static 
    bool Registry::ReadBool(const std::wstring &subKey, const std::wstring &valueName, bool defaultValue = false)
    {
        return ReadDWORD(subKey, valueName, defaultValue ? 1 : 0) != 0;
    }
}
