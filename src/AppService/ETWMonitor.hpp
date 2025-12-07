// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include <windows.h>
#include <evntrace.h>
#include <string>
#include <thread>
#include <atomic>
#include "Tools/Event.hpp"
#include <tdh.h>

namespace AnyFSE::App::AppService
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
        Event OnLauncherStopped;
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
        void EnableMonitoringExitLauncher();
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

        bool m_trackStop;
        std::string m_launcherProcName;
        std::string m_LauncherAltProcName;

        std::atomic<bool> m_isRunning;
        std::atomic<bool> m_stopRequested;
        HANDLE m_threadHandle;

        // ETW real-time session handles
        TRACEHANDLE m_sessionHandle;
        TRACEHANDLE m_traceHandle;
        TRACEHANDLE m_consumerHandle;
        EVENT_TRACE_LOGFILEW m_logFile;
        EVENT_TRACE_PROPERTIES *m_pTraceProperties;

        LONG m_explorerProcessId;
        void HandleStartProcessEvent(EVENT_RECORD *eventRecord);
        void HandleStopProcessEvent(EVENT_RECORD *eventRecord);
        void HandleRegistryQueryValueEvent(EVENT_RECORD *eventRecord);
        static bool memimem(const char *pData, size_t dataLen, const char *pSample, size_t sampleLen);
    };

}