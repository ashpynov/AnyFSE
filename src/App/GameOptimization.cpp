#include <windows.h>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include "GameOptimization.hpp"
#include "GameBoost.hpp"
#include "GamingExperience.hpp"
#include "Constants.hpp"
#include "Tools/Elevated.hpp"
#include "Tools/Process.hpp"
#include "Tools/Registry.hpp"
#include "Logging/LogManager.hpp"

#pragma comment(lib, "advapi32.lib")

namespace AnyFSE::App::GameOptimization
{
    namespace c = AnyFSE::App::Constants;
    namespace
    {
        Logger log = LogManager::GetLogger("GameOptimization");
        struct ServiceCloser { void operator()(SC_HANDLE handle) const { CloseServiceHandle(handle); } };
        typedef std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, ServiceCloser> ServiceHandle;

        bool Status(SC_HANDLE service, SERVICE_STATUS_PROCESS& status)
        {
            DWORD bytes = 0;
            return QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                reinterpret_cast<BYTE*>(&status), sizeof(status), &bytes) != FALSE;
        }

        bool WaitForState(SC_HANDLE service, DWORD desired)
        {
            const ULONGLONG deadline = GetTickCount64() + 10000;
            SERVICE_STATUS_PROCESS status{};
            do
            {
                if (!Status(service, status)) return false;
                if (status.dwCurrentState == desired) return true;
                Sleep(100);
            } while (GetTickCount64() < deadline);
            return false;
        }

        bool HasPendingRestore()
        {
            if (GameBoost::HasPendingRestore()) return true;
            for (const auto name : c::OptimizationServices)
                if (Registry::ValueExists(c::OptimizationStateKey, name)) return true;
            return false;
        }
    }

    void Synchronize()
    {
        const bool boostComplete = GameBoost::Synchronize(GamingExperience::IsFullscreenMode());
        ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
        if (!manager) throw std::runtime_error("Cannot open service manager");
        bool complete = true;
        for (const auto name : c::OptimizationServices)
        {
            // Recheck for every service: the user may leave gaming mode during a transition.
            const bool gaming = GamingExperience::IsFullscreenMode();
            const bool recorded = Registry::ValueExists(c::OptimizationStateKey, name);
            if (!gaming && !recorded) continue;
            ServiceHandle service(OpenServiceW(manager.get(), name, SERVICE_QUERY_STATUS | SERVICE_STOP | SERVICE_START));
            if (!service)
            {
                const DWORD error = GetLastError();
                if (error == ERROR_SERVICE_DOES_NOT_EXIST)
                {
                    if (recorded && !Registry::DeleteValue(c::OptimizationStateKey, name)) complete = false;
                    continue;
                }
                log.Warn("Cannot open optimization service %ls: %lu", name, error);
                complete = false;
                continue;
            }
            SERVICE_STATUS_PROCESS status{};
            if (!Status(service.get(), status)) { complete = false; continue; }
            if (gaming)
            {
                // A journal entry means this service has already been handled this session.
                // Do not fight Windows or another application that restarts it.
                if (recorded || status.dwCurrentState == SERVICE_STOPPED) continue;
                if (status.dwCurrentState != SERVICE_RUNNING) { complete = false; continue; }
                if (!(status.dwControlsAccepted & SERVICE_ACCEPT_STOP)) continue;
                // Persist before STOP; never modify service startup configuration.
                if (!Registry::WriteDWORD(c::OptimizationStateKey, name, SERVICE_RUNNING)
                    || !Registry::Flush(c::OptimizationStateKey))
                {
                    complete = false;
                    continue;
                }
                SERVICE_STATUS stopped{};
                if (!ControlService(service.get(), SERVICE_CONTROL_STOP, &stopped))
                {
                    const DWORD error = GetLastError();
                    // SCM rejects active dependents; never stop them recursively.
                    log.Warn("Service %ls was not stopped: %lu", name, error);
                    // Keep the journal even on failure: a timeout can have an ambiguous outcome.
                    continue;
                }
                if (!WaitForState(service.get(), SERVICE_STOPPED)) complete = false;
                else log.Info("Temporarily stopped %ls", name);
            }
            else
            {
                if (status.dwCurrentState == SERVICE_STOP_PENDING && !WaitForState(service.get(), SERVICE_STOPPED))
                { complete = false; continue; }
                if (!Status(service.get(), status)) { complete = false; continue; }
                if (status.dwCurrentState == SERVICE_STOPPED && !StartServiceW(service.get(), 0, nullptr))
                {
                    log.Warn("Restore failed for %ls: %lu; journal retained", name, GetLastError());
                    complete = false;
                    continue;
                }
                if (!WaitForState(service.get(), SERVICE_RUNNING)) { complete = false; continue; }
                if (!Registry::DeleteValue(c::OptimizationStateKey, name)) complete = false;
                else log.Info("Restored %ls", name);
            }
        }
        if (!complete || !boostComplete) throw std::runtime_error("Gaming optimization incomplete; will retry");
    }

    void StartMonitor()
    {
        HANDLE existing = OpenMutexW(SYNCHRONIZE, FALSE, c::OptimizationMonitorMutex);
        if (existing) { CloseHandle(existing); return; }
        wchar_t executable[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
        if (!length || length >= MAX_PATH || !Process::StartProcess(executable, c::OptimizationMonitorArgument))
            log.Error("Cannot start gaming optimization monitor");
    }

    int Monitor()
    {
        HANDLE mutex = CreateMutexW(nullptr, FALSE, c::OptimizationMonitorMutex);
        const DWORD error = GetLastError();
        if (!mutex) return 1;
        if (error == ERROR_ALREADY_EXISTS) { CloseHandle(mutex); return 0; }
        // This independent process survives the splash/launcher process and detects external mode changes.
        bool initialized = false;
        bool previous = false;
        ULONGLONG nextRetry = 0;
        for (;;)
        {
            const bool gaming = GamingExperience::IsFullscreenMode();
            if (!initialized || gaming != previous || GetTickCount64() >= nextRetry)
            {
                const bool needed = gaming || HasPendingRestore();
                const bool success = !needed || Elevated::Call(c::ElevatedOptimizeServices);
                previous = gaming;
                initialized = true;
                nextRetry = success ? ~0ULL : GetTickCount64() + 30000;
            }
            Sleep(1000);
        }
    }
}
