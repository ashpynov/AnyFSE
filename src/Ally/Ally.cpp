#include <string>
#include <vector>
#include <algorithm>
#include "Ally/Ally.hpp"
#include "Ally/Handlers.hpp"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Process.hpp"
#include "Ally.hpp"


namespace Ally
{
    static Logger log = LogManager::GetLogger("HIDListener");
    static const wchar_t *HidListenerClass = L"HIDListener";

    static HANDLE hHidThread = nullptr;

    bool IsSupported()
    {
        static int supported = -1;
        if (supported == -1)
        {
            supported = FindHIDDevice() ? 1 : 0;

            log.Trace("Ally HID is %d", supported);
        }
        return supported;
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

                if (devicePath.find(L"VID_0B05") != std::string::npos
                    && ( devicePath.find(L"PID_1ABE") != std::string::npos
                        || devicePath.find(L"PID_1B4C") != std::string::npos )
                    && devicePath.find(L"MI_02") != std::string::npos
                    && devicePath.find(L"COL01") != std::string::npos)
                {
                    return deviceList[i].hDevice;
                }
            }
        }
        return NULL;
    }

    bool GetRawInputDevice(HANDLE hDevice, HWND hWnd, RAWINPUTDEVICE *pRid)
    {
        // --- Now you have the Device Path. Next, get its Usage Page and Usage ---
        RID_DEVICE_INFO deviceInfo;
        deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
        UINT infoSize = sizeof(RID_DEVICE_INFO);

        if (GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &deviceInfo, &infoSize) > 0)
        {
            // This is the mapping you are looking for!
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
        ButtonBind[Ally::EventCode::ACPress] =          Handlers::GetByName(Config::AllyHidACPress);
        ButtonBind[Ally::EventCode::ACHold] =           Handlers::GetByName(Config::AllyHidACHold);
        ButtonBind[Ally::EventCode::CCPress] =          Handlers::GetByName(Config::AllyHidCCPress);

        ButtonBind[Ally::EventCode::MACPress] =         Handlers::GetByName(Config::AllyHidModeACPress);
        ButtonBind[Ally::EventCode::MACHold] =          Handlers::GetByName(Config::AllyHidModeACHold);
        ButtonBind[Ally::EventCode::MCCPress] =         Handlers::GetByName(Config::AllyHidModeCCPress);

        ButtonBind[Ally::EventCode::ToggleMicrophone] = Handlers::ToggleMicrophone;
        ButtonBind[Ally::EventCode::ScreenShot] =       Handlers::TakeScreenShoot;
        ButtonBind[Ally::EventCode::ShowKeyboard] =     Handlers::ShowKeyboard;
        ButtonBind[Ally::EventCode::ToggleRecord] =     Handlers::ToggleRecord;
        ButtonBind[Ally::EventCode::ModePress] =        NULL;
        ButtonBind[Ally::EventCode::ROGHoldRelease] =   NULL;
        ButtonBind[Ally::EventCode::Unknown] =          NULL;
        ButtonBind[Ally::EventCode::Release] =          NULL;
    }

    DWORD WINAPI HIDListener(LPVOID lpParam)
    {
        if (!Config::AllyHidEnable)
        {
            return -1;
        }

        HANDLE hDevice = Ally::FindHIDDevice();

        if (!hDevice)
        {
            return -1;
        }

        if (FindWindow(HidListenerClass,NULL) != NULL)
        {
            return -1;
        }

        Load();

        // Create hidden window for raw input
        WNDCLASS wc = {0};
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = HidListenerClass;
        RegisterClass(&wc);

        HWND hwnd = CreateWindow(HidListenerClass, NULL, 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);

        // Register raw input for ASUS Rog Ally service device
        RAWINPUTDEVICE rid;
        if (Ally::GetRawInputDevice(hDevice, hwnd, &rid))
        {
            RegisterRawInputDevices(&rid, 1, sizeof(rid));
        }

        log.Trace("Starting Ally HID sink window");

        MSG msg;

        bool bModePressed = false;

        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (msg.message == WM_INPUT)
            {
                UINT dwSize;
                GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
                std::vector<BYTE> lpb(dwSize);

                if (GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
                {
                    continue;
                }

                RAWINPUT *raw = (RAWINPUT *)lpb.data();
                if (raw->header.dwType == RIM_TYPEHID
                    && (raw->header.hDevice == hDevice
                     || raw->header.hDevice == Ally::FindHIDDevice()))
                {
                    hDevice = raw->header.hDevice;

                    std::stringstream seq;
                    for (size_t i = 0; i < raw->data.hid.dwCount * raw->data.hid.dwSizeHid; i++)
                    {
                        seq << (int)raw->data.hid.bRawData[i] << " ";
                    }
                    seq << ("\n");
                    log.Trace("Recieved sequence: %s", seq.str().c_str());

                    Ally::EventCode buttonCode = (Ally::EventCode) raw->data.hid.bRawData[1];

                    if (buttonCode == Ally::EventCode::ModePress)
                    {
                        bModePressed = true;
                    }
                    else if (buttonCode == Ally::EventCode::Release)
                    {
                        bModePressed = false;
                    }
                    else if (bModePressed &&
                                (  buttonCode == Ally::ACHold
                                || buttonCode == Ally::ACPress
                                || buttonCode == Ally::CCPress )
                    )
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
            else if (msg.message == WM_USER)
            {
                Config::Load();
                if (!Config::AllyHidEnable)
                {
                    PostQuitMessage(0);
                }
                else
                {
                    Ally::Load();
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        log.Trace("Exit HID sink thread");
        return 0;
    }

    void EnableNativeHandler(bool bEnable)
    {

        bool active = IsNativeHandlerEnabled();
        if (active == bEnable)
        {
            return;
        }

        std::wstring serviceName = L"ASUSOptimization";
        std::wstring commandLine = bEnable
            ? L"sc config " + serviceName + L" start=auto & sc start " + serviceName
            : L"sc stop " + serviceName + L" & sc config " + serviceName + L" start= disabled";


        // Execute through cmd.exe with hidden window
        ShellExecuteW(
            NULL,                           // Parent window handle
            L"runas",                       // Operation (runas for elevation)
            L"cmd.exe",                     // Program to execute
            (L"/c " + commandLine).c_str(), // Command line arguments
            NULL,                           // Working directory
            SW_HIDE                         // Window state (hidden)
        );
    }

    bool IsNativeHandlerEnabled()
    {
        return 0 != Process::FindFirstByExe(L"AsusOptimization.exe");
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
        return Ally::IsSupported() && FindWindow(HidListenerClass, NULL) == NULL;
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