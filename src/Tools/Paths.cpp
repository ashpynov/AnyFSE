#pragma once

#include <string>
#include <windows.h>
#include <filesystem>
#include "Paths.hpp"
#include "Packages.hpp"

#include <winrt/Windows.Storage.h>
#pragma comment(lib, "windowsapp.lib")      // WinRT runtime
#include "App/Constants.hpp"

namespace AnyFSE::Tools::Paths
{
    namespace storage = winrt::Windows::Storage;
    namespace c = AnyFSE::App::Constants;

    std::wstring GetAppLocalPath()
    {
        static std::wstring path = L"";
        if (path.empty())
        {
            winrt::init_apartment();
            path = (std::filesystem::path(GetAppPath()) / c::AppLocalStateDirectory).wstring();
            if (Packages::HasPackageIdentity())
            {
                try
                {
                    path = storage::ApplicationData::Current().LocalFolder().Path();
                }
                catch (const winrt::hresult_error&)
                {
                    // Keep the fallback path if the package data store is unavailable.
                }
            }
        }
        return path;
    }

    std::wstring GetAppLocalCachePath()
    {
        static std::wstring path = L"";
        if (path.empty())
        {
            winrt::init_apartment();
            path = (std::filesystem::path(GetAppPath()) / c::AppLocalCacheDirectory).wstring();
            if (Packages::HasPackageIdentity())
            {
                try
                {
                    path = storage::ApplicationData::Current().LocalCacheFolder().Path();
                }
                catch (const winrt::hresult_error&)
                {
                    // Keep the fallback path if the package data store is unavailable.
                }
            }
        }
        return path;
    }

    std::wstring GetAppTempPath()
    {
        static std::wstring path = L"";
        if (path.empty())
        {
            winrt::init_apartment();
            path = (std::filesystem::path(GetAppPath()) / c::AppTempStateDirectory).wstring();
            if (Packages::HasPackageIdentity())
            {
                try
                {
                    path = storage::ApplicationData::Current().TemporaryFolder().Path();
                }
                catch (const winrt::hresult_error&)
                {
                    // Keep the fallback path if the package data store is unavailable.
                }
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
        return Tools::Paths::GetAppTempPath();
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

    std::wstring GetExePath()
    {
        static std::wstring _dir;
        if (_dir.empty())
        {
            _dir = std::filesystem::path(GetExeFileName()).parent_path().wstring();
        }
        return _dir;
    }

    std::wstring GetDataPath()
    {
        wchar_t appData[MAX_PATH]={0};
        ExpandEnvironmentStringsW(L"%PROGRAMDATA%\\AnyFSE", appData, MAX_PATH);
        //Local\Packages\<PackageFamilyName>\LocalState
        return appData;
    }

    std::wstring GetInstallPath()
    {
        wchar_t appData[MAX_PATH]={0};
        ExpandEnvironmentStringsW(L"%PROGRAMFILES%\\AnyFSE", appData, MAX_PATH);
        return appData;
    }

    std::wstring GetAppPath()
    {
        wchar_t appData[MAX_PATH]={0};
        ExpandEnvironmentStringsW((std::wstring(L"%LOCALAPPDATA%\\Packages\\") + App::Constants::PackageFamilyName).c_str(), appData, MAX_PATH);
        return appData;
    }
}
