#pragma once
#include <windows.h>
#include <evntrace.h>
#include <string>
#include <thread>
#include <atomic>
#include "Event.h"
#include <tdh.h>

namespace AnyFSE::Monitors
{

    //return SkipSID(Version >= 4 ? HostOffset(28, 2) : (Version >= 3) ? HostOffset(24, 2) : HostOffset(20, 1));
#pragma pack(push, 1)
    typedef struct _ETW_PROCESS_START
    {
        ULONG _size;
        ULONG UniqueProcessKey;
        ULONG ProcessId;
        ULONG  ParentId;
        ULONG SessionId;
        LONG ExitStatus;
        ULONG DirectoryTableBase;
        ULONG sidSize;
        WCHAR UserSID[15];
        PWSTR ImageFileName;
        PWSTR CommandLine;
    } ETW_PROCESS_START;
#pragma pack(pop)


    // UNICODE_STRING structure
    typedef struct _UNICODE_STRING
    {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR Buffer;
    } UNICODE_STRING;

    class ETWMonitor
    {
    public:
        explicit ETWMonitor(const std::wstring &processName);
        ~ETWMonitor();

        HANDLE Run(bool &cancelToken);
        void Stop();
        HANDLE StopAsync();

        Event OnProcessExecuted;

    private:

    // ETW Process Start event structure for Microsoft-Windows-Kernel-Process

        void MonitoringThread(bool &cancelToken);
        void StartRealtimeETW();
        void StopRealtimeETW();
        static void WINAPI EventRecordCallback(EVENT_RECORD *eventRecord);
        void ParseWithTDH(PEVENT_RECORD pEvent);
        DWORD ExtractProperty(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, int type, LPCWSTR propertyName);
        LPWSTR GetStringPropertyByName(PEVENT_RECORD pEvent, LPCWSTR propertyName);
        void PrintEventSchema(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo);
        static VOID WINAPI FastProcessEventCallback(PEVENT_TRACE pEvent);
        void ProcessEvent(EVENT_RECORD *eventRecord);
        bool IsTargetProcess(const std::wstring &processName);
        void InitializeTargetUnicodeString(const std::wstring& processName);
        bool CompareProcessNames(const UNICODE_STRING* detectedName);

        bool EnableKernelProvider(ULONGLONG keywords);

        UNICODE_STRING m_targetProcessName;
        std::string m_processNameA;
        std::wstring m_sessionName;
        std::thread m_monitoringThread;
        std::atomic<bool> m_isRunning;
        std::atomic<bool> m_stopRequested;
        HANDLE m_threadHandle;

        // ETW real-time session handles
        TRACEHANDLE m_sessionHandle;    // From StartTrace
        TRACEHANDLE m_traceHandle;
        TRACEHANDLE m_consumerHandle;
        EVENT_TRACE_LOGFILEW m_logFile;
        EVENT_TRACE_PROPERTIES* m_traceProperties;
    };

} // namespace AnyFSE::Monitors