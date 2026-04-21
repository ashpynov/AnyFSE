
#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace AnyFSE::Tools::Steam
{
    std::wstring GetConfigValue(const std::wstring &key, const std::wstring &defValue);
    std::vector<WORD> ParseKeySequence(const std::string &steamSequence);
    std::vector<WORD> GetOverlaySequence();
};
