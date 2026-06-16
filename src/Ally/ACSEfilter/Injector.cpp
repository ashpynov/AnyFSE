#include <windows.h>
#include <tlhelp32.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <shellapi.h>
#include "../../App/AppConstants.hpp"

namespace
{

    constexpr const wchar_t *kServiceName = AnyFSE::AppConstants::InjectorServiceName;
    constexpr const wchar_t *kServiceDisplayName = L"AnyFSE ACSE Filter Injector";
    constexpr const wchar_t *kServiceDescription = L"Injects ACSEFilterHook into ASUS Optimization process and blocks it from ASUS-specific keys processing.";
    constexpr const wchar_t *kTargetProcessName = L"AsusOptimization.exe";
    constexpr const wchar_t *kTargetServiceName = L"ASUSOptimization";
    constexpr const wchar_t *kHookDllName = L"ACSEFilterHook.dll";
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

    std::wstring FormatError(DWORD error)
    {
        wchar_t *message = nullptr;
        const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        const DWORD length = FormatMessageW(flags, nullptr, error, 0, reinterpret_cast<LPWSTR>(&message), 0, nullptr);

        std::wstring result;
        if (length && message)
        {
            result.assign(message, length);
            while (!result.empty() && (result.back() == L'\r' || result.back() == L'\n'))
            {
                result.pop_back();
            }
        }
        else
        {
            result = L"error " + std::to_wstring(error);
        }

        if (message)
        {
            LocalFree(message);
        }

        return result;
    }

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

    std::wstring QuotePath(const std::filesystem::path &path)
    {
        return L"\"" + path.wstring() + L"\"";
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

    bool InjectDll(HANDLE process, DWORD pid, const std::filesystem::path &dllPath)
    {
        const std::filesystem::path fullDllPath = std::filesystem::absolute(dllPath);
        if (!std::filesystem::exists(fullDllPath))
        {
            std::wcerr << L"DLL does not exist: " << fullDllPath.wstring() << L"\n";
            return false;
        }

        const std::wstring fullDllPathText = fullDllPath.wstring();
        const SIZE_T bytes = (fullDllPathText.size() + 1) * sizeof(wchar_t);
        void *remotePath = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePath)
        {
            std::wcerr << L"VirtualAllocEx failed: " << FormatError(GetLastError()) << L"\n";
            return false;
        }

        if (!WriteProcessMemory(process, remotePath, fullDllPathText.c_str(), bytes, nullptr))
        {
            std::wcerr << L"WriteProcessMemory failed: " << FormatError(GetLastError()) << L"\n";
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            return false;
        }

        const auto loadLibraryW = reinterpret_cast<LPTHREAD_START_ROUTINE>(
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));

        if (!loadLibraryW)
        {
            std::wcerr << L"Could not resolve LoadLibraryW.\n";
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            return false;
        }

        HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibraryW, remotePath, 0, nullptr);
        if (!thread)
        {
            std::wcerr << L"CreateRemoteThread failed: " << FormatError(GetLastError()) << L"\n";
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            return false;
        }

        const DWORD waitResult = WaitForSingleObject(thread, kRemoteThreadTimeoutMs);
        if (waitResult != WAIT_OBJECT_0)
        {
            std::wcerr << L"LoadLibraryW remote thread did not finish: " << FormatError(GetLastError()) << L"\n";
            CloseHandle(thread);
            return false;
        }

        CloseHandle(thread);
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);

        if (!IsDllLoaded(pid, dllPath))
        {
            std::wcerr << L"LoadLibraryW failed in target process. Check DLL dependencies and bitness.\n";
            return false;
        }

        return true;
    }

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

        std::wcout << L"Watching for " << kTargetProcessName << L". Press Ctrl+C to stop in console mode.\n";
        std::wcout << L"Loading DLL: " << dllPath.wstring() << L"\n";

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
                std::wcerr << L"OpenProcess failed for PID " << pid << L": " << FormatError(GetLastError()) << L"\n";
                if (WaitForStop(stopEvent, kMissingProcessDelayMs))
                {
                    return 0;
                }
                continue;
            }

            std::wcout << L"Found " << kTargetProcessName << L" process. PID: " << pid << L"\n";
            bool loaded = true;
            if (IsDllLoaded(pid, dllPath))
            {
                std::wcout << kHookDllName << L" is already loaded in PID " << pid << L".\n";
            }
            else
            {
                loaded = InjectDll(process, pid, dllPath);
                if (loaded)
                {
                    std::wcout << L"Injected successfully into PID " << pid << L".\n";
                }
                else
                {
                    std::wcerr << L"Injection failed for PID " << pid << L". Retrying later.\n";
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

            std::wcout << L"Wait for " << kTargetProcessName << L" exit or stop request.\n";

            HANDLE waitHandles[] = {stopEvent, process};
            const DWORD waitResult = WaitForMultipleObjects(ARRAYSIZE(waitHandles), waitHandles, FALSE, INFINITE);
            if (waitResult == WAIT_OBJECT_0)
            {
                std::wcout << L"Stop requested. Watcher exits.\n";
                CloseHandle(process);
                return 0;
            }
            else if (waitResult == WAIT_OBJECT_0 + 1)
            {
                std::wcout << kTargetProcessName << L" exit detected. Reinject in  " << kMissingProcessDelayMs / 1000 << L" seconds.\n";
            }
            else
            {
                std::wcerr << L"WaitForMultipleObjects failed: " << FormatError(GetLastError()) << L"\n";
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
            std::wcerr << L"StartServiceCtrlDispatcherW failed: " << FormatError(GetLastError()) << L"\n";
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
            std::wcerr << L"CreateEventW failed: " << FormatError(GetLastError()) << L"\n";
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
                std::wcerr << L"QueryServiceStatusEx failed: " << FormatError(GetLastError()) << L"\n";
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
        std::wcout << L"Restarting " << kTargetServiceName << L" service.\n";

        SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!manager)
        {
            std::wcerr << L"OpenSCManagerW failed: " << FormatError(GetLastError()) << L"\n";
            return false;
        }

        SC_HANDLE service = OpenServiceW(
            manager,
            kTargetServiceName,
            SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!service)
        {
            std::wcerr << L"OpenServiceW failed for " << kTargetServiceName << L": " << FormatError(GetLastError()) << L"\n";
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
            std::wcerr << L"QueryServiceStatusEx failed for " << kTargetServiceName << L": " << FormatError(GetLastError()) << L"\n";
            CloseServiceHandle(service);
            CloseServiceHandle(manager);
            return false;
        }

        if (status.dwCurrentState != SERVICE_STOPPED)
        {
            SERVICE_STATUS stopStatus = {};
            if (!ControlService(service, SERVICE_CONTROL_STOP, &stopStatus) && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
            {
                std::wcerr << L"ControlService stop failed for " << kTargetServiceName << L": " << FormatError(GetLastError()) << L"\n";
                CloseServiceHandle(service);
                CloseServiceHandle(manager);
                return false;
            }

            if (!WaitForServiceState(service, SERVICE_STOPPED, kTargetServiceRestartTimeoutMs))
            {
                std::wcerr << kTargetServiceName << L" did not stop in time.\n";
                CloseServiceHandle(service);
                CloseServiceHandle(manager);
                return false;
            }
        }

        if (!StartServiceW(service, 0, nullptr) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
        {
            std::wcerr << L"StartServiceW failed for " << kTargetServiceName << L": " << FormatError(GetLastError()) << L"\n";
            CloseServiceHandle(service);
            CloseServiceHandle(manager);
            return false;
        }

        const bool running = WaitForServiceState(service, SERVICE_RUNNING, kTargetServiceRestartTimeoutMs);
        std::wcout << (running ? L"ASUS Optimization restarted.\n" : L"ASUS Optimization restart is still pending.\n");

        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return running;
    }

    int InstallService()
    {
        const std::filesystem::path exePath = CurrentExecutablePath();
        if (exePath.empty())
        {
            std::wcerr << L"Could not resolve executable path.\n";
            return 1;
        }

        const std::wstring binaryPath = QuotePath(exePath) + L" --service";
        SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
        if (!manager)
        {
            std::wcerr << L"OpenSCManagerW failed: " << FormatError(GetLastError()) << L"\n";
            return 1;
        }

        SC_HANDLE service = CreateServiceW(
            manager,
            kServiceName,
            kServiceDisplayName,
            SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS | SERVICE_START,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            binaryPath.c_str(),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr);

        if (!service)
        {
            const DWORD error = GetLastError();
            if (error == ERROR_SERVICE_EXISTS || error == ERROR_DUP_NAME)
            {
                std::wcout << L"Service " << kServiceName << L" is already installed.\n";
                CloseServiceHandle(manager);
                return 0;
            }

            std::wcerr << L"CreateServiceW failed: " << FormatError(error) << L"\n";
            CloseServiceHandle(manager);
            return 1;
        }

        SERVICE_DESCRIPTIONW description = {};
        description.lpDescription = const_cast<LPWSTR>(kServiceDescription);
        ChangeServiceConfig2W(service, SERVICE_CONFIG_DESCRIPTION, &description);

        std::wcout << L"Installed service " << kServiceName << L".\n";
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return StartInstalledService();
    }

    int StartInstalledService()
    {
        SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!manager)
        {
            std::wcerr << L"OpenSCManagerW failed: " << FormatError(GetLastError()) << L"\n";
            return 1;
        }

        SC_HANDLE service = OpenServiceW(manager, kServiceName, SERVICE_START | SERVICE_QUERY_STATUS);
        if (!service)
        {
            std::wcerr << L"OpenServiceW failed: " << FormatError(GetLastError()) << L"\n";
            CloseServiceHandle(manager);
            return 1;
        }

        if (!StartServiceW(service, 0, nullptr) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
        {
            std::wcerr << L"StartServiceW failed: " << FormatError(GetLastError()) << L"\n";
            CloseServiceHandle(service);
            CloseServiceHandle(manager);
            return 1;
        }

        const bool started = WaitForServiceState(service, SERVICE_RUNNING, 30000);
        std::wcout << (started ? L"Service started.\n" : L"Service start is still pending.\n");

        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return started ? 0 : 1;
    }

    int StopInstalledService()
    {
        SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!manager)
        {
            std::wcerr << L"OpenSCManagerW failed: " << FormatError(GetLastError()) << L"\n";
            return 1;
        }

        SC_HANDLE service = OpenServiceW(manager, kServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!service)
        {
            std::wcerr << L"OpenServiceW failed: " << FormatError(GetLastError()) << L"\n";
            CloseServiceHandle(manager);
            return 1;
        }

        SERVICE_STATUS_PROCESS status = {};
        DWORD bytesNeeded = 0;
        if (QueryServiceStatusEx(
                service,
                SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&status),
                sizeof(status),
                &bytesNeeded) &&
            status.dwCurrentState == SERVICE_STOPPED)
        {
            std::wcout << L"Service is already stopped.\n";
            CloseServiceHandle(service);
            CloseServiceHandle(manager);
            return 0;
        }

        SERVICE_STATUS serviceStatus = {};
        if (!ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus) && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
        {
            std::wcerr << L"ControlService stop failed: " << FormatError(GetLastError()) << L"\n";
            CloseServiceHandle(service);
            CloseServiceHandle(manager);
            return 1;
        }

        const bool stopped = WaitForServiceState(service, SERVICE_STOPPED, kServiceStopWaitHintMs + 5000);
        std::wcout << (stopped ? L"Service stopped.\n" : L"Service stop timed out.\n");

        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return stopped ? 0 : 1;
    }

    int UninstallService()
    {
        StopInstalledService();

        SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!manager)
        {
            std::wcerr << L"OpenSCManagerW failed: " << FormatError(GetLastError()) << L"\n";
            return 1;
        }

        SC_HANDLE service = OpenServiceW(manager, kServiceName, DELETE);
        if (!service)
        {
            const DWORD error = GetLastError();
            if (error == ERROR_SERVICE_DOES_NOT_EXIST)
            {
                std::wcout << L"Service " << kServiceName << L" is not installed.\n";
                CloseServiceHandle(manager);
                return 0;
            }

            std::wcerr << L"OpenServiceW failed: " << FormatError(error) << L"\n";
            CloseServiceHandle(manager);
            return 1;
        }

        if (!DeleteService(service))
        {
            std::wcerr << L"DeleteService failed: " << FormatError(GetLastError()) << L"\n";
            CloseServiceHandle(service);
            CloseServiceHandle(manager);
            return 1;
        }

        std::wcout << L"Uninstalled service " << kServiceName << L".\n";
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return 0;
    }

    int EnableService()
    {
        const int installResult = InstallService();
        if (installResult != 0)
        {
            return installResult;
        }

        return StartInstalledService();
    }

    int DisableService()
    {
        const int stopResult = StopInstalledService();
        if (stopResult != 0)
        {
            return stopResult;
        }

        return UninstallService();
    }

    void PrintUsage()
    {
        std::wcout
            << L"ACSEFilterInjector commands:\n"
            << L"  --debug, --console   Run foreground watcher for debugging.\n"
            << L"  --install            Install the Windows service.\n"
            << L"  --start              Start the installed service.\n"
            << L"  --stop               Stop the installed service and restart ASUS Optimization.\n"
            << L"  --uninstall          Stop and remove the Windows service.\n"
            << L"  --enable             Install service as autorun if needed and start it.\n"
            << L"  --disable            Stop service if running and remove it.\n"
            << L"  --service            Entry point used by the Service Control Manager.\n";
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

    if (command == L"--install")
    {
        return InstallService();
    }

    if (command == L"--start")
    {
        return StartInstalledService();
    }

    if (command == L"--stop")
    {
        return StopInstalledService();
    }

    if (command == L"--uninstall")
    {
        return UninstallService();
    }

    if (command == L"--enable")
    {
        return EnableService();
    }

    if (command == L"--disable")
    {
        return DisableService();
    }

    if (command == L"--help" || command == L"-h" || command == L"/?")
    {
        PrintUsage();
        return 0;
    }

    std::wcerr << L"Unknown command: " << argv[1] << L"\n";
    PrintUsage();
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
