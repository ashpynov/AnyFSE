#include "ETWMonitor.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Tools.hpp"
#include <tdh.h>
#include <iostream>
#include <sstream>
#include <string>

#pragma comment(lib, "advapi32.lib")

namespace AnyFSE::Monitors
{

    static Logger log = LogManager::GetLogger("ETWMonitor");
    
    ETWMonitor::ETWMonitor(const std::wstring &processName)
        : m_isRunning(false),
          m_stopRequested(false),
          m_threadHandle(NULL),
          m_sessionHandle(NULL),
          m_consumerHandle(NULL),
          m_traceProperties(nullptr)
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

    HANDLE ETWMonitor::Run(bool &cancelToken)
    {

        if (m_isRunning)
        {
            log.Info("Monitoring already running for process: %s", Tools::to_string(m_processName).c_str());
            return m_threadHandle;
        }

        m_stopRequested = false;
        m_isRunning = true;

        m_monitoringThread = std::thread(&ETWMonitor::MonitoringThread, this, std::ref(cancelToken));
        m_threadHandle = m_monitoringThread.native_handle();

        log.Info("Started thread for monitoring for process: %s", Tools::to_string(m_processName).c_str());
        return m_threadHandle;
    }

    void ETWMonitor::MonitoringThread(bool &cancelToken)
    {
        log.Info("Starting real-time ETW monitoring for process: %s", Tools::to_string(m_processName).c_str());

        try
        {
            StartRealtimeETW();

            // Main monitoring loop - ETW callbacks will handle events
            while (!cancelToken && !m_stopRequested)
            {
                // ProcessTrace will block until events arrive or session stops
                log.Info("Entering wait cycle for real-time ETW monitoring");
                ULONG status = ProcessTrace(&m_consumerHandle, 1, NULL, NULL);
                if (status != ERROR_SUCCESS && status != ERROR_CANCELLED)
                {
                    log.Info(log.APIError(status), "ProcessTrace ended with status: %lu", status);
                    break;
                }
            }

            StopRealtimeETW();
        }
        catch (const std::exception &ex)
        {
            log.Error(ex, "Exception in monitoring thread for process: %s:", Tools::to_string(m_processName).c_str());
            OnFailure.Notify();
        }

        m_isRunning = false;
        log.Info("Real-time monitoring stopped for process: %s", Tools::to_string(m_processName).c_str());
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
            log.Info("Stop called but monitoring is not running for process: %s", Tools::to_string(m_processName).c_str());
            return;
        }

        log.Info("Stopping monitoring for process: %s", Tools::to_string(m_processName).c_str());
        m_stopRequested = true;

        StopRealtimeETW();

        if (m_monitoringThread.joinable())
        {
            m_monitoringThread.join();
        }

        m_isRunning = false;
        m_threadHandle = NULL;

        log.Info("Monitoring successfully stopped for process: %s", Tools::to_string(m_processName).c_str());
    }

    HANDLE ETWMonitor::StopAsync()
    {

        if (!m_isRunning)
        {
            log.Info("StopAsync called but monitoring is not running for process: %s", Tools::to_string(m_processName).c_str());
            return NULL;
        }

        log.Info("Stopping monitoring asynchronously for process: %s", Tools::to_string(m_processName).c_str());
        m_stopRequested = true;

        StopRealtimeETW();

        return m_threadHandle;
    }

} // namespace AnyFSE::Monitors
