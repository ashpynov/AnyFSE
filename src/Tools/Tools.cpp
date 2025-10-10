#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include "Logging/LogManager.hpp"
#include "Tools.hpp"
#include <algorithm>

namespace AnyFSE::Tools
{
    static Logger log = LogManager::GetLogger("Tools");

    HICON LoadIcon(const std::wstring &icon)
    {
        if ( icon[0] == L'#')
        {
            return 0;
        }

        int index = 0;
        std::filesystem::path path(icon);

        size_t commaPos = icon.find(L',');
        if (commaPos != std::wstring::npos)
        {
            path = icon.substr(0, commaPos);
            std::wstring indexStr = icon.substr(commaPos + 1);
            try
            {
                index = std::stoi(indexStr);
            }
            catch (const std::exception& ex)
            {
                log.Warn(ex, "Index is invalid: %ls", indexStr);
            }
        }

        if (!std::filesystem::exists(path))
        {
            log.Warn("Icon file not found %s", path.string().c_str());
            return NULL;
        }

        HICON hIcon = ExtractIcon(GetModuleHandle(NULL), path.wstring().c_str(), index);

        if (!hIcon)
        {
            log.Warn(log.APIError(), "No icon with index %d", index);
            return NULL;
        }

        return hIcon;
    }
    std::string to_string(const std::wstring& wstr)
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";

        std::string result;
        result.resize(len - 1); // Exact size needed
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
        return result;
    }

    std::wstring to_wstring(const std::string& str)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";

        std::wstring result;
        result.resize(len - 1); // Exact size needed
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
        return result;
    }
    std::wstring to_lower(const std::wstring &str)
    {
        std::wstring wstr = str;
        std::transform(wstr.begin(), wstr.end(), wstr.begin(), ::towlower);
        return wstr;
    }


}
