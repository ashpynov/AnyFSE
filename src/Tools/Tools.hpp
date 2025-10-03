#pragma once
#include <string>
#include <Windows.h>

namespace AnyFSE::Tools
{
    HICON LoadIcon(const std::wstring& icon);
    std::wstring to_wstring(const std::string& str);
    std::string to_string(const std::wstring& wstr);
}

namespace Tools = AnyFSE::Tools;