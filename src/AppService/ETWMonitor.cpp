// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>

#include "ETWMonitor.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Process.hpp"
#include "Tools/Unicode.hpp"
#include <tdh.h>
#include <iostream>
#include <sstream>
#include <string>
#include <tlhelp32.h>

#pragma comment(lib, "advapi32.lib")

namespace AnyFSE::App::AppService
{

    static Logger log = LogManager::GetLogger("ETWMonitor");

    ETWMonitor::ETWMonitor(const std::wstring &processName)
        : m_isRunning(false),
          m_stopRequested(false),
          m_threadHandle(NULL),
          m_sessionHandle(NULL),
          m_consumerHandle(NULL),
          m_pTraceProperties(nullptr),
          m_explorerProcessId(Process::FindFirstByName(L"explorer.exe"))
    {
        ZeroMemory(&m_logFile, sizeof(m_logFile));
        m_processName = processName;
        m_sessionName = L"AnyFSE-Process-Monitor";
    }

    ETWMonitor::~ETWMonitor()
    {
        if (m_isRunning)
            Stop();
    }

    HANDLE ETWMonitor::Start()
    {
        if (m_isRunning)
        {
            log.Debug("Monitoring already running for process: %s", Unicode::to_string(m_processName).c_str());
            return m_threadHandle;
        }

        m_stopRequested = false;
        m_isRunning = true;

        m_monitoringThread = std::thread(&ETWMonitor::MonitoringThread, this);
        m_threadHandle = m_monitoringThread.native_handle();

        log.Debug("Started thread for monitoring for process: %s", Unicode::to_string(m_processName).c_str());
        return m_threadHandle;
    }

    DWORD ETWMonitor::IsExploredPid(DWORD processId)
    {
        if (processId == m_explorerProcessId)
        {
            return true;
        }

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
        if (!hProcess) return false;

        WCHAR processPath[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        std::wstring result;

        if (QueryFullProcessImageNameW(hProcess, 0, processPath, &size))
        {
            std::wstring fullPath = processPath;
            size_t lastBackslash = fullPath.find_last_of(L'\\');
            WCHAR * processName = processPath;
            if (lastBackslash != std::wstring::npos)
            {
                processName = processPath + (lastBackslash + 1);
            }
            if (_wcsicmp(processName, L"explorer.exe") == 0)
            {
                CloseHandle(hProcess);
                m_explorerProcessId = processId;
                return true;
            }

        }

        CloseHandle(hProcess);
        return false;

    }

    void ETWMonitor::MonitoringThread()
    {
        log.Debug("Starting real-time ETW monitoring for process: %s", Unicode::to_string(m_processName).c_str());

        try
        {
            StartRealtimeETW();

            // Main monitoring loop - ETW callbacks will handle events
            while (!m_stopRequested)
            {
                // ProcessTrace will block until events arrive or session stops
                log.Debug("Entering wait cycle for real-time ETW monitoring");
                ULONG status = ProcessTrace(&m_consumerHandle, 1, NULL, NULL);
                if (status != ERROR_SUCCESS && status != ERROR_CANCELLED)
                {
                    log.Debug(log.APIError(status), "ProcessTrace ended with status: %lu", status);
                    break;
                }
            }

            StopRealtimeETW();
        }
        catch (const std::exception &ex)
        {
            log.Error(ex, "Exception in monitoring thread for process: %s:", Unicode::to_string(m_processName).c_str());
            OnFailure.Notify();
        }

        m_isRunning = false;
        log.Debug("Real-time monitoring stopped for process: %s", Unicode::to_string(m_processName).c_str());
    }

    // Static callback function
    void WINAPI ETWMonitor::EventRecordCallback(EVENT_RECORD *eventRecord)
    {
        if (eventRecord && eventRecord->UserContext)
        {
            // Retrieve the ETWMonitor instance from UserContext
            ETWMonitor *monitor = static_cast<ETWMonitor *>(eventRecord->UserContext);
            monitor->ProcessEvent(eventRecord);
        }
    }

    void ETWMonitor::Stop()
    {

        if (!m_isRunning)
        {
            log.Debug("Stop called but monitoring is not running for process: %s", Unicode::to_string(m_processName).c_str());
            return;
        }

        log.Debug("Stopping monitoring for process: %s", Unicode::to_string(m_processName).c_str());
        m_stopRequested = true;

        StopRealtimeETW();

        if (m_monitoringThread.joinable())
        {
            m_monitoringThread.join();
        }

        m_isRunning = false;
        m_threadHandle = NULL;

        log.Debug("Monitoring successfully stopped for process: %s", Unicode::to_string(m_processName).c_str());
    }

    HANDLE ETWMonitor::StopAsync()
    {

        if (!m_isRunning)
        {
            log.Debug("StopAsync called but monitoring is not running for process: %s", Unicode::to_string(m_processName).c_str());
            return NULL;
        }

        log.Debug("Stopping monitoring asynchronously for process: %s", Unicode::to_string(m_processName).c_str());
        m_stopRequested = true;

        StopRealtimeETW();

        return m_threadHandle;
    }

}
