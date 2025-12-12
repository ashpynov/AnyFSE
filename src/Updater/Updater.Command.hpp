#pragma once

#include <string>
#include <windows.h>
#include "Updater/Updater.hpp"

namespace AnyFSE::Updater
{
    enum COMMAND
    {
        SUBSCRIBE = 0,
        CHECK,
        UPDATE,
        GETUPDATEINFO,
        NOTIFYCONFIGUPDATED
    };

    struct SUBSCRIPTION
    {
        HWND hWnd;
        UINT uMsg;
    };

    struct UPDATECHECK
    {
        BOOL bIncludePreRelease;
        HWND hWnd;
        UINT uMsg;
    };

    struct UPDATEUPDATE
    {
        std::wstring newVersion;
        BOOL bSilent;
        HWND hWnd;
        UINT uMsg;
    };

    struct UPDATEINFO
    {
        UpdaterState uiState;
        wchar_t newVersion[MAX_PATH];
        wchar_t downloadPath[MAX_PATH];
        long long lCommandAge;
        UpdaterState uiCommand;

        void put(UpdateInfo i)
        {
            uiState = i.uiState;
            lCommandAge = i.lCommandAge;
            uiCommand = i.uiCommand;
            wcsncpy_s(newVersion, i.newVersion.c_str(), MAX_PATH - 1);
            wcsncpy_s(downloadPath, i.downloadPath.c_str(), MAX_PATH - 1);
        }
    };

    struct CONFIGUPDATED
    {
        int iCheckInterval;
        bool bPreRelease;
        bool bNotifications;
        bool bOnClick;
    };
}
