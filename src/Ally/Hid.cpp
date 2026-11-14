#include <algorithm>
#include <string>
#include <windows.h>
#include <hidsdi.h>
#include "Ally/Ally.hpp"
#include "Ally/Hid.hpp"
#include "App/Constants.hpp"
#include "Logging/LogManager.hpp"

#pragma comment(lib, "hid.lib")

namespace Ally::Hid
{
    namespace c = AnyFSE::App::Constants;
    static auto log = AnyFSE::Logging::LogManager::GetLogger("Hid");

    constexpr BYTE ReportId = 0x5A;
    constexpr USHORT ButtonAddress = 0xE500;
    constexpr size_t ReadBlockSize = 16;
    constexpr unsigned ButtonReadyRetries = 20;
    constexpr DWORD ButtonReadyDelay = 15;

    FeatureReport::FeatureReport(USHORT blockAddress, size_t count)
        : reportId(ReportId)
        , command(Command::Read)
        , address(blockAddress)
        , reserved(0)
        , length(static_cast<BYTE>(count + sizeof(FeatureReport) - sizeof(payload) - sizeof(reportId)))
        , payload {}
    {
    }

    ButtonStatusReport::ButtonStatusReport()
        : reportId(ReportId)
        , command(Command::Customize)
        , subcommand(Command::ButtonStatus)
        , length(sizeof(status))
        , status(0)
        , reserved {}
    {
    }

    SetButtonsReport::SetButtonsReport(BYTE blockIndex, const ButtonBlock& block)
        : reportId(ReportId)
        , command(Command::Customize)
        , subcommand(Command::SetButtons)
        , index(blockIndex)
        , length(sizeof(buttons))
        , buttons(block)
        , reserved {}
    {
    }

    SetSticksReport::SetSticksReport(const StickMapping& left, const StickMapping& right)
        : reportId(ReportId)
        , command(Command::Customize)
        , subcommand(Command::SetSticks)
        , length(static_cast<BYTE>(sizeof(leftStick) + sizeof(rightStick)))
        , leftStick(left)
        , rightStick(right)
        , reserved {}
    {
    }

    McuLock::McuLock()
    {
        // Match AC SE's order and namespaces, including its session-local MCU mutex.
        struct MutexRequest
        {
            const wchar_t* name;
            DWORD timeout;
        };
        constexpr MutexRequest requests[] = {
            { c::AllyMcuMutex, 60000 },
            { c::AllyAacNbHalMutex, 3000 },
            { c::AllyGamepadDeviceMutex, 2000 }
        };
        static_assert(sizeof(requests) / sizeof(requests[0]) == sizeof(m_handles) / sizeof(m_handles[0]));
        size_t index = 0;
        for (const MutexRequest& request : requests)
        {
            HANDLE handle = OpenMutex(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, request.name);
            if (!handle)
            {
                const DWORD error = GetLastError();
                if (error == ERROR_FILE_NOT_FOUND)
                {
                    log.Warn("Controller mutex '%ls' does not exist; continuing without it", request.name);
                    ++index;
                    continue;
                }
                log.Warn(log.APIError(error), "Could not open controller mutex '%ls'", request.name);
                Release();
                return;
            }
            const DWORD result = WaitForSingleObject(handle, request.timeout);
            if (result != WAIT_OBJECT_0 && result != WAIT_ABANDONED)
            {
                const DWORD error = GetLastError();
                if (result == WAIT_TIMEOUT)
                    log.Warn("Controller mutex '%ls' timed out after %lu ms", request.name, request.timeout);
                else
                    log.Warn(log.APIError(error), "Could not acquire controller mutex '%ls' (wait result %lu)", request.name, result);
                CloseHandle(handle);
                Release();
                return;
            }
            if (result == WAIT_ABANDONED)
                log.Warn("Controller mutex '%ls' was abandoned; ownership acquired", request.name);
            m_handles[index++] = handle;
        }
        m_acquired = true;
    }

    McuLock::~McuLock()
    {
        Release();
    }

    void McuLock::Release()
    {
        for (size_t index = sizeof(m_handles) / sizeof(m_handles[0]); index > 0; --index)
        {
            HANDLE& handle = m_handles[index - 1];
            if (handle)
            {
                ReleaseMutex(handle);
                CloseHandle(handle);
                handle = nullptr;
            }
        }
        m_acquired = false;
    }

    HANDLE Open()
    {
        std::wstring path;
        if (!FindHIDDevice(&path))
            return INVALID_HANDLE_VALUE;
        return CreateFile(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    }

    static std::string ReportBytes(const void* report, size_t size)
    {
        constexpr char hex[] = "0123456789ABCDEF";
        const BYTE* bytes = static_cast<const BYTE*>(report);
        std::string result;
        result.reserve(size * 3);
        for (size_t i = 0; i < size; ++i)
        {
            if (i)
                result += ' ';
            result += hex[bytes[i] >> 4];
            result += hex[bytes[i] & 0x0F];
        }
        return result;
    }

    static bool ReadBlock(HANDLE device, USHORT address, void* destination, size_t count, bool& deviceLost)
    {
        if (count > sizeof(FeatureReport::payload))
            return false;

        FeatureReport report(address, count);

        const FeatureReport request = report;
        if (!HidD_SetFeature(device, &report, DWORD(sizeof(report))) ||
            !HidD_GetFeature(device, &report, DWORD(sizeof(report))))
        {
            const DWORD error = GetLastError();
            deviceLost = (error == ERROR_DEVICE_NOT_CONNECTED || error == ERROR_INVALID_HANDLE || error == ERROR_NO_SUCH_DEVICE);
            log.Warn(log.APIError(error), "Could not read controller mapping");
            return false;
        }
        if (report != request)
        {
            log.Warn("Unexpected controller mapping response at 0x%04X: %s", unsigned(address), ReportBytes(&report, sizeof(report)).c_str());
            return false;
        }
        std::copy_n(report.payload, count, (BYTE*)destination);
        return true;
    }

    bool ReadMapping(HANDLE device, Mapping& mapping, bool& deviceLost)
    {
        BYTE* destination = (BYTE*)&mapping;
        for (size_t offset = 0; offset < sizeof(Mapping); offset += ReadBlockSize)
        {
            const size_t count = (std::min)(ReadBlockSize, sizeof(Mapping) - offset);
            if (!ReadBlock(device, USHORT(ButtonAddress + offset), destination + offset, count, deviceLost))
                return false;
        }
        return true;
    }

    static bool WaitForButtons(HANDLE device)
    {
        for (unsigned attempt = 0; attempt <= ButtonReadyRetries; ++attempt)
        {
            ButtonStatusReport report;
            if (!HidD_SetFeature(device, &report, DWORD(sizeof(report))))
            {
                log.Warn(log.APIError(), "Could not request controller button status");
                return false;
            }
            // Clear the request so unchanged bytes cannot masquerade as an acknowledgement.
            std::memset(&report, 0, sizeof(report));
            report.reportId = ReportId;
            if (!HidD_GetFeature(device, &report, DWORD(sizeof(report))))
            {
                log.Warn(log.APIError(), "Could not read controller button status");
                return false;
            }
            // The MCU acknowledges readiness with command 0x0A, not a zero payload byte.
            if (report.reportId == ReportId && report.command == Command::Customize && report.subcommand == Command::ButtonStatus)
                return true;
            log.Trace("Unexpected controller button status response (attempt %u): %s", attempt + 1, ReportBytes(&report, sizeof(report)).c_str());
            if (attempt == ButtonReadyRetries)
                log.Warn("Controller readiness was not acknowledged; last response: %s", ReportBytes(&report, sizeof(report)).c_str());
            if (attempt < ButtonReadyRetries)
                Sleep(ButtonReadyDelay);
        }
        return false;
    }

    bool WriteMapping(HANDLE device, const Mapping& mapping)
    {
        const RogAllyVersion version = GetRogAllyVersion();
        // ASUS checks button readiness only on RC71; newer MCU families write directly.
        const bool checkButtons = version == RogAllyVersion::ROGAlly;
        // The MCU needs the complete button table before the stick command,
        // even when only the sticks change. Preserve every button byte.
        BYTE index = 1;
        for (const ButtonBlock& block : mapping.buttons.blocks)
        {
            if (checkButtons && !WaitForButtons(device))
                return false;
            SetButtonsReport report(index++, block);
            if (!HidD_SetFeature(device, &report, DWORD(sizeof(report))))
            {
                log.Warn(log.APIError(), "Could not write controller buttons");
                return false;
            }
        }
        SetSticksReport report(mapping.leftStick, mapping.rightStick);
        if (HidD_SetFeature(device, &report, DWORD(sizeof(report))))
            return true;

        log.Warn(log.APIError(), "Could not write controller sticks");
        return false;
    }
}
