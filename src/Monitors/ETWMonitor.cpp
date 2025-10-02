#include "ETWMonitor.h"
#include "Logging/LogManager.h"
#include "Tools/Tools.h"
#include <tdh.h>
#include <iostream>
#include <sstream>
#include <string>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "tdh.lib")

namespace AnyFSE::Monitors
{

    static Logger log = LogManager::GetLogger("ETWMonitor");
    
    // Process Provider GUID for real-time process monitoring
    // {22FB2CD6-0E7B-422B-A0C7-2FAD1FD0E716} - Microsoft-Windows-Kernel-Process
    static const GUID ProcessProviderGuid =
        {0x22fb2cd6, 0x0e7b, 0x422b, {0xa0, 0xc7, 0x2f, 0xad, 0x1f, 0xd0, 0xe7, 0x16}};
    
    // SystemTraceControlGuid
    // {9e814aad-3204-11d2-9a82-006008a86939}
    static const GUID SystemTraceControlGuid = 
    { 0x9e814aad, 0x3204, 0x11d2, { 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39 } };
    
    // Kernel logger GUID for process events
    // {3d6fa8d1-fe05-11d0-9d6f-00c04fd8cba6}
    static const GUID ProcessEventsGuid = 
    { 0x3d6fa8d0, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };

    // internal static readonly Guid ProcessTaskGuid = new Guid(unchecked((int)0x3d6fa8d0), unchecked((short)0xfe05), unchecked((short)0x11d0), 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c);

    

    ETWMonitor::ETWMonitor(const std::wstring &processName)
        : m_isRunning(false),
          m_stopRequested(false),
          m_threadHandle(NULL),
          m_sessionHandle(NULL),
          m_consumerHandle(NULL),
          m_traceProperties(nullptr)
    {
        ZeroMemory(&m_logFile, sizeof(m_logFile));
        InitializeTargetUnicodeString(processName);
        m_processNameA = Tools::to_string(processName);

        // Generate unique session name
        m_sessionName = L"AnyFSE_ETW_Monitor_" + std::to_wstring(GetCurrentProcessId());
    }

    ETWMonitor::~ETWMonitor()
    {
        Stop();
    }

    void ETWMonitor::InitializeTargetUnicodeString(const std::wstring &processName)
    {
        // Initialize UNICODE_STRING
        m_targetProcessName.Length = static_cast<USHORT>(processName.size() * sizeof(WCHAR));
        m_targetProcessName.MaximumLength = m_targetProcessName.Length + sizeof(WCHAR);
        m_targetProcessName.Buffer = const_cast<WCHAR *>(processName.c_str());
    }

    bool ETWMonitor::CompareProcessNames(const UNICODE_STRING *detectedName)
    {

        const WCHAR *detectedBuffer = detectedName->Buffer;
        const WCHAR *detectedBufferEnd = detectedName->Buffer + (detectedName->Length / sizeof(WCHAR));

        const WCHAR *detectedFileName = detectedBuffer;
        for (const WCHAR *p = detectedBuffer; p < detectedBufferEnd && *p; ++p)
        {
            if (*p == L'\\')
            {
                detectedFileName = p + 1;
            }
        }

        if (detectedFileName == detectedBufferEnd)
        {
            detectedFileName = detectedBuffer;
        }

        const size_t charCount = m_targetProcessName.Length / sizeof(WCHAR);
        if (detectedBufferEnd - detectedFileName != charCount)
        {
            return false;
        }

        for (size_t i = 0; i < charCount; ++i, ++detectedFileName)
        {
            if (*detectedFileName != m_targetProcessName.Buffer[i])
            {
                return false;
            }
        }

        return true;
    }

    HANDLE ETWMonitor::Run(bool &cancelToken)
    {

        if (m_isRunning)
        {
            log.Info("Monitoring already running for process: %s", m_processNameA.c_str());
            return m_threadHandle;
        }

        m_stopRequested = false;
        m_isRunning = true;

        m_monitoringThread = std::thread(&ETWMonitor::MonitoringThread, this, std::ref(cancelToken));
        m_threadHandle = m_monitoringThread.native_handle();

        log.Info("Started monitoring for process: %s", m_processNameA.c_str());
        return m_threadHandle;
    }

    void ETWMonitor::MonitoringThread(bool &cancelToken)
    {
        log.Info("Starting real-time monitoring for process: %s", m_processNameA.c_str());

        try
        {
            StartRealtimeETW();

            // Main monitoring loop - ETW callbacks will handle events
            while (!cancelToken && !m_stopRequested)
            {
                // ProcessTrace will block until events arrive or session stops
                ULONG status = ProcessTrace(&m_consumerHandle, 1, NULL, NULL);
                if (status != ERROR_SUCCESS && status != ERROR_CANCELLED)
                {
                    log.Info(log.APIError(status), "ProcessTrace ended with status: %lu", status);
                    break;
                }

                // Small sleep to prevent tight loop if ProcessTrace returns immediately
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            StopRealtimeETW();
        }
        catch (const std::exception &ex)
        {
            log.Error(ex, "Exception in monitoring thread for process: %s:", m_processNameA.c_str());
        }

        m_isRunning = false;
        log.Info("Real-time monitoring stopped for process: %s", m_processNameA.c_str());
    }

    void ETWMonitor::StartRealtimeETW()
    {
        // Step 1: Create ETW session
        size_t bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + (m_sessionName.size() + 1) * sizeof(WCHAR)+256;
        m_traceProperties = (EVENT_TRACE_PROPERTIES *)malloc(bufferSize);
        if (!m_traceProperties)
        {
            throw std::runtime_error("Failed to allocate memory for ETW properties");
        }

        ZeroMemory(m_traceProperties, bufferSize);
        m_traceProperties->Wnode.BufferSize = (ULONG)bufferSize;
        m_traceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        m_traceProperties->Wnode.ClientContext = 1; // QoS
        m_traceProperties->Wnode.Guid = ProcessProviderGuid;
        m_traceProperties->EnableFlags = EVENT_TRACE_FLAG_PROCESS;
        m_traceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_SYSTEM_LOGGER_MODE | EVENT_TRACE_INDEPENDENT_SESSION_MODE;
        m_traceProperties->MaximumFileSize = 1; // 1MB
        m_traceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        m_traceProperties->FlushTimer = 1;


        ENABLE_TRACE_PARAMETERS params = {0};
        ULONG controlCode = EVENT_CONTROL_CODE_ENABLE_PROVIDER;
        
        // Set up the parameters structure
        params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
        //params.SourceId = 0; // Use 0 for session-wide enabling
        
        // Specify we only want process start events
        // Process start events are typically from the Microsoft-Windows-Kernel-Process provider
        // with keyword 0x10 for process start
        params.EnableProperty = EVENT_ENABLE_PROPERTY_IGNORE_KEYWORD_0;
        params.ControlFlags = 0;

        // Copy session name
        wcscpy_s((WCHAR *)((BYTE *)m_traceProperties + m_traceProperties->LoggerNameOffset),
                 m_sessionName.size() + 1, m_sessionName.c_str());
                 
        // Create the trace session
        ULONG status = StartTraceW(&m_sessionHandle, m_sessionName.c_str(), m_traceProperties);
        if (status != ERROR_SUCCESS)
        {
            free(m_traceProperties);
            m_traceProperties = nullptr;
            throw log.APIError("Failed to start ETW trace session: ");
        }

        // Step 2: Enable the process provider on the session
        status = EnableTraceEx2(
            m_sessionHandle, // Use session handle, not consumer handle
            &ProcessProviderGuid,
            //&SystemTraceControlGuid,
            EVENT_CONTROL_CODE_ENABLE_PROVIDER,
            TRACE_LEVEL_INFORMATION,
            0x10, 0, 0, &params);

        if (status != ERROR_SUCCESS)
        {
            StopTrace(m_sessionHandle, m_sessionName.c_str(), m_traceProperties);
            free(m_traceProperties);
            m_traceProperties = nullptr;
            m_sessionHandle = NULL;
            throw log.APIError("Failed to enable process provider: ");
        }

        // Step 3: Open the trace for consumption
        ZeroMemory(&m_logFile, sizeof(m_logFile));
        m_logFile.LoggerName = (WCHAR *)m_sessionName.c_str();
        m_logFile.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP | PROCESS_TRACE_MODE_REAL_TIME;
        m_logFile.EventRecordCallback = &ETWMonitor::EventRecordCallback;
        //m_logFile.EventCallback = &ETWMonitor::FastProcessEventCallback;
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

        log.Info("Real-time ETW monitoring started successfully for process: %s", m_processNameA.c_str());
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
            { // Disable provider first
                EnableTraceEx2(
                    m_sessionHandle,
                    &ProcessProviderGuid,
                    EVENT_CONTROL_CODE_DISABLE_PROVIDER,
                    TRACE_LEVEL_INFORMATION,
                    0, 0, 0, NULL);

                StopTrace(m_sessionHandle, m_sessionName.c_str(), m_traceProperties);
                free(m_traceProperties);
                m_traceProperties = nullptr;
            }
            m_sessionHandle = NULL;
        }

        log.Info("Real-time ETW monitoring stopped for process: %s", m_processNameA.c_str());
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

void ETWMonitor::ParseWithTDH(PEVENT_RECORD pEvent) {
        DWORD status = ERROR_SUCCESS;
        DWORD bufferSize = 0;
        PTRACE_EVENT_INFO pInfo = NULL;
        
        // Get required buffer size
        status = TdhGetEventInformation(pEvent, 0, NULL, NULL, &bufferSize);
        if (status == ERROR_INSUFFICIENT_BUFFER) {
            pInfo = (PTRACE_EVENT_INFO)malloc(bufferSize);
            status = TdhGetEventInformation(pEvent, 0, NULL, pInfo, &bufferSize);
        }
        
        if (status != ERROR_SUCCESS) {
            if (pInfo) free(pInfo);
            return;
        }
        
        PrintEventSchema(pEvent, pInfo);       
        free(pInfo);
    }
    
    DWORD ETWMonitor::ExtractProperty(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, int type, LPCWSTR propertyName) {
        PROPERTY_DATA_DESCRIPTOR descriptors[1];
        DWORD propertySize = 0;
        DWORD status;
        
        descriptors[0].PropertyName = (ULONGLONG)propertyName;
        descriptors[0].ArrayIndex = ULONG_MAX;
        
        status = TdhGetPropertySize(pEvent, 0, NULL, 1, descriptors, &propertySize);
        if (status == ERROR_SUCCESS) {
            PBYTE pData = (PBYTE)malloc(propertySize);
            if (pData) {
                status = TdhGetProperty(pEvent, 0, NULL, 1, descriptors, propertySize, pData);
                if (status == ERROR_SUCCESS) {

                    //else
                    {
                        printf(">> %ld bytes.\n", propertySize);
                    }
                }
                free(pData);
            }
        }
        return propertySize;
    }

    void ETWMonitor::PrintEventSchema(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo) {

        std::stringstream ss;
        wprintf(L"\n[AnyFSE/ETWMonitor]      | === Event Schema ===\n");
        
        // Provider name
        if (pInfo->ProviderNameOffset > 0) {
            wprintf(L"[AnyFSE/ETWMonitor]      | Provider Name: %s\n", (PWCHAR)((PBYTE)pInfo + pInfo->ProviderNameOffset));
        }
        
        // Event name
        if (pInfo->EventNameOffset > 0) {
            wprintf(L"[AnyFSE/ETWMonitor]      | Event Name: %s\n", (PWCHAR)((PBYTE)pInfo + pInfo->EventNameOffset));
        }
        
        ss << "Event ID: " << pInfo->EventDescriptor.Id << std::endl;
        ss << "Opcode: " << (int)pInfo->EventDescriptor.Opcode << std::endl;
        ss << "Level: " << (int)pInfo->EventDescriptor.Level << std::endl;
        ss << "Task: " << pInfo->TaskNameOffset << std::endl;
        log.Info("Event: %s", ss.str().c_str());
        // Print all properties
        DWORD offset = 0;
         for (DWORD i = 0; i < pInfo->TopLevelPropertyCount; i++) {
            PEVENT_PROPERTY_INFO prop = &pInfo->EventPropertyInfoArray[i];
            PWSTR propName = (PWSTR)((PBYTE)pInfo + prop->NameOffset);

            wprintf(L"[AnyFSE/ETWMonitor]      |[%ld]|Property %d: %s  %d %d ",
                    offset, i, propName,
                    prop->nonStructType.InType,
                    prop->nonStructType.OutType

            );
            int type = prop->nonStructType.InType;
            if (type == TDH_INTYPE_UNICODESTRING || type == TDH_INTYPE_ANSISTRING ) {
                PBYTE pData = ((PBYTE)pEvent->UserData) + offset;
                printf(" >> %s ", type == TDH_INTYPE_UNICODESTRING ? Tools::to_string((LPWSTR)pData).c_str() : (LPSTR)pData);
            }
            offset += ExtractProperty(pEvent, pInfo, prop->nonStructType.InType, propName);

        }
        fflush(stdout);
        
    }
    

    VOID WINAPI ETWMonitor::FastProcessEventCallback(PEVENT_TRACE pEvent)
    {
        if (!pEvent )
            return;

        if (!IsEqualGUID(pEvent->Header.Guid, ProcessEventsGuid))
        {
           return;
        }

        //if (pEvent->Header.Class.Type == 80 || pEvent->Header.Class.Type == 64 )
        //return;
static int a = 1;
        printf("%d event: Level = %u, Type = %u, Version = %u\n", a++, pEvent->Header.Class.Level, pEvent->Header.Class.Type, pEvent->Header.Class.Version );
        //if (pEvent->Header.Class.Version < 4)
        return;


        // Fast GUID comparison
        
        PBYTE eventData = (PBYTE)pEvent->MofData;

        return;
        // Ultra-fast parsing - just extract essential fields
        ULONG processId = *(PULONG)eventData;
        ULONG parentPid = *(PULONG)(eventData + 4);
        ULONG sessionId = *(PULONG)(eventData + 8);

        // Get image name (located after the fixed fields)
        // The offset varies by Windows version, but ~0x58 bytes for Win10
        PCHAR imageName = NULL;

        // Try different known offsets for image name
        if (pEvent->MofLength >= 0x60)
        {
            imageName = (PCHAR)(eventData + 0x58); // Common Win10 offset
        }
        else if (pEvent->MofLength >= 0x20)
        {
            imageName = (PCHAR)(eventData + 0x1C); // Older Windows
        }

        // Validate and print image name
        if (imageName && *imageName)
        {
            // Simple ASCII validation
            BOOL valid = TRUE;
            PCHAR name = imageName;
            for (int i = 0; i < 16 && *name; i++)
            {
                if (*name < 0x20 || *name > 0x7E)
                {
                    valid = FALSE;
                    break;
                }
                name++;
            }

            if (valid)
            {
                wprintf(L"[%lu] PID: %lu, Parent: %lu, Image: %hs\n",
                        pEvent->Header.ProcessId, processId, parentPid, imageName);
            }
            else
            {
                wprintf(L"[%lu] PID: %lu, Parent: %lu\n",
                        pEvent->Header.ProcessId, processId, parentPid);
            }
        }
        else
        {
            wprintf(L"[%lu] PID: %lu, Parent: %lu\n",
                    pEvent->Header.ProcessId, processId, parentPid);
        }
    }

    void ETWMonitor::ProcessEvent(EVENT_RECORD *eventRecord)
    {
        static size_t count = 0;
        count++;
             
        if (eventRecord->EventHeader.EventDescriptor.Opcode != 1
            //|| eventRecord->EventHeader.EventDescriptor.Id != 1
            || !IsEqualGUID(eventRecord->EventHeader.ProviderId, ProcessProviderGuid)
        )
        {
            return;
        }

        /// internal static readonly Guid ProcessTaskGuid = new Guid(unchecked((int)0x3d6fa8d0), unchecked((short)0xfe05), unchecked((short)0x11d0), 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c);

        UCHAR version = eventRecord->EventHeader.EventDescriptor.Version;
        // public string KernelImageFileName { get { if (Version >= 1) { return GetUTF8StringAt(GetKernelImageNameOffset()); } return ""; } }
        // return SkipSID(Version >= 4 ? HostOffset(28, 2) : (Version >= 3) ? HostOffset(24, 2) : HostOffset(20, 1));

        //  internal int SkipSID(int offset)
        // {
        //     IntPtr mofData = DataStart;
        //     // This is a Security Token.  Either it is null, which takes 4 bytes, 
        //     // Otherwise it is an 8 byte structure (TOKEN_USER) followed by SID, which is variable
        //     // size (sigh) depending on the 2nd byte in the SID
        //     int sid = TraceEventRawReaders.ReadInt32(mofData, offset);
        //     if (sid == 0)
        //     {
        //         return offset + 4;      // TODO confirm 
        //     }
        //     else
        //     {
        //         int tokenSize = HostOffset(8, 2);
        //         int numAuthorities = TraceEventRawReaders.ReadByte(mofData, offset + (tokenSize + 1));
        //         return offset + tokenSize + 8 + 4 * numAuthorities;
        //     }
        // }

        // [EventType{1, 2, 3, 4}, EventTypeName{"Start", "End", "DCStart", "DCEnd"}]
        // class Process_V0_TypeGroup1 : Process_V0
        // {
        // uint32 ProcessId;
        // uint32 ParentId;
        // object UserSID;
        // string ImageFileName;
        // };


        // v0 = sizeof(uint32) * 2  + sizeof(sid)
        // v1 = sizeof(uint32) * 5  + sizeof(sid)
        // v2 = sizeof(uint32) * 5  + sizeof(sid)

        /*
        internal int SkipSID(int offset)
        {
            IntPtr mofData = DataStart;
            // This is a Security Token.  Either it is null, which takes 4 bytes, 
            // Otherwise it is an 8 byte structure (TOKEN_USER) followed by SID, which is variable
            // size (sigh) depending on the 2nd byte in the SID
            int sid = TraceEventRawReaders.ReadInt32(mofData, offset);
            if (sid == 0)
            {
                return offset + 4;      // TODO confirm 
            }
            else
            {
                int tokenSize = HostOffset(8, 2);
                int numAuthorities = TraceEventRawReaders.ReadByte(mofData, offset + (tokenSize + 1));
                return offset + tokenSize + 8 + 4 * numAuthorities;
            }
        }
        */
        printf ("event number: %llu\n", count);
        ParseWithTDH(eventRecord);

        size_t sidOffset = version >= 4 ? 2 : version >=3 ? 24 : 20;



        char * pOffset = (char*)(eventRecord->UserData);
        
        return;
        // Direct structure mapping
        const ETW_PROCESS_START *processStart = reinterpret_cast<const ETW_PROCESS_START *>(eventRecord->UserData);

        return;
        if (!processStart->ImageFileName)
        {
            return;
        }

        const UNICODE_STRING *detectedName = reinterpret_cast<const UNICODE_STRING *>(processStart->ImageFileName);

        if (!detectedName->Buffer || detectedName->Length == 0)
        {
            return;
        }

        // Fast comparison using UNICODE_STRING structures
        if (CompareProcessNames(detectedName))
        {
            log.Info("Target process executed: (PID: %lu)", processStart->ProcessId);
            OnProcessExecuted.Notify();
        }
    }

    void ETWMonitor::Stop()
    {

        if (!m_isRunning)
        {
            log.Info("Stop called but monitoring is not running for process: %s", m_processNameA.c_str());
            return;
        }

        log.Info("Stopping monitoring for process: %s", m_processNameA.c_str());
        m_stopRequested = true;

        StopRealtimeETW();

        if (m_monitoringThread.joinable())
        {
            m_monitoringThread.join();
        }

        m_isRunning = false;
        m_threadHandle = NULL;

        log.Info("Monitoring successfully stopped for process: %s", m_processNameA.c_str());
    }

    HANDLE ETWMonitor::StopAsync()
    {

        if (!m_isRunning)
        {
            log.Info("StopAsync called but monitoring is not running for process: %s", m_processNameA.c_str());
            return NULL;
        }

        log.Info("Stopping monitoring asynchronously for process: %s", m_processNameA.c_str());
        m_stopRequested = true;

        StopRealtimeETW();

        return m_threadHandle;
    }

} // namespace AnyFSE::Monitors
