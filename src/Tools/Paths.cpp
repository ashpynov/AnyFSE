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

    std::wstring GetDataPath()
    {
        wchar_t appData[MAX_PATH]={0};
        ExpandEnvironmentStringsW(L"%PROGRAMDATA%\\AnyFSE", appData, MAX_PATH);
        return (std::filesystem::exists(std::filesystem::path(std::wstring(appData) + L"\\AnyFSE.json")))
            ? appData
            : GetExePath();
    }
}