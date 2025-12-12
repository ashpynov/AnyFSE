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

#include "ETWMonitor.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Unicode.hpp"
#include <tdh.h>
#include <iostream>
#include <sstream>
#include <string>

#pragma comment(lib, "advapi32.lib")
//#pragma comment(lib, "tdh.lib")

namespace AnyFSE::App::AppService
{

    static Logger log = LogManager::GetLogger("ETWMonitor/Parser");

    template<class T>
    PBYTE DataAt(T data, ULONG offset)
    {
        return ((PBYTE)data) + offset;
    }

    template<class T>
    uint32_t UInt32At(T data, ULONG offset)
    {
        return *(uint32_t*)(((PBYTE)data) + offset);
    }

    template<class T>
    uint32_t UInt8At(T data, ULONG offset)
    {
        return *(uint8_t*)(((PBYTE)data) + offset);
    }

    ULONG GetSidOffset(EVENT_RECORD *eventRecord)
    {
        bool isInt64 = eventRecord->EventHeader.Flags | EVENT_HEADER_FLAG_32_BIT_HEADER;
        ULONG version = eventRecord->EventHeader.EventDescriptor.Version;
        return version >=4 ? 48 : version >=3 ? 44 : 28;
    }

    ULONG SkipSID(EVENT_RECORD *eventRecord, ULONG offset)
    {
        // SID structure breakdown:
        // BYTE Revision; (1 byte)
        // BYTE SubAuthorityCount; (1 byte)
        // SID_IDENTIFIER_AUTHORITY IdentifierAuthority; (6 bytes)
        // DWORD SubAuthority[1]
        uint8_t revision = UInt8At(eventRecord->UserData, offset);  // should be 1
        uint8_t count = 1;                                          // UInt8At(eventRecord->UserData, offset);     // should be 1
        int result = offset + sizeof(uint32_t) + 8 + 4 * count;
        return result;
    }

    ULONG SkipUnicode(EVENT_RECORD *eventRecord, ULONG offset)
    {
        for (
            wchar_t * data = (wchar_t *)DataAt(eventRecord->UserData, offset);
            *data !=0;
            data++, offset++
        );

        return offset + 2;
    }

    ULONG GetImageNameOffset(EVENT_RECORD *eventRecord)
    {
        return SkipSID(eventRecord, GetSidOffset(eventRecord));
    }

    void ETWMonitor::ProcessEvent(EVENT_RECORD *eventRecord)
    {
        static size_t count = 0;
        count++;

        if (eventRecord->EventHeader.EventDescriptor.Opcode == 1
            && eventRecord->EventHeader.EventDescriptor.Task == 1
            && eventRecord->EventHeader.EventDescriptor.Id == 1
            /*&& IsEqualGUID(eventRecord->EventHeader.ProviderId, ProcessProviderGuid)*/)
        {
            HandleStartProcessEvent(eventRecord);
            #ifdef _TRACE
            // TraceEvent(eventRecord, count);
            #endif
        }
        else if (m_trackStop
            && eventRecord->EventHeader.EventDescriptor.Opcode == 2
            && eventRecord->EventHeader.EventDescriptor.Task == 2
            && eventRecord->EventHeader.EventDescriptor.Id == 2
            /*&& IsEqualGUID(eventRecord->EventHeader.ProviderId, ProcessProviderGuid)*/)
        {
            #ifdef _TRACE
            TraceEvent(eventRecord, count);
            #endif
            HandleStopProcessEvent(eventRecord);
        }
        else if (eventRecord->EventHeader.EventDescriptor.Opcode == 38  // QueryValueKey
            && eventRecord->EventHeader.EventDescriptor.Task == 0       //
            && eventRecord->EventHeader.EventDescriptor.Id == 7
            && IsEqualGUID(eventRecord->EventHeader.ProviderId, RegistryProviderGuid)
        )
        {
            HandleRegistryQueryValueEvent(eventRecord);
        }
        else {
            count++;

            #ifdef _TRACE
            // TraceEvent(eventRecord, count);
            #endif
        }

        return;
    }

    void ETWMonitor::HandleStartProcessEvent(EVENT_RECORD *eventRecord)
    {
        int offset = GetImageNameOffset(eventRecord);
        PWSTR processFullName = wCharAt(eventRecord->UserData, offset);

        processFullName = processFullName ? processFullName : L"";

        PWSTR pFileName = NULL;

        for (PWSTR p = processFullName; *p; p++) {
            if (*p == L'\\') {
                pFileName = p;
            }
        }

        pFileName = pFileName ? (pFileName + 1) : processFullName;

        if (_wcsicmp(pFileName, m_processName.c_str()) == 0)
        {
            LARGE_INTEGER timestamp = eventRecord->EventHeader.TimeStamp;
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

            // Calculate latency (rough approximation)
            LONGLONG latency = (currentTime.QuadPart - timestamp.QuadPart) / 10000; // Convert to milliseconds
            log.Debug("Process %s is detected, Latency: %lldms", Unicode::to_string(pFileName).c_str(), latency);
            OnProcessExecuted.Notify();
        }
        else if (_wcsicmp(pFileName, L"explorer.exe") ==0)
        {
            m_explorerProcessId = eventRecord->EventHeader.ProcessId;
            log.Debug("Explorer.exe was started with pid %d", m_explorerProcessId);
        }
    }

    bool ETWMonitor::memimem(const char * pData, size_t dataLen, const char * pSample, size_t sampleLen)
    {
        if ( dataLen < sampleLen || sampleLen < 2)
        {
            return false;
        }

        for (int pos = (int)(dataLen - sampleLen - 1); pos >= 0; pos--)
        {
            if ((pData[pos+sampleLen-1] & ~0x20) != (pSample[sampleLen-1] & ~0x20))
            {
                continue;
            }
            if (!_memicmp(pData+pos, pSample, sampleLen))
            {
                return true;
            }
        }
        return false;
    }

    void ETWMonitor::HandleStopProcessEvent(EVENT_RECORD *eventRecord)
    {
        char *pData = (char*)eventRecord->UserData;
        size_t len = eventRecord->UserDataLength;

        if ( memimem(pData, len, m_launcherProcName.c_str(), min(14, m_launcherProcName.size()))
            || memimem(pData, len, m_LauncherAltProcName.c_str(), min(14, m_LauncherAltProcName.size()))
        )
        {
            OnLauncherStopped.Notify();
        }
    }

    void ETWMonitor::HandleRegistryQueryValueEvent(EVENT_RECORD *eventRecord)
    {
        wchar_t * valueName = wCharAtSafe(eventRecord->UserData, SkipUnicode(eventRecord, 20));

        bool isGamingApp = (_wcsicmp(valueName, L"GamingHomeApp") == 0);
        bool isDeviceForm = !isGamingApp && (_wcsicmp(valueName, L"DeviceForm") == 0);
        bool isExplorer = (isGamingApp || isDeviceForm) && IsExploredPid(eventRecord->EventHeader.ProcessId);

        if (isGamingApp && isExplorer)
        {
            #ifdef _TRACE
            TRACE("Explorer has accessed to %s registry value", Unicode::to_string(valueName).c_str());
            TraceEvent(eventRecord, 0);
            #endif

            OnHomeAppTouched.Notify();
        }
        else if (isDeviceForm && isExplorer)
        {
            #ifdef _TRACE
            TRACE("Explorer has accessed to %s registry value", Unicode::to_string(valueName).c_str());
            TraceEvent(eventRecord, 0);
            #endif

            OnDeviceFormTouched.Notify();
        }
    }

}
