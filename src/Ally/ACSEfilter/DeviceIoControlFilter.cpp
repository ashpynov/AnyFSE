#include "DeviceIoControlFilter.h"

#include "Config.h"
#include "DebugLog.h"

#include <cstring>

namespace ACSEFilter
{
    namespace
    {
        struct AtkButtonRequest
        {
            DWORD method;
            DWORD argumentSize;
            DWORD deviceId;
            DWORD buttonCode;
        };

        bool ShouldSuppressRequest(DWORD controlCode, const void *inputBuffer, DWORD inputSize, DWORD& buttonCode)
        {
            if (controlCode != Config::kAtkWmiIoControlCode || !inputBuffer || inputSize != sizeof(AtkButtonRequest))
            {
                return false;
            }

            AtkButtonRequest request;
            std::memcpy(&request, inputBuffer, sizeof(request));
            if (request.method != Config::kAtkDevsMethod
                || request.argumentSize != 2 * sizeof(DWORD)
                || request.deviceId != Config::kAtkHidEventDeviceId)
            {
                return false;
            }

            for (const BYTE code : Config::kSuppressedButtonCodes)
            {
                if (request.buttonCode == code)
                {
                    buttonCode = request.buttonCode;
                    return true;
                }
            }

            return false;
        }
    } // namespace

    BOOL WINAPI HookDeviceIoControl(
        HANDLE device,
        DWORD controlCode,
        LPVOID inputBuffer,
        DWORD inputSize,
        LPVOID outputBuffer,
        DWORD outputSize,
        LPDWORD bytesReturned,
        LPOVERLAPPED overlapped)
    {
        DWORD buttonCode = 0;
        if (device
            && device != INVALID_HANDLE_VALUE
            && !overlapped
            && bytesReturned
            && outputBuffer
            && outputSize >= sizeof(DWORD)
            && ShouldSuppressRequest(controlCode, inputBuffer, inputSize, buttonCode))
        {
            // The inspected ASUS caller tests only BOOL and accepts an empty response.
            // Complete this synchronous request without issuing it to the driver.
            *bytesReturned = 0;
            LOG(L"Suppress ATK DEVS HID button code 0x%02lX.", buttonCode);
            return TRUE;
        }

        // Only the ASUS executable's import is patched; this DLL keeps its own original import.
        return ::DeviceIoControl(device, controlCode, inputBuffer, inputSize, outputBuffer, outputSize, bytesReturned, overlapped);
    }
} // namespace ACSEFilter
