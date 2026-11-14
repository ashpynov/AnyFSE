#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <windows.h>
#include <shellapi.h>
#include "Ally/Ally.hpp"
#include "Ally/Handlers.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Process.hpp"
#include "Tools/Paths.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/PowerEfficiency.hpp"
#include "App/Constants.hpp"
#include "Ally.hpp"
#include "Ally/Services.hpp"
#include "App/GamingExperience.hpp"
#include "Ally/Controller.hpp"
#include "Ally/Profile.hpp"

using namespace AnyFSE::App;

namespace Ally
{
    enum ControlModeHotkeyId
    {
        HotkeyProfileAuto = 0x100,
        HotkeyProfileGamepad,
        HotkeyProfileDesktop,
        HotkeyProfileToggle,
        HotkeyProfileNext
    };

    struct ControlModeHotkey
    {
        int id;
        UINT modifiers;
        UINT key;
        void (*handler)();
        const char* combination;
    };

    static Logger log = LogManager::GetLogger("HIDListener");
    static const wchar_t *HidListenerClass = L"HIDListener";
    static constexpr int HotkeyEnterFSEWithReboot = (int)GamingExperience::ConfirmationMode::Reboot;
    static constexpr int HotkeyEnterFSENow = (int)GamingExperience::ConfirmationMode::Now;
    static constexpr UINT ControlModeModifiers = MOD_WIN | MOD_ALT | MOD_CONTROL | MOD_NOREPEAT;
    static const ControlModeHotkey controlModeHotkeys[] = {
        {HotkeyProfileAuto, ControlModeModifiers, VK_F8, Profile::SetAuto, "Win+Alt+Ctrl+F8"},
        {HotkeyProfileGamepad, ControlModeModifiers, VK_F9, Profile::SetGamepad, "Win+Alt+Ctrl+F9"},
        {HotkeyProfileDesktop, ControlModeModifiers | MOD_SHIFT, VK_F9, Profile::SetDesktop, "Win+Alt+Ctrl+Shift+F9"},
        {HotkeyProfileToggle, ControlModeModifiers, VK_F10, Profile::SetToggle, "Win+Alt+Ctrl+F10"},
        {HotkeyProfileNext, ControlModeModifiers | MOD_SHIFT, VK_F10, Profile::SetNext, "Win+Alt+Ctrl+Shift+F10"}
    };
    static HANDLE hHidThread = nullptr;

    static bool UpdateHotkeys(HWND hWnd)
    {
        UnregisterHotKey(hWnd, HotkeyEnterFSEWithReboot);
        UnregisterHotKey(hWnd, HotkeyEnterFSENow);
        for (const ControlModeHotkey& hotkey : controlModeHotkeys)
            UnregisterHotKey(hWnd, hotkey.id);

        bool profileRegistered = false;
        if (Config::AllyControlModeHotkeys && Ally::IsSupported())
        {
            for (const ControlModeHotkey& hotkey : controlModeHotkeys)
            {
                if (RegisterHotKey(hWnd, hotkey.id, hotkey.modifiers, hotkey.key))
                    profileRegistered = true;
                else
                    log.Warn(log.APIError(), "Could not register %s", hotkey.combination);
            }
        }

        if (!Config::HotkeysEnable)
        {
            return profileRegistered;
        }

        bool rebootRegistered = RegisterHotKey(
            hWnd,
            HotkeyEnterFSEWithReboot,
            MOD_WIN | MOD_SHIFT | MOD_NOREPEAT,
            VK_F11) != FALSE;
        if (!rebootRegistered)
        {
            log.Warn(log.APIError(), "Could not register Win+Shift+F11");
        }

        bool nowRegistered = RegisterHotKey(
            hWnd,
            HotkeyEnterFSENow,
            MOD_WIN | MOD_CONTROL | MOD_NOREPEAT,
            VK_F11) != FALSE;
        if (!nowRegistered)
        {
            log.Warn(log.APIError(), "Could not register Win+Ctrl+F11");
        }

        return rebootRegistered || nowRegistered || profileRegistered;
    }

    bool IsSupported()
    {
        static int supported = -1;
        if (supported == -1)
        {
            supported = FindHIDDevice() ? 1 : 0;

            log.Trace("Ally HID is %ssupported", supported ? "" : "not ");
        }
        return supported;
    }

    RogAllyVersion GetRogAllyVersion()
    {
        static RogAllyVersion version = RogAllyVersion::NotDefined;

        if (version == RogAllyVersion::NotDefined)
        {
            std::wstring product = Registry::ReadString(
                L"HKLM\\SYSTEM\\HardwareConfig\\Current",
                L"BaseBoardProduct",
                L"");

            log.Debug("Detected ASUS device: '%s'", Unicode::to_string(product).c_str());

            version =   (product == L"RC73XA") ? RogAllyVersion::XboxROGAllyX
                      : (product == L"RC73YA") ? RogAllyVersion::XboxROGAlly
                      : (product == L"RC72LA") ? RogAllyVersion::ROGAllyX
                      : (product == L"RC71L")  ? RogAllyVersion::ROGAlly
                                               : RogAllyVersion::NotSupported;
        }
        return version;
    }

    bool IsXBoxRogAlly()
    {
        return GetRogAllyVersion() == RogAllyVersion::XboxROGAllyX
            || GetRogAllyVersion() == RogAllyVersion::XboxROGAlly;
    }

    HANDLE FindHIDDevice(std::wstring* path)
    {
        if (path)
            path->clear();
        UINT numDevices = 0;
        // Call once with NULL to get the required array size
        GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST));

        std::vector<RAWINPUTDEVICELIST> deviceList(numDevices);
        GetRawInputDeviceList(deviceList.data(), &numDevices, sizeof(RAWINPUTDEVICELIST));

        USHORT usagePage = 0;
        USHORT usage = 0;
        HANDLE hDevice = NULL;

        for (UINT i = 0; i < numDevices; i++)
        {
            if (deviceList[i].dwType == RIM_TYPEHID)
            {
                UINT pathSize = 0;
                GetRawInputDeviceInfo(deviceList[i].hDevice, RIDI_DEVICENAME, NULL, &pathSize);

                std::wstring devicePath(pathSize, 0);
                GetRawInputDeviceInfo(deviceList[i].hDevice, RIDI_DEVICENAME, devicePath.data(), &pathSize);

                std::transform(devicePath.begin(), devicePath.end(), devicePath.begin(), ::toupper);

                if (devicePath.find(L"VID_0B05") != std::string::npos)
                {
                    log.Debug("Detected ASUS HID device at: '%s'", Unicode::to_string(devicePath).c_str());
                }

                if (devicePath.find(L"VID_0B05") != std::string::npos
                    && ( devicePath.find(L"PID_1ABE") != std::string::npos
                        || devicePath.find(L"PID_1B4C") != std::string::npos )
                    && devicePath.find(L"MI_02") != std::string::npos
                    && devicePath.find(L"COL01") != std::string::npos)
                {
                    log.Debug("Selected ASUS HID device: '%s'", Unicode::to_string(devicePath).c_str());
                    if (path)
                        *path = devicePath.c_str();
                    return deviceList[i].hDevice;
                }
            }
        }
        return NULL;
    }

    bool GetRawInputDevice(HANDLE hDevice, HWND hWnd, RAWINPUTDEVICE *pRid)
    {
        RID_DEVICE_INFO deviceInfo;
        deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
        UINT infoSize = sizeof(RID_DEVICE_INFO);

        if (GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &deviceInfo, &infoSize) > 0)
        {
            pRid->dwFlags = RIDEV_INPUTSINK;
            pRid->hwndTarget = hWnd;
            pRid->usUsagePage = deviceInfo.hid.usUsagePage;
            pRid->usUsage = deviceInfo.hid.usUsage;
            return true;
        }
        return false;
    }

    void Load()
    {
        Config::Load();
        ButtonBind[Ally::EventCode::ACPress] =          IsXBoxRogAlly() ? Handlers::GetByName(Config::AllyHidLibraryPress) : Handlers::GetByName(Config::AllyHidACPress);
        ButtonBind[Ally::EventCode::ACHold] =           Handlers::GetByName(Config::AllyHidACHold);
        ButtonBind[Ally::EventCode::CCPress] =          IsXBoxRogAlly() ? Handlers::GetByName(Config::AllyHidACPress) : Handlers::GetByName(Config::AllyHidCCPress);
        ButtonBind[Ally::EventCode::LibraryPress] =     Handlers::GetByName(Config::AllyHidLibraryPress);

        ButtonBind[Ally::EventCode::MACPress] =         IsXBoxRogAlly() ? Handlers::GetByName(Config::AllyHidModeLibraryPress) : Handlers::GetByName(Config::AllyHidModeACPress);
        ButtonBind[Ally::EventCode::MACHold] =          Handlers::GetByName(Config::AllyHidModeACHold);
        ButtonBind[Ally::EventCode::MCCPress] =         IsXBoxRogAlly() ? Handlers::GetByName(Config::AllyHidModeACPress) : Handlers::GetByName(Config::AllyHidModeCCPress);
        ButtonBind[Ally::EventCode::MLibraryPress] =    Handlers::GetByName(Config::AllyHidModeLibraryPress);

        ButtonBind[Ally::EventCode::ROGHoldRelease] =   NULL;
        ButtonBind[Ally::EventCode::ModePress] =        NULL;
        ButtonBind[Ally::EventCode::Release] =          NULL;
        ButtonBind[Ally::EventCode::Unknown] =          NULL;

        if (Config::AllyHidExtraCommandsEnable)
        {
            ButtonBind[Ally::EventCode::ToggleMicrophone] = Handlers::ToggleMicrophone;
            ButtonBind[Ally::EventCode::ScreenShot] =       Handlers::TakeScreenShoot;
            ButtonBind[Ally::EventCode::ShowKeyboard] =     Handlers::ShowKeyboard;
            ButtonBind[Ally::EventCode::ToggleRecord] =     Handlers::ToggleRecord;
        }

        log.Trace("Bind Mode is %s", IsXBoxRogAlly() ? "XBoxRoxAlly" : "RogAlly");

        log.Trace("Bind ACPress to %s", IsXBoxRogAlly() ? Config::AllyHidLibraryPress : Config::AllyHidACPress );
        log.Trace("Bind ACHold to %s", Config::AllyHidACHold);
        log.Trace("Bind CCPress to %s", IsXBoxRogAlly() ? Config::AllyHidACPress : Config::AllyHidCCPress );
        log.Trace("Bind LibraryPress to %s", Config::AllyHidLibraryPress );

        log.Trace("Bind Mode + ACPress to %s", IsXBoxRogAlly() ? Config::AllyHidModeLibraryPress : Config::AllyHidModeACPress );
        log.Trace("Bind Mode + ACHold to %s", Config::AllyHidModeACHold);
        log.Trace("Bind Mode + CCPress to %s", IsXBoxRogAlly() ? Config::AllyHidModeACPress : Config::AllyHidModeCCPress );
        log.Trace("Bind Mode + LibraryPress to %s", Config::AllyHidModeLibraryPress );
    }

    void OnInput(bool hidEnabled, HANDLE hDevice, HRAWINPUT rawInput, bool* pbModePressed, Controller& controller)
    {
        if (!hidEnabled)
        {
            return;
        }

        UINT dwSize;
        GetRawInputData(rawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        std::vector<BYTE> lpb(dwSize);

        if (GetRawInputData(rawInput, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
        {
            return;
        }

        RAWINPUT *raw = (RAWINPUT *)lpb.data();
        if (raw->header.dwType == RIM_TYPEHID && (raw->header.hDevice == hDevice || raw->header.hDevice == Ally::FindHIDDevice()))
        {
            hDevice = raw->header.hDevice;

            std::stringstream seq;
            for (size_t i = 0; i < raw->data.hid.dwCount * raw->data.hid.dwSizeHid; i++)
            {
                seq << (int)raw->data.hid.bRawData[i] << " ";
            }
            seq << ("\n");
            log.Trace("Recieved sequence: %s", seq.str().c_str());

            Ally::EventCode buttonCode = (Ally::EventCode)raw->data.hid.bRawData[1];

            if (buttonCode == Ally::EventCode::ModePress)
            {
                if (!*pbModePressed)
                {
                    *pbModePressed = true;
                    if (Config::AllyHidModeMouse)
                        controller.Start(Config::AllyHidModeMouse);
                }
            }
            else if (buttonCode == Ally::EventCode::Release)
            {
                if (*pbModePressed)
                {
                    *pbModePressed = false;
                    controller.Stop();
                }
            }
            else if (*pbModePressed &&
                     (buttonCode == Ally::ACHold || buttonCode == Ally::ACPress || buttonCode == Ally::CCPress || buttonCode == Ally::LibraryPress))
            {
                buttonCode = (Ally::EventCode)(buttonCode + Ally::EventCode::ModePress);
            }

            if (Config::AllyHidEnable && Ally::ButtonBind.find(buttonCode) != Ally::ButtonBind.end() &&
                Ally::ButtonBind[buttonCode])
            {
                Ally::ButtonBind[buttonCode]();
            }
        }
    }

    void OnHotkey(int id)
    {
        if (Config::AllyControlModeHotkeys)
        {
            for (const ControlModeHotkey& hotkey : controlModeHotkeys)
            {
                if (id == hotkey.id)
                {
                    hotkey.handler();
                    return;
                }
            }
        }
        switch (id)
        {
            case HotkeyEnterFSEWithReboot:
                GamingExperience::EnterFSEModeWithReboot();
                break;
            case HotkeyEnterFSENow:
                GamingExperience::EnterFSEModeNow();
                break;
        }
    }

    void OnUser(HWND hwnd, bool& hidEnabled, HANDLE& hDevice, RAWINPUTDEVICE& rid, bool& bModePressed, Controller& controller)
    {
        const int previousMouseMode = Config::AllyHidModeMouse;
        Config::Load();
        if (previousMouseMode != Config::AllyHidModeMouse || Config::AllyHidModeMouse == Config::MouseDisabled)
            controller.Stop();
        bool enableHid = (Config::AllyHidEnable || Config::AllyHidModeMouse) && Ally::IsSupported();
        if (enableHid)
        {
            Ally::Load();
            if (!hidEnabled)
            {
                hDevice = Ally::FindHIDDevice();
                if (Ally::GetRawInputDevice(hDevice, hwnd, &rid))
                {
                    RegisterRawInputDevices(&rid, 1, sizeof(rid));
                }
            }
        }
        else if (hidEnabled)
        {
            bModePressed = false;
            controller.Stop();
            rid.dwFlags = RIDEV_REMOVE;
            rid.hwndTarget = NULL;
            RegisterRawInputDevices(&rid, 1, sizeof(rid));
            rid = {};
            hDevice = NULL;
        }

        hidEnabled = enableHid;
        if (hidEnabled && bModePressed && previousMouseMode != Config::AllyHidModeMouse && Config::AllyHidModeMouse != Config::MouseDisabled)
            controller.Start(Config::AllyHidModeMouse);
        bool hotkeysRegistered = UpdateHotkeys(hwnd);
        if (!hidEnabled && !hotkeysRegistered)
        {
            PostQuitMessage(0);
        }
    }

    DWORD WINAPI HIDListener(LPVOID lpParam)
    {
        bool hidEnabled = (Config::AllyHidEnable || Config::AllyHidModeMouse) && Ally::IsSupported();
        if (!hidEnabled && !Config::HotkeysEnable && !(Config::AllyControlModeHotkeys && Ally::IsSupported()))
        {
            return -1;
        }

        HANDLE hDevice = hidEnabled ? Ally::FindHIDDevice() : NULL;

        if (FindWindow(HidListenerClass,NULL) != NULL)
        {
            return -1;
        }

        AnyFSE::Tools::EnablePowerEfficencyMode(true);
        if (hidEnabled)
        {
            Load();
        }

        // Create hidden window for raw input
        WNDCLASS wc = {0};
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = HidListenerClass;
        RegisterClass(&wc);

        HWND hwnd = CreateWindow(HidListenerClass, NULL, 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
        if (!hwnd)
        {
            return -1;
        }

        // Register raw input for ASUS Rog Ally service device
        RAWINPUTDEVICE rid = {};
        if (hidEnabled && Ally::GetRawInputDevice(hDevice, hwnd, &rid))
        {
            RegisterRawInputDevices(&rid, 1, sizeof(rid));
        }

        bool hotkeysRegistered = UpdateHotkeys(hwnd);
        if (!hidEnabled && !hotkeysRegistered)
        {
            DestroyWindow(hwnd);
            return -1;
        }

        log.Trace("Starting Ally HID and hotkey sink window");

        MSG msg;

        bool bModePressed = false;
        Controller controller;

        while (GetMessage(&msg, NULL, 0, 0))
        {
            switch (msg.message)
            {
            case WM_INPUT:
                OnInput(hidEnabled, hDevice, (HRAWINPUT)msg.lParam, &bModePressed, controller);
                break;
            case WM_HOTKEY:
                OnHotkey((int)msg.wParam);
                break;
            case WM_USER:
                OnUser(hwnd, hidEnabled, hDevice, rid, bModePressed, controller);
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        controller.Stop();
        UnregisterHotKey(hwnd, HotkeyEnterFSEWithReboot);
        UnregisterHotKey(hwnd, HotkeyEnterFSENow);
        for (const ControlModeHotkey& hotkey : controlModeHotkeys)
            UnregisterHotKey(hwnd, hotkey.id);
        DestroyWindow(hwnd);
        log.Trace("Exit HID and hotkey sink thread");
        return 0;
    }

    void EnableACSEInjector(bool bEnable)
    {
        bEnable &= IsNativeHandlerEnabled();

        if (IsInjectorEnabled() == bEnable )
        {
            return;
        }

        const std::wstring injectorExe = std::filesystem::path(AnyFSE::Tools::Paths::GetInstallPath()).append(Constants::InjectorExe).wstring();
        SHELLEXECUTEINFOW execute = {};
        execute.cbSize = sizeof(execute);
        execute.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
        execute.lpFile = injectorExe.c_str();
        execute.lpParameters = bEnable ? Constants::CreateServiceArgument : Constants::RemoveServiceArgument;
        execute.nShow = SW_HIDE;
        if (!ShellExecuteExW(&execute))
        {
            log.Error(log.APIError(), "Could not launch injector service operation");
            return;
        }
        if (!execute.hProcess)
        {
            log.Error("Injector service operation returned no process handle");
            return;
        }
        DWORD exitCode = ERROR_GEN_FAILURE;
        if (WaitForSingleObject(execute.hProcess, INFINITE) != WAIT_OBJECT_0 || !GetExitCodeProcess(execute.hProcess, &exitCode))
            log.Error(log.APIError(), "Could not obtain injector service operation result");
        else if (exitCode != ERROR_SUCCESS)
            log.Error("Injector service operation failed: %lu", exitCode);
        CloseHandle(execute.hProcess);
    }

    bool IsNativeHandlerEnabled()
    {
        return 0 != Process::FindFirstByExe(Constants::AsusOptimizationProcess);
    }

    bool IsInjectorEnabled()
    {
        return 0 != Process::FindFirstByExe(Constants::InjectorExe);
    }

    bool UpdateHidListener()
    {
        HWND hWnd = FindWindow(Ally::HidListenerClass, L"");
        if (hWnd)
        {
            PostMessage(hWnd, WM_USER, 0, 0);
            return true;
        }
        return false;
    }

    bool EnsureListener()
    {
        if (Ally::CheckListener())
        {
            log.Debug("Background HID/hotkey listener is not running; starting it\n");
            Process::StartProtocol(Constants::AnyFseProtocolHidListener);
        }
        return false;
    }

    bool CheckListener()
    {
        return (Config::HotkeysEnable ||
            ((Config::AllyHidEnable || Config::AllyHidModeMouse || Config::AllyControlModeHotkeys) && Ally::IsSupported()))
            && FindWindow(HidListenerClass, NULL) == NULL;
    }

    bool SetupListener()
    {
        DWORD threadId = 0;
        hHidThread = CreateThread(NULL, 0, HIDListener, NULL, 0, &threadId);
        return hHidThread != NULL;
    }

    bool WaitListener()
    {
        if (hHidThread)
        {
            WaitForSingleObjectEx(hHidThread, 5000, TRUE);
        }
        return true;
    }
}
