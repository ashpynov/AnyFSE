#pragma once
#include <Windows.h>
#include <map>
#include <functional>

namespace Ally
{
    enum EventCode
    {
        Release = 0,
        ACPress = 56,
        ToggleMicrophone = 124,
        GyroButtonDown = 144,
        GyroButtonUp = 145,
        LibraryPress = 147, // Xbox Ally
        ScreenShot = 160,
        ShowKeyboard = 162,
        ToggleRecord = 164,
        ModePress = 165,
        CCPress = 166, // It is AC (left on XBox Ally versions)
        ACHold = 167,
        ROGHoldRelease = 168,
        Unknown = 236,
        MACPress = ACPress + ModePress,
        MCCPress = CCPress + ModePress,
        MACHold = ACHold + ModePress,
        MLibraryPress = LibraryPress + ModePress
    };

    enum RogAllyVersion
    {
        NotDefined,
        NotSupported,
        ROGAlly,
        ROGAllyX,
        XboxROGAlly,
        XboxROGAllyX
    };

    bool IsSupported();

    HANDLE FindHIDDevice();
    bool GetRawInputDevice(HANDLE hDevice, HWND hWnd, RAWINPUTDEVICE *pRid);

    inline std::map<Ally::EventCode,  std::function<void()>> ButtonBind;
    void Load();
    DWORD WINAPI HIDListener(LPVOID lpParam);

    void EnableNativeHandler(bool bEnable);
    bool IsNativeHandlerEnabled();
    bool UpdateHidListener();
    bool CheckListener();
    bool SetupListener();
    bool WaitListener();

    RogAllyVersion GetRogAllyVersion();
    bool IsXBoxRogAlly();
}