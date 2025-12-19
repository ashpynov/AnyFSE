#pragma once
#include <string>

namespace AnyFSE::Tools::Paths
{
    std::wstring GetExePath();
    std::wstring GetExeFileName();
    std::wstring GetDataPath();
}