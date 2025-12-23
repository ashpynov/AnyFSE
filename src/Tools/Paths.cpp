#pragma once

#include <string>
#include <windows.h>
#include <filesystem>
#include "Paths.hpp"

namespace AnyFSE::Tools::Paths
{
    std::wstring GetExePath()
    {
        static std::wstring _path;
        if (_path.empty())
        {
            wchar_t path[MAX_PATH];
            ::GetModuleFileNameW(NULL, path, MAX_PATH);
            std::filesystem::path binary = path;
            _path = binary.parent_path().wstring();
        }
        return _path;
    }

    std::wstring GetExeFileName()
    {
        static std::wstring _path;
        if (_path.empty())
        {
            wchar_t path[MAX_PATH];
            ::GetModuleFileNameW(NULL, path, MAX_PATH);
            _path = path;
        }
        return _path;
    }

    std::wstring GetProgramDataPath()
    {
        wchar_t appData[MAX_PATH]={0};
        ExpandEnvironmentStringsW(L"%PROGRAMDATA%\\AnyFSE", appData, MAX_PATH);
        return appData;
    }

    std::wstring GetDataPath()
    {
        std::wstring appData = GetProgramDataPath();
        return (std::filesystem::exists(std::filesystem::path(appData + L"\\AnyFSE.json")))
            ? appData
            : GetExePath();
    }
}