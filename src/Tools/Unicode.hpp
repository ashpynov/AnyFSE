#pragma once
#include <string>
#include <Windows.h>

namespace AnyFSE::Tools::Unicode
{
    std::wstring to_wstring(const std::string& str);
    std::string to_string(const std::wstring& wstr);
    std::wstring to_lower(const std::wstring &str);
}

namespace Unicode = AnyFSE::Tools::Unicode;