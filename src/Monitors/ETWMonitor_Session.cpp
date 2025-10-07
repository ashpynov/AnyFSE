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

    static Logger log = LogManager::GetLogger("ETWMonitor/Session");
    
    // Process Provider GUID for real-time process monitoring
    // {22FB2CD6-0E7B-422B-A0C7-2FAD1FD0E716} - Microsoft-Windows-Kernel-Process
    const GUID ETWMonitor::ProcessProviderGuid =
        {0x22fb2cd6, 0x0e7b, 0x422b, {0xa0, 0xc7, 0x2f, 0xad, 0x1f, 0xd0, 0xe7, 0x16}};

    // Registry Provider GUID
    // {0x70eb4f03-c1de-4f73-a051-33d13d5413bd} - Microsoft-Windows-Kernel-Registry
    const GUID ETWMonitor::RegistryProviderGuid = 
        {0x70eb4f03, 0xc1de, 0x4f73, {0xa0, 0x51, 0x33, 0xd1, 0x3d, 0x54, 0x13, 0xbd}};
        
    void ETWMonitor::SetTraceProperties()
    {
        size_t bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + (m_sessionName.size() + 1) * sizeof(WCHAR)+ 4096;
        if (m_traceProperties == nullptr) 
        {            
            m_traceProperties = (EVENT_TRACE_PROPERTIES *)malloc(bufferSize);
            if (!m_traceProperties)
            {
                throw std::runtime_error("Failed to allocate memory for ETW properties");
            }
        }

        ZeroMemory(m_traceProperties, bufferSize);
        m_traceProperties->Wnode.BufferSize = (ULONG)bufferSize;
        m_traceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        m_traceProperties->Wnode.ClientContext = 1; // QoS
        m_traceProperties->Wnode.Guid = ProcessProviderGuid;
        m_traceProperties->EnableFlags = EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_THREAD | EVENT_TRACE_FLAG_IMAGE_LOAD;
        m_traceProperties->LogFileMode = EVENT_TRACE_NO_PER_PROCESSOR_BUFFERING | EVENT_TRACE_REAL_TIME_MODE;
        m_traceProperties->MaximumFileSize = 1; // 1MB
        m_traceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

        // optimization for low latency
        m_traceProperties->BufferSize = 1;      // Smallest possible buffers
        m_traceProperties->MinimumBuffers = 0;  // Minimum buffers
        m_traceProperties->MaximumBuffers = 40; // Prevent buffer buildup
        m_traceProperties->FlushTimer = 0;      // No timer flush (immediate)
        m_traceProperties->FlushThreshold = 0;

        wcscpy_s((WCHAR *)((BYTE *)m_traceProperties + m_traceProperties->LoggerNameOffset),
            m_sessionName.size() + 1, m_sessionName.c_str());
    }


    ULONG ETWMonitor::StopSession()
    {
         // Get the session handle
        if (ControlTraceW(0, m_sessionName.c_str(), m_traceProperties, EVENT_TRACE_CONTROL_QUERY)
            || ControlTraceW(m_traceProperties->Wnode.HistoricalContext, m_sessionName.c_str(), m_traceProperties, EVENT_TRACE_CONTROL_STOP)
        )
        {
            throw log.APIError("Failed to Stop ETW trace session: ");
        }
        return ERROR_SUCCESS;
    }

    ULONG ETWMonitor::StartSession()
    {
        ULONG status = StartTraceW(&m_sessionHandle, m_sessionName.c_str(), m_traceProperties);

        if (status == ERROR_ALREADY_EXISTS)
        {
            log.Info("Session is exist already, stopping");
            StopSession();
            // Get the session handle
            log.Info("Old session stopped, Start new session");
            SetTraceProperties();
            status = StartTraceW(&m_sessionHandle, m_sessionName.c_str(), m_traceProperties);
        }
        if (status)
        {
            free(m_traceProperties);
            m_traceProperties = nullptr;
            throw log.APIError("Failed to start ETW trace session: ");
        }
        return status;
    }

    ULONG ETWMonitor::EnableProcessProvider()
    {
        // Step 2: Enable the process provider on the session
        ENABLE_TRACE_PARAMETERS params = {0};
        params.SourceId = ProcessProviderGuid;
        params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
        params.EnableProperty = 0;
        params.ControlFlags = 0;

        if (EnableTraceEx2(
                m_sessionHandle, // Use session handle, not consumer handle
                &ProcessProviderGuid,
                //&SystemTraceControlGuid,
                EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                TRACE_LEVEL_INFORMATION,
                0x10, 0, INFINITE, &params
            )
        )
        {
            StopTrace(m_sessionHandle, m_sessionName.c_str(), m_traceProperties);
            free(m_traceProperties);
            m_traceProperties = nullptr;
            m_sessionHandle = NULL;
            throw log.APIError("Failed to enable process provider: ");
        }
        return ERROR_SUCCESS;
    }

    ULONG ETWMonitor::EnableRegistryProvider()
    {
        // Step 2: Enable the process provider on the session
        ENABLE_TRACE_PARAMETERS params = {0};
        params.SourceId = RegistryProviderGuid;
        params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
        params.EnableProperty = 0;
        params.ControlFlags = 0;

        if (EnableTraceEx2(
                m_sessionHandle, // Use session handle, not consumer handle
                &RegistryProviderGuid,
                //&SystemTraceControlGuid,
                EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                TRACE_LEVEL_VERBOSE,
                0x400, 0, INFINITE, &params
            )
        )
        {
            StopTrace(m_sessionHandle, m_sessionName.c_str(), m_traceProperties);
            free(m_traceProperties);
            m_traceProperties = nullptr;
            m_sessionHandle = NULL;
            throw log.APIError("Failed to enable registry provider: ");
        }
        return ERROR_SUCCESS;
    }

    ULONG ETWMonitor::OpenConsumer()
    {
        // Step 3: Open the trace for consumption
        ZeroMemory(&m_logFile, sizeof(m_logFile));
        m_logFile.LoggerName = (WCHAR *)m_sessionName.c_str();
        m_logFile.LogFileMode = EVENT_TRACE_PRIVATE_LOGGER_MODE  | EVENT_TRACE_REAL_TIME_MODE;
        m_logFile.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP | PROCESS_TRACE_MODE_REAL_TIME;
        m_logFile.EventRecordCallback = &ETWMonitor::EventRecordCallback;
        m_logFile.Context = this;

        m_consumerHandle = OpenTrace(&m_logFile);
        if (m_consumerHandle == INVALID_PROCESSTRACE_HANDLE)
        {
            StopTrace(m_sessionHandle, m_sessionName.c_str(), m_traceProperties);
            free(m_traceProperties);
            m_traceProperties = nullptr;
            m_sessionHandle = NULL;
            throw log.APIError(GetLastError(), "Failed to open ETW trace for consumption: Error: " );
        }
        return ERROR_SUCCESS;
    }
        void ETWMonitor::StartRealtimeETW()
    {
        SetTraceProperties();
        StartSession();
        EnableProcessProvider();
        EnableRegistryProvider();
        OpenConsumer();

        log.Info("Real-time ETW monitoring started successfully for process: %s", Tools::to_string(m_processName).c_str());
    }

    void ETWMonitor::StopRealtimeETW()
    {
        if (m_consumerHandle != NULL)
        {
            CloseTrace(m_consumerHandle);
            m_consumerHandle = NULL;
        }

        if (m_sessionHandle != NULL)
        {
            if (m_traceProperties)
            { 
                EnableTraceEx2(
                    m_sessionHandle,
                    &ProcessProviderGuid,
                    EVENT_CONTROL_CODE_DISABLE_PROVIDER,
                    TRACE_LEVEL_INFORMATION,
                    0x0, 0, 0, NULL);

                EnableTraceEx2(
                    m_sessionHandle,
                    &RegistryProviderGuid,
                    EVENT_CONTROL_CODE_DISABLE_PROVIDER,
                    TRACE_LEVEL_INFORMATION,
                    0x0, 0, 0, NULL);

                StopTrace(m_sessionHandle, m_sessionName.c_str(), m_traceProperties);
                free(m_traceProperties);
                m_traceProperties = nullptr;
            }
            m_sessionHandle = NULL;
        }

        log.Info("Real-time ETW monitoring stopped for process: %s", Tools::to_string(m_processName).c_str());
    }
} // namespace AnyFSE::Monitors
