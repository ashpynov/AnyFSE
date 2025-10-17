#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include "Logging/LogManager.hpp"
#include "Unicode.hpp"
#include <algorithm>

namespace AnyFSE::Tools::Unicode
{
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
