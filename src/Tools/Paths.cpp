#pragma once

#include <string>
#include <windows.h>
#include <filesystem>
#include "Paths.hpp"

#include <winrt/Windows.Storage.h>
#pragma comment(lib, "windowsapp.lib")      // WinRT runtime

namespace AnyFSE::Tools::Paths
{
    using namespace winrt;
    using namespace Windows::Storage;

    std::wstring GetAppLocalPath()
    {
        static std::wstring path = L"";
        if (path.empty())
        {
            init_apartment();

            try
            {
                path = ApplicationData::Current().LocalFolder().Path();
            }
            catch(const winrt::hresult_error&)
            {
                path = GetAppPath() + L"\\LocalState";
            }
        }
        return path;
    }

    std::wstring GetAppLocalCachePath()
    {
        static std::wstring path = L"";
        if (path.empty())
        {
            init_apartment();
            try
            {
                path = ApplicationData::Current().LocalCacheFolder().Path();
            }
            catch(const winrt::hresult_error&)
            {
                path = GetAppPath() + L"\\LocalCache";
            }
        }
        return path;
    }

    std::wstring GetAppTempPath()
    {
        static std::wstring path = L"";
        if (path.empty())
        {
            init_apartment();
            try
            {
                path = ApplicationData::Current().TemporaryFolder().Path();
            }
            catch(const winrt::hresult_error&)
            {
                path = GetAppPath() + L"\\TempState";
            }
        }
        return path;
    }

    std::wstring GetConfigPath()
    {
        return Tools::Paths::GetAppLocalPath();
    }

    std::wstring GetLogsPath()
    {
        return Tools::Paths::GetAppLocalCachePath() + L"\\logs";
    }

    std::wstring GetTempPath()
    {
        return Tools::Paths::GetAppTempPath() + L"\\tmp";
    }

    std::wstring GetDumpsPath()
    {
        return Tools::Paths::GetAppTempPath() + L"\\dumps";
    }

    std::wstring GetSplashDefaultPath()
    {
        return Tools::Paths::GetDataPath() + L"\\splash";
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
        //Local\Packages\ArtemShpynov.AnyFSE_by4wjhxmygwn4\LocalState
        return appData;
    }

    std::wstring GetAppPath()
    {
        wchar_t appData[MAX_PATH]={0};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\Packages\\ArtemShpynov.AnyFSE_by4wjhxmygwn4", appData, MAX_PATH);
        return appData;
    }
}