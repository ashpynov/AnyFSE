#include <windows.h>
#include <tlhelp32.h>

#include <algorithm>
#include <cwctype>
#include <exception>
#include <filesystem>
#include <shellapi.h>
#include <string>
#include "../../App/AppConstants.hpp"
#include "../../Tools/PowerEfficiency.hpp"
#include "DebugLog.h"

namespace
{

    constexpr const wchar_t *kServiceName = AnyFSE::AppConstants::InjectorServiceName;
    constexpr const wchar_t *kTargetProcessName = AnyFSE::AppConstants::AsusOptimizationProcess;
    constexpr const wchar_t *kTargetServiceName = AnyFSE::AppConstants::AsusOptimizationService;
    constexpr const wchar_t *kHookDllName = L"AnyFSE.ACSEFilterHook.dll";
    constexpr DWORD kMissingProcessDelayMs = 10000;
    constexpr DWORD kRemoteThreadTimeoutMs = 30000;
    constexpr DWORD kServiceStopWaitHintMs = 60000;
    constexpr DWORD kTargetServiceRestartTimeoutMs = 45000;
    constexpr DWORD kTargetProcessAccess =
        SYNCHRONIZE |
        PROCESS_CREATE_THREAD |
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE |
        PROCESS_VM_READ;

    SERVICE_STATUS_HANDLE g_serviceStatusHandle = nullptr;
    SERVICE_STATUS g_serviceStatus = {};
    HANDLE g_serviceStopEvent = nullptr;
    HANDLE g_consoleStopEvent = nullptr;
    DWORD g_serviceCheckpoint = 1;
    bool g_shutdownRequested = false;

    bool RestartTargetService();

    bool EnableDebugPrivilege()
    {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        {
            return false;
        }

        TOKEN_PRIVILEGES privileges = {};
        privileges.PrivilegeCount = 1;

        if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &privileges.Privileges[0].Luid))
        {
            CloseHandle(token);
            return false;
        }

        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr);

        const bool result = GetLastError() == ERROR_SUCCESS;
        CloseHandle(token);
        return result;
    }

    std::wstring ToLower(std::wstring value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch)
                       { return static_cast<wchar_t>(std::towlower(ch)); });
        return value;
    }

    std::filesystem::path CurrentExecutablePath()
    {
        wchar_t modulePath[MAX_PATH] = {};
        const DWORD modulePathLength = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        return modulePathLength ? std::filesystem::path(modulePath) : std::filesystem::path();
    }

    std::filesystem::path DefaultDllPath()
    {
        const std::filesystem::path exePath = CurrentExecutablePath();
        const std::filesystem::path basePath = exePath.empty() ? std::filesystem::current_path() : exePath.parent_path();
        return basePath / kHookDllName;
    }

    DWORD FindProcessIdByName(const std::wstring &processName)
    {
        const std::wstring wanted = ToLower(processName);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            return 0;
        }

        PROCESSENTRY32W entry = {};
        entry.dwSize = sizeof(entry);

        if (!Process32FirstW(snapshot, &entry))
        {
            CloseHandle(snapshot);
            return 0;
        }

        do
        {
            if (ToLower(entry.szExeFile) == wanted)
            {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &entry));

        CloseHandle(snapshot);
        return 0;
    }

    bool ModuleMatchesDll(const MODULEENTRY32W &entry, const std::filesystem::path &dllPath)
    {
        const std::wstring wantedName = ToLower(dllPath.filename().wstring());
        const std::wstring moduleName = ToLower(entry.szModule);

        if (moduleName == wantedName)
        {
            return true;
        }

        const std::wstring wantedPath = ToLower(std::filesystem::absolute(dllPath).wstring());
        const std::wstring modulePath = ToLower(entry.szExePath);
        return !wantedPath.empty() && modulePath == wantedPath;
    }

    bool FindLoadedModule(DWORD pid, const std::filesystem::path &dllPath, MODULEENTRY32W &moduleEntry)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        MODULEENTRY32W entry = {};
        entry.dwSize = sizeof(entry);

        if (!Module32FirstW(snapshot, &entry))
        {
            CloseHandle(snapshot);
            return false;
        }

        do
        {
            if (ModuleMatchesDll(entry, dllPath))
            {
                moduleEntry = entry;
                CloseHandle(snapshot);
                return true;
            }
        } while (Module32NextW(snapshot, &entry));

        CloseHandle(snapshot);
        return false;
    }

    bool IsDllLoaded(DWORD pid, const std::filesystem::path &dllPath)
    {
        MODULEENTRY32W ignored = {};
        return FindLoadedModule(pid, dllPath, ignored);
    }

    #define TRY(x, e) if (!(x)) throw std::exception(e);

    bool InjectDll(HANDLE process, DWORD pid, const std::filesystem::path &dllPath)
    {
        const std::filesystem::path fullDllPath = std::filesystem::absolute(dllPath);
        if (!std::filesystem::exists(fullDllPath))
        {
            LOG(L"DLL does not exist: %s", fullDllPath.wstring().c_str());
            return false;
        }

        const std::wstring fullDllPathText = fullDllPath.wstring();
        const SIZE_T bytes = (fullDllPathText.size() + 1) * sizeof(wchar_t);
        void *remotePath = nullptr;
        HANDLE thread = nullptr;

        try
        {
            TRY(remotePath = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE),
                "VirtualAllocEx failed");

            TRY(WriteProcessMemory(process, remotePath, fullDllPathText.c_str(), bytes, nullptr),
                "WriteProcessMemory failed");

            const auto loadLibraryW = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
            TRY(loadLibraryW,
                "Could not resolve LoadLibraryW.");

            TRY(thread = CreateRemoteThread(process, nullptr, 0, loadLibraryW, remotePath, 0, nullptr),
                "CreateRemoteThread failed");

            const DWORD waitResult = WaitForSingleObject(thread, kRemoteThreadTimeoutMs);
            TRY(waitResult == WAIT_OBJECT_0,
                "LoadLibraryW remote thread did not finish.");

            CloseHandle(thread);
            thread = nullptr;

            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            remotePath = nullptr;

            if (!IsDllLoaded(pid, dllPath))
            {
                LOG(L"LoadLibraryW failed in target process. Check DLL dependencies and bitness.");
                return false;
            }

            return true;
        }

        catch (const std::exception &e)
        {
            LOG_ERROR(e.what());

            if (thread)  CloseHandle(thread);
            if (remotePath)  VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);

            return false;
        }
    }

#undef TRY

    bool IsStopRequested(HANDLE stopEvent)
    {
        return stopEvent && WaitForSingleObject(stopEvent, 0) == WAIT_OBJECT_0;
    }

    bool WaitForStop(HANDLE stopEvent, DWORD milliseconds)
    {
        return stopEvent && WaitForSingleObject(stopEvent, milliseconds) == WAIT_OBJECT_0;
    }

    int WatchAndInject(HANDLE stopEvent)
    {
        const std::filesystem::path dllPath = DefaultDllPath();

        AnyFSE::Tools::EnablePowerEfficencyMode(true);

        LOG(L"Watching for %s. Press Ctrl+C to stop in console mode.", kTargetProcessName);
        LOG(L"Loading DLL: %s", dllPath.wstring().c_str());

        for (;;)
        {
            if (IsStopRequested(stopEvent))
            {
                return 0;
            }

            const DWORD pid = FindProcessIdByName(kTargetProcessName);
            if (!pid)
            {
                if (WaitForStop(stopEvent, kMissingProcessDelayMs))
                {
                    return 0;
                }
                continue;
            }

            HANDLE process = OpenProcess(kTargetProcessAccess, FALSE, pid);
            if (!process)
            {
                LOG_ERROR("OpenProcess failed for PID %lu", pid);
                if (WaitForStop(stopEvent, kMissingProcessDelayMs))
                {
                    return 0;
                }
                continue;
            }

            LOG(L"Found %s process. PID: %lu", kTargetProcessName, pid);
            bool loaded = true;
            if (IsDllLoaded(pid, dllPath))
            {
                LOG(L"%s is already loaded in PID %lu.", kHookDllName, pid);
            }
            else
            {
                loaded = InjectDll(process, pid, dllPath);
                if (loaded)
                {
                    LOG(L"Injected successfully into PID %lu.", pid);
                }
                else
                {
                    LOG(L"Injection failed for PID %lu. Retrying later.", pid);
                }
            }

            if (!loaded)
            {
                CloseHandle(process);
                if (WaitForStop(stopEvent, kMissingProcessDelayMs))
                {
                    return 0;
                }
                continue;
            }

            LOG(L"Wait for %s exit or stop request.", kTargetProcessName);

            HANDLE waitHandles[] = {stopEvent, process};
            const DWORD waitResult = WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE);
            if (waitResult == WAIT_OBJECT_0)
            {
                LOG(L"Stop requested. Watcher exits.");
                CloseHandle(process);
                return 0;
            }
            else if (waitResult == WAIT_OBJECT_0 + 1)
            {
                LOG(L"%s exit detected. Reinject in %lu seconds.", kTargetProcessName, kMissingProcessDelayMs / 1000);
            }
            else
            {
                LOG_ERROR("WaitForMultipleObjects failed");
            }

            CloseHandle(process);

            if (WaitForStop(stopEvent, kMissingProcessDelayMs))
            {
                return 0;
            }
        }
    }

    void ReportServiceStatus(DWORD currentState, DWORD win32ExitCode = NO_ERROR, DWORD waitHint = 0)
    {
        if (!g_serviceStatusHandle)
        {
            return;
        }

        g_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        g_serviceStatus.dwCurrentState = currentState;
        g_serviceStatus.dwWin32ExitCode = win32ExitCode;
        g_serviceStatus.dwWaitHint = waitHint;
        g_serviceStatus.dwControlsAccepted = currentState == SERVICE_RUNNING
                                                 ? SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN
                                                 : 0;
        g_serviceStatus.dwCheckPoint = (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED)
                                           ? 0
                                           : g_serviceCheckpoint++;

        SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);
    }

    void WINAPI ServiceControlHandler(DWORD control)
    {
        switch (control)
        {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            g_shutdownRequested = control == SERVICE_CONTROL_SHUTDOWN;
            if (g_serviceStatus.dwCurrentState != SERVICE_RUNNING)
            {
                return;
            }

            ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, kServiceStopWaitHintMs);
            if (g_serviceStopEvent)
            {
                SetEvent(g_serviceStopEvent);
            }
            return;
        default:
            return;
        }
    }

    void WINAPI ServiceMain(DWORD, LPWSTR *)
    {
        g_serviceStatusHandle = RegisterServiceCtrlHandlerW(kServiceName, ServiceControlHandler);
        if (!g_serviceStatusHandle)
        {
            return;
        }

        ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

        g_serviceStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!g_serviceStopEvent)
        {
            ReportServiceStatus(SERVICE_STOPPED, GetLastError());
            return;
        }

        EnableDebugPrivilege();
        ReportServiceStatus(SERVICE_RUNNING);

        const int result = WatchAndInject(g_serviceStopEvent);
        bool restartedTargetService = true;
        if (!g_shutdownRequested)
        {
            ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, kServiceStopWaitHintMs);
            restartedTargetService = RestartTargetService();
        }

        CloseHandle(g_serviceStopEvent);
        g_serviceStopEvent = nullptr;

        ReportServiceStatus(
            SERVICE_STOPPED,
            result == 0 && restartedTargetService ? NO_ERROR : ERROR_SERVICE_SPECIFIC_ERROR);
    }

    int RunServiceDispatcher()
    {
        SERVICE_TABLE_ENTRYW serviceTable[] =
            {
                {const_cast<LPWSTR>(kServiceName), ServiceMain},
                {nullptr, nullptr},
            };

        if (!StartServiceCtrlDispatcherW(serviceTable))
        {
            LOG_ERROR("StartServiceCtrlDispatcherW failed");
            return 1;
        }

        return 0;
    }

    BOOL WINAPI ConsoleControlHandler(DWORD control)
    {
        switch (control)
        {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            if (g_consoleStopEvent)
            {
                SetEvent(g_consoleStopEvent);
                return TRUE;
            }
            break;
        default:
            break;
        }

        return FALSE;
    }

    int RunConsoleDebug()
    {
        EnableDebugPrivilege();

        g_consoleStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!g_consoleStopEvent)
        {
            LOG_ERROR("CreateEventW failed");
            return 1;
        }

        SetConsoleCtrlHandler(ConsoleControlHandler, TRUE);
        const int result = WatchAndInject(g_consoleStopEvent);
        SetConsoleCtrlHandler(ConsoleControlHandler, FALSE);

        CloseHandle(g_consoleStopEvent);
        g_consoleStopEvent = nullptr;
        return result;
    }

    bool WaitForServiceState(SC_HANDLE service, DWORD desiredState, DWORD timeoutMs)
    {
        const ULONGLONG deadline = GetTickCount64() + timeoutMs;

        for (;;)
        {
            SERVICE_STATUS_PROCESS status = {};
            DWORD bytesNeeded = 0;
            if (!QueryServiceStatusEx(
                    service,
                    SC_STATUS_PROCESS_INFO,
                    reinterpret_cast<LPBYTE>(&status),
                    sizeof(status),
                    &bytesNeeded))
            {
                LOG_ERROR("QueryServiceStatusEx failed");
                return false;
            }

            if (status.dwCurrentState == desiredState)
            {
                return true;
            }

            if (GetTickCount64() >= deadline)
            {
                return false;
            }

            Sleep(std::clamp(status.dwWaitHint / 10, 250UL, 1000UL));
        }
    }

    bool RestartTargetService()
    {
        LOG(L"Restarting %s service.", kTargetServiceName);

        SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!manager)
        {
            LOG_ERROR("OpenSCManagerW failed");
            return false;
        }

        SC_HANDLE service = OpenServiceW(
            manager,
            kTargetServiceName,
            SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!service)
        {
            LOG_ERROR("OpenServiceW failed for %ls", kTargetServiceName);
            CloseServiceHandle(manager);
            return false;
        }

        SERVICE_STATUS_PROCESS status = {};
        DWORD bytesNeeded = 0;
        if (!QueryServiceStatusEx(
                service,
                SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&status),
                sizeof(status),
                &bytesNeeded))
        {
            LOG_ERROR("QueryServiceStatusEx failed for %ls", kTargetServiceName);
            CloseServiceHandle(service);
            CloseServiceHandle(manager);
            return false;
        }

        if (status.dwCurrentState != SERVICE_STOPPED)
        {
            SERVICE_STATUS stopStatus = {};
            if (!ControlService(service, SERVICE_CONTROL_STOP, &stopStatus) && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
            {
                LOG(L"ControlService stop failed for %ls", kTargetServiceName);
                LOG_ERROR("Last error");
                CloseServiceHandle(service);
                CloseServiceHandle(manager);
                return false;
            }

            if (!WaitForServiceState(service, SERVICE_STOPPED, kTargetServiceRestartTimeoutMs))
            {
                LOG(L"%s did not stop in time.", kTargetServiceName);
                CloseServiceHandle(service);
                CloseServiceHandle(manager);
                return false;
            }
        }

        if (!StartServiceW(service, 0, nullptr) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
        {
            LOG_ERROR("StartServiceW failed for %ls", kTargetServiceName);
            CloseServiceHandle(service);
            CloseServiceHandle(manager);
            return false;
        }

        const bool running = WaitForServiceState(service, SERVICE_RUNNING, kTargetServiceRestartTimeoutMs);
        LOG(running ? L"ASUS Optimization restarted." : L"ASUS Optimization restart is still pending.");

        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return running;
    }

} // namespace

int RunFromArgs(int argc, wchar_t **argv)
{
    if (argc <= 1)
    {
        MessageBoxW(
            nullptr,
            L"This executable is intended to run as a Windows service.\nDo not run it directly without parameters.",
            L"AnyFSE ACSE Filter Injector",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    const std::wstring command = ToLower(argv[1]);
    if (command == L"--service")
    {
        return RunServiceDispatcher();
    }

    if (command == L"--debug" || command == L"--console")
    {
        return RunConsoleDebug();
    }

    LOG(L"Unknown command: %s", argv[1]);
    return 1;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
    {
        MessageBoxW(nullptr, L"Failed to parse command line.", L"AnyFSE ACSE Filter Injector", MB_OK | MB_ICONERROR);
        return 1;
    }

    const int result = RunFromArgs(argc, argv);
    LocalFree(argv);
    return result;
}
