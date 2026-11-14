#pragma once
#include <Windows.h>
#include <map>
#include <functional>
#include <string>

namespace Ally
{
    class Controller;

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
        MLibraryPress = LibraryPress + ModePress,
        Double = 400,
        DoubleACPress = ACPress + Double,
        DoubleCCPress = CCPress + Double,
        DoubleLibraryPress = LibraryPress + Double,
        DoubleModePress = ModePress + Double
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

    HANDLE FindHIDDevice(std::wstring* path = nullptr);
    bool GetRawInputDevice(HANDLE hDevice, HWND hWnd, RAWINPUTDEVICE *pRid);

    inline std::map<Ally::EventCode,  std::function<void()>> ButtonBind;
    void Load();
    DWORD WINAPI HIDListener(LPVOID lpParam);

    void EnableACSEInjector(bool bEnable);
    bool IsNativeHandlerEnabled();
    bool IsInjectorEnabled();
    bool UpdateHidListener();

    bool EnsureListener();
    bool CheckListener();
    bool SetupListener();
    bool WaitListener();

    RogAllyVersion GetRogAllyVersion();
    bool IsXBoxRogAlly();

    void OnInput(bool hidEnabled, HANDLE hDevice, HRAWINPUT rawInput, bool* pbModePressed, Controller& controller,
                 HWND hwnd, EventCode& pressedKey);
    void OnHotkey(int id);

}
