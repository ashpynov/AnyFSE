#include <string>
#include <vector>
#include <algorithm>
#include "Ally/Ally.hpp"
#include "Ally/Handlers.hpp"
#include "Logging/LogManager.hpp"
#include "Ally.hpp"


namespace Ally
{
    static Logger log = LogManager::GetLogger("HIDListener");
    static const wchar_t *HidListenerClass = L"HIDListener";

    static HANDLE hHidThread = nullptr;

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

                if (devicePath.find(L"VID_0B05") != std::string::npos &&
                    devicePath.find(L"PID_1ABE") != std::string::npos &&
                    devicePath.find(L"MI_02") != std::string::npos &&
                    devicePath.find(L"COL01") != std::string::npos)
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
        ButtonBind[Ally::EventCode::ROGShortPress] =    Handlers::OpenGameBarComandCenter;
        ButtonBind[Ally::EventCode::ROGHold] =          Handlers::OpenTaskSwitcher;
        ButtonBind[Ally::EventCode::CommandCenter] =    Handlers::OpenLibrary;

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
                if (raw->header.dwType == RIM_TYPEHID && raw->header.hDevice == hDevice)
                {
                    std::stringstream seq;
                    for (size_t i = 0; i < raw->data.hid.dwCount * raw->data.hid.dwSizeHid; i++)
                    {
                        seq << (int)raw->data.hid.bRawData[i] << " ";
                    }
                    seq << ("\n");
                    log.Trace("Recieved sequence: %s", seq.str().c_str());

                    Ally::EventCode buttonCode = (Ally::EventCode) raw->data.hid.bRawData[1];
                    if (Ally::ButtonBind.find(buttonCode) != Ally::ButtonBind.end() &&
                        Ally::ButtonBind[buttonCode])
                    {
                        Ally::ButtonBind[buttonCode]();
                    }
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        log.Trace("Exit HID sink thread");
        return 0;
    }

    bool CheckListener()
    {
        return Ally::FindHIDDevice() != NULL && FindWindow(HidListenerClass, NULL) != NULL;
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