#pragma once
#include <string>

namespace AnyFSE::Tools::Paths
{
    std::wstring GetConfigPath();
    std::wstring GetLogsPath();
    std::wstring GetTempPath();
    std::wstring GetDumpsPath();
    std::wstring GetSplashDefaultPath();
    std::wstring GetExeFileName();

    std::wstring GetAppPath();
    std::wstring GetDataPath();
}