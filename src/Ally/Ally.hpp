#pragma once
#include <Windows.h>
#include <map>
#include <functional>

namespace Ally
{
    enum EventCode
    {
        Release = 0,
        ROGShortPress = 56,
        ToggleMicrophone = 124,
        ScreenShot = 160,
        ShowKeyboard = 162,
        ToggleRecord = 164,
        ModePress = 165,
        CommandCenter = 166,
        ROGHold = 167,
        ROGHoldRelease = 168,
        Unknown = 236
    };

    HANDLE FindHIDDevice();
    bool GetRawInputDevice(HANDLE hDevice, HWND hWnd, RAWINPUTDEVICE *pRid);

    inline std::map<Ally::EventCode,  std::function<void()>> ButtonBind;
    void Load();
    DWORD WINAPI HIDListener(LPVOID lpParam);

    bool CheckListener();
    bool SetupListener();
    bool WaitListener();
}