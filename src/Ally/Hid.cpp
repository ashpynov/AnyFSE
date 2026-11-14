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
        , length(sizeof(busy))
        , busy(0)
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
        m_handle = OpenMutex(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, c::AllyMcuGlobalMutex);
        if (!m_handle)
            return;
        const DWORD result = WaitForSingleObject(m_handle, INFINITE);
        if (result != WAIT_OBJECT_0 && result != WAIT_ABANDONED)
        {
            CloseHandle(m_handle);
            m_handle = nullptr;
        }
    }

    McuLock::~McuLock()
    {
        if (m_handle)
        {
            ReleaseMutex(m_handle);
            CloseHandle(m_handle);
        }
    }

    HANDLE Open()
    {
        std::wstring path;
        if (!FindHIDDevice(&path))
            return INVALID_HANDLE_VALUE;
        return CreateFile(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
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
            log.Warn("Unexpected controller mapping response");
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
            if (!HidD_SetFeature(device, &report, DWORD(sizeof(report))) ||
                !HidD_GetFeature(device, &report, DWORD(sizeof(report))))
            {
                log.Warn(log.APIError(), "Could not read controller button status");
                return false;
            }
            if (report.reportId != ReportId || report.command != Command::Customize ||
                report.subcommand != Command::ButtonStatus || report.length != sizeof(report.busy))
            {
                log.Warn("Unexpected controller button status response");
                return false;
            }
            if (!report.busy)
                return true;
            if (attempt < ButtonReadyRetries)
                Sleep(ButtonReadyDelay);
        }
        log.Warn("Controller button mapping remained busy");
        return false;
    }

    bool WriteMapping(HANDLE device, const Mapping& mapping)
    {
        // The MCU needs the complete button table before the stick command,
        // even when only the sticks change. Preserve every button byte.
        BYTE index = 1;
        for (const ButtonBlock& block : mapping.buttons.blocks)
        {
            if (!WaitForButtons(device))
                return false;
            SetButtonsReport report(index++, block);
            if (!HidD_SetFeature(device, &report, DWORD(sizeof(report))))
            {
                log.Warn(log.APIError(), "Could not write controller buttons");
                return false;
            }
        }
        if (!WaitForButtons(device))
            return false;

        SetSticksReport report(mapping.leftStick, mapping.rightStick);
        if (HidD_SetFeature(device, &report, DWORD(sizeof(report))))
            return true;

        log.Warn(log.APIError(), "Could not write controller sticks");
        return false;
    }
}
