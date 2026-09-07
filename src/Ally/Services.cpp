#include <filesystem>
#include "Services.hpp"
#include <windows.h>
#include "Tools/Paths.hpp"
#include "Tools/Process.hpp"
#include "App/Constants.hpp"
#include "Logging/LogManager.hpp"

#pragma comment(lib, "Advapi32.lib")

namespace Ally::Services
{
    namespace c = AnyFSE::App::Constants;
    namespace Tools = AnyFSE::Tools;
    static Logger log = LogManager::GetLogger("AllyServices");

    namespace
    {
        constexpr DWORD ServiceTimeoutMs = 30000;

        struct ServiceHandle
        {
            SC_HANDLE value;
            explicit ServiceHandle(SC_HANDLE handle) : value(handle) {}
            ~ServiceHandle() { const DWORD error = GetLastError(); if (value) CloseServiceHandle(value); SetLastError(error); }
            ServiceHandle(const ServiceHandle&) = delete;
            ServiceHandle& operator=(const ServiceHandle&) = delete;
        };

        bool Fail(DWORD error)
        {
            SetLastError(error);
            return false;
        }

        bool QueryStatus(SC_HANDLE service, SERVICE_STATUS_PROCESS& status)
        {
            DWORD bytesNeeded = 0;
            return QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, reinterpret_cast<BYTE*>(&status), sizeof(status), &bytesNeeded) != FALSE;
        }

        bool WaitForState(SC_HANDLE service, DWORD desiredState)
        {
            const ULONGLONG deadline = GetTickCount64() + ServiceTimeoutMs;
            for (;;)
            {
                SERVICE_STATUS_PROCESS status = {};
                if (!QueryStatus(service, status)) return false;
                if (status.dwCurrentState == desiredState) return true;
                if (desiredState == SERVICE_RUNNING && status.dwCurrentState == SERVICE_STOPPED)
                    return Fail(status.dwWin32ExitCode ? status.dwWin32ExitCode : ERROR_SERVICE_NOT_ACTIVE);
                if (GetTickCount64() >= deadline) return Fail(ERROR_TIMEOUT);
                Sleep(250);
            }
        }

        bool Start(SC_HANDLE service)
        {
            SERVICE_STATUS_PROCESS status = {};
            if (!QueryStatus(service, status)) return false;
            if (status.dwCurrentState == SERVICE_RUNNING) return true;
            if (status.dwCurrentState == SERVICE_STOP_PENDING && !WaitForState(service, SERVICE_STOPPED)) return false;
            if (status.dwCurrentState != SERVICE_START_PENDING && !StartServiceW(service, 0, nullptr))
            {
                if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) return false;
            }
            return WaitForState(service, SERVICE_RUNNING);
        }

        bool Stop(SC_HANDLE service)
        {
            SERVICE_STATUS_PROCESS status = {};
            if (!QueryStatus(service, status)) return false;
            if (status.dwCurrentState == SERVICE_STOPPED) return true;
            if (status.dwCurrentState == SERVICE_START_PENDING && !WaitForState(service, SERVICE_RUNNING))
            {
                const DWORD error = GetLastError();
                if (!QueryStatus(service, status)) return false;
                return status.dwCurrentState == SERVICE_STOPPED ? true : Fail(error);
            }
            if (status.dwCurrentState != SERVICE_STOP_PENDING)
            {
                SERVICE_STATUS stopped = {};
                if (!ControlService(service, SERVICE_CONTROL_STOP, &stopped) && GetLastError() != ERROR_SERVICE_NOT_ACTIVE) return false;
            }
            return WaitForState(service, SERVICE_STOPPED);
        }
    }

    // These methods require an elevated caller.
    bool CreateInjector()
    {
        const std::wstring injectorExe = std::filesystem::path(Tools::Paths::GetInstallPath()).append(c::InjectorExe).wstring();
        ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE));
        if (!manager.value) return false;

        const std::wstring commandLine = L"\"" + injectorExe + L"\" " + c::InjectorServiceArgument;
        constexpr DWORD access = SERVICE_CHANGE_CONFIG | SERVICE_START | SERVICE_QUERY_STATUS;
        ServiceHandle service(OpenServiceW(manager.value, c::InjectorServiceName, access));
        if (!service.value)
        {
            if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST) return false;
            service.value = CreateServiceW(manager.value, c::InjectorServiceName, c::InjectorServiceDisplayName, access,
                SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, commandLine.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);
            if (!service.value)
            {
                if (GetLastError() != ERROR_SERVICE_EXISTS) return false;
                service.value = OpenServiceW(manager.value, c::InjectorServiceName, access);
                if (!service.value) return false;
            }
        }

        if (!ChangeServiceConfigW(service.value, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE,
                commandLine.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr, c::InjectorServiceDisplayName)) return false;

        std::wstring descriptionText(c::InjectorServiceDescription);
        SERVICE_DESCRIPTIONW description = {descriptionText.data()};
        if (!ChangeServiceConfig2W(service.value, SERVICE_CONFIG_DESCRIPTION, &description)) return false;

        SC_ACTION actions[] = {{SC_ACTION_RESTART, 5000}, {SC_ACTION_RESTART, 5000}, {SC_ACTION_RESTART, 5000}};
        SERVICE_FAILURE_ACTIONSW failureActions = {};
        failureActions.dwResetPeriod = 0;
        failureActions.cActions = ARRAYSIZE(actions);
        failureActions.lpsaActions = actions;
        if (!ChangeServiceConfig2W(service.value, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions)) return false;
        return Start(service.value);
    }

    bool RemoveInjector()
    {
        ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
        if (!manager.value) return false;
        ServiceHandle service(OpenServiceW(manager.value, c::InjectorServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE));
        if (!service.value) return GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST;
        if (!Stop(service.value)) return false;
        if (!DeleteService(service.value)) return GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE;
        return true;
    }

    bool EnableAsusOptimization()
    {
        ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
        if (!manager.value) return false;
        ServiceHandle service(OpenServiceW(manager.value, c::AsusOptimizationService, SERVICE_CHANGE_CONFIG | SERVICE_START | SERVICE_QUERY_STATUS));
        if (!service.value) return false;
        if (!ChangeServiceConfigW(service.value, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) return false;
        return Start(service.value);
    }

    bool EnableInjectorService()
    {
        const bool result = CreateInjector();
        if (!result) log.Error(log.APIError(), "Create injector service failed");
        return result;
    }

    bool DisableInjectorService()
    {
        const bool result = RemoveInjector();
        if (!result) log.Error(log.APIError(), "Remove injector service failed");
        return result;
    }

    bool EnableAsusOptimizationService()
    {
        if (!Process::FindFirstByExe(c::ArmouryCrateServiceProcess) || Process::FindFirstByExe(c::AsusOptimizationProcess)) return true;
        const bool result = EnableAsusOptimization();
        if (!result) log.Error(log.APIError(), "Enable ASUS Optimization service failed");
        return result;
    }
}
