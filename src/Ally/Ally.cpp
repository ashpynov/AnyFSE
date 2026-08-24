#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
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

using namespace AnyFSE::App;

namespace Ally
{

    static Logger log = LogManager::GetLogger("HIDListener");
    static const wchar_t *HidListenerClass = L"HIDListener";
    static constexpr int HotkeyEnterFSEWithReboot = (int)GamingExperience::ConfirmationMode::Reboot;
    static constexpr int HotkeyEnterFSENow = (int)GamingExperience::ConfirmationMode::Now;
    static HANDLE hHidThread = nullptr;

    static bool UpdateHotkeys(HWND hWnd)
    {
        UnregisterHotKey(hWnd, HotkeyEnterFSEWithReboot);
        UnregisterHotKey(hWnd, HotkeyEnterFSENow);

        if (!Config::HotkeysEnable)
        {
            return false;
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

        return rebootRegistered || nowRegistered;
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

    HANDLE FindHIDDevice()
    {
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
    }

    void OnInput(bool allyEnabled, HANDLE hDevice, HRAWINPUT rawInput, bool * pbModePressed)
    {
        if (!allyEnabled)
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
                *pbModePressed = true;
            }
            else if (buttonCode == Ally::EventCode::Release)
            {
                *pbModePressed = false;
            }
            else if (*pbModePressed &&
                     (buttonCode == Ally::ACHold || buttonCode == Ally::ACPress || buttonCode == Ally::CCPress || buttonCode == Ally::LibraryPress))
            {
                buttonCode = (Ally::EventCode)(buttonCode + Ally::EventCode::ModePress);
            }

            if (Ally::ButtonBind.find(buttonCode) != Ally::ButtonBind.end() &&
                Ally::ButtonBind[buttonCode])
            {
                Ally::ButtonBind[buttonCode]();
            }
        }
    }

    void OnHotkey(int id)
    {
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

    void OnUser(HWND hwnd, bool allyEnabled, HANDLE hDevice, RAWINPUTDEVICE& rid)
    {
        Config::Load();
        bool enableAlly = Config::AllyHidEnable && Ally::IsSupported();
        if (enableAlly)
        {
            Ally::Load();
            if (!allyEnabled)
            {
                hDevice = Ally::FindHIDDevice();
                if (Ally::GetRawInputDevice(hDevice, hwnd, &rid))
                {
                    RegisterRawInputDevices(&rid, 1, sizeof(rid));
                }
            }
        }
        else if (allyEnabled)
        {
            rid.dwFlags = RIDEV_REMOVE;
            rid.hwndTarget = NULL;
            RegisterRawInputDevices(&rid, 1, sizeof(rid));
            rid = {};
            hDevice = NULL;
        }

        allyEnabled = enableAlly;
        bool hotkeysRegistered = UpdateHotkeys(hwnd);
        if (!allyEnabled && !hotkeysRegistered)
        {
            PostQuitMessage(0);
        }
    }

    DWORD WINAPI HIDListener(LPVOID lpParam)
    {
        bool allyEnabled = Config::AllyHidEnable && Ally::IsSupported();
        if (!allyEnabled && !Config::HotkeysEnable)
        {
            return -1;
        }

        HANDLE hDevice = allyEnabled ? Ally::FindHIDDevice() : NULL;

        if (FindWindow(HidListenerClass,NULL) != NULL)
        {
            return -1;
        }

        AnyFSE::Tools::EnablePowerEfficencyMode(true);
        if (allyEnabled)
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
        if (allyEnabled && Ally::GetRawInputDevice(hDevice, hwnd, &rid))
        {
            RegisterRawInputDevices(&rid, 1, sizeof(rid));
        }

        bool hotkeysRegistered = UpdateHotkeys(hwnd);
        if (!allyEnabled && !hotkeysRegistered)
        {
            DestroyWindow(hwnd);
            return -1;
        }

        log.Trace("Starting Ally HID and hotkey sink window");

        MSG msg;

        bool bModePressed = false;

        while (GetMessage(&msg, NULL, 0, 0))
        {
            switch (msg.message)
            {
            case WM_INPUT:
                OnInput(allyEnabled, hDevice, (HRAWINPUT)msg.lParam, &bModePressed);
                break;
            case WM_HOTKEY:
                OnHotkey((int)msg.wParam);
                break;
            case WM_USER:
                OnUser(hwnd, allyEnabled, hDevice, rid);
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        UnregisterHotKey(hwnd, HotkeyEnterFSEWithReboot);
        UnregisterHotKey(hwnd, HotkeyEnterFSENow);
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

        bEnable ? Services::EnableInjectorService() : Services::DisableInjectorService();
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

    bool CheckListener()
    {
        return (Config::HotkeysEnable || (Config::AllyHidEnable && Ally::IsSupported()))
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
