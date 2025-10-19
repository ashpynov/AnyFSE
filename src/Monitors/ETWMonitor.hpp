#pragma once
#include <windows.h>
#include <evntrace.h>
#include <string>
#include <thread>
#include <atomic>
#include "Tools/Event.hpp"
#include <tdh.h>

namespace AnyFSE::Monitors
{
    class ETWMonitor
    {
        static const GUID ProcessProviderGuid;
        static const GUID RegistryProviderGuid;

    public:
        explicit ETWMonitor(const std::wstring &processName);
        ~ETWMonitor();

        HANDLE Start();
        void Stop();
        HANDLE StopAsync();
        Event OnProcessExecuted;
        Event OnHomeAppTouched;
        Event OnDeviceFormTouched;
        Event OnFailure;

        ULONG EnableRegistryProvider();

    private:
        template <class T>
        PWCHAR wCharAt(T data, ULONG offset)
        {
            return (PWCHAR)((PBYTE)data + offset);
        }

        template <class T>
        PWCHAR wCharAtSafe(T data, ULONG offset)
        {
            return offset ? (PWCHAR)((PBYTE)data + offset) : L"";
        }

        void MonitoringThread();

        void SetTraceProperties();
        ULONG StopSession();
        ULONG StartSession();
        ULONG EnableProcessProvider();

        ULONG OpenConsumer();
        void StartRealtimeETW();
        void StopRealtimeETW();
        static void WINAPI EventRecordCallback(EVENT_RECORD *eventRecord);

        void TraceEvent(PEVENT_RECORD pEvent, size_t count);
        DWORD GetPropertySizeTDH(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, int type, LPCWSTR propertyName);
        void DumpEventTDH(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, size_t count);
        DWORD FindExploredPid();
        DWORD IsExploredPid(DWORD pid);
        DWORD GetExploredPid();
        bool IsExplorerProcessId(LONG pid);

        void ProcessEvent(EVENT_RECORD *eventRecord);

        std::wstring m_processName;
        std::wstring m_sessionName;
        std::thread m_monitoringThread;
        std::atomic<bool> m_isRunning;
        std::atomic<bool> m_stopRequested;
        HANDLE m_threadHandle;

        // ETW real-time session handles
        TRACEHANDLE m_sessionHandle;
        TRACEHANDLE m_traceHandle;
        TRACEHANDLE m_consumerHandle;
        EVENT_TRACE_LOGFILEW m_logFile;
        EVENT_TRACE_PROPERTIES *m_traceProperties;

        LONG m_explorerProcessId;
        void HandleStartProcessEvent(EVENT_RECORD *eventRecord);
        void HandleRegistryQueryValueEvent(EVENT_RECORD *eventRecord);
    };

} // namespace AnyFSE::Monitors