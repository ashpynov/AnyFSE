// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>

#ifdef _TRACE

#include "ETWMonitor.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Unicode.hpp"
#include <tdh.h>
#include <iostream>
#include <sstream>
#include <string>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "tdh.lib")

namespace AnyFSE::App::AppService
{
    static Logger log = LogManager::GetLogger("ETWMonitor/Dump");

    void ETWMonitor::TraceEvent(PEVENT_RECORD pEvent, size_t count) {
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

        DumpEventTDH(pEvent, pInfo, count);
        free(pInfo);
    }

    DWORD ETWMonitor::GetPropertySizeTDH(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, int type, LPCWSTR propertyName)
    {
        PROPERTY_DATA_DESCRIPTOR descriptors[1];
        DWORD propertySize = 0;
        DWORD status;

        descriptors[0].PropertyName = (ULONGLONG)propertyName;
        descriptors[0].ArrayIndex = ULONG_MAX;

        status = TdhGetPropertySize(pEvent, 0, NULL, 1, descriptors, &propertySize);
        return propertySize;
    }

    void ETWMonitor::DumpEventTDH(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, size_t count)
    {
        std::wstringstream ss;
        static size_t lastCount = 0;
        ss << L"=== Event #" << count << L" ( " << (count - lastCount) << L"  skipped) ===" << std::endl;
        lastCount = count;

        LARGE_INTEGER timestamp = pEvent->EventHeader.TimeStamp;
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

        // Calculate latency (rough approximation)
        LONGLONG latency = (currentTime.QuadPart - timestamp.QuadPart) / 10000; // Convert to milliseconds

        ss << L"Provider: "    << wCharAtSafe(pInfo, pInfo->ProviderNameOffset) << std::endl;
        ss << L"Event: "       << wCharAtSafe(pInfo, pInfo->EventNameOffset)    << L" [" << pInfo->EventDescriptor.Id     << L"]" << std::endl;
        ss << L"Task: "        << wCharAtSafe(pInfo, pInfo->TaskNameOffset)     << L" [" << pInfo->EventDescriptor.Task   << L"]" << std::endl;
        ss << L"Opcode: "      << wCharAtSafe(pInfo, pInfo->OpcodeNameOffset)   << L" [" << pInfo->EventDescriptor.Opcode << L"]" << std::endl;
        ss << L"Level: "       << (int)pInfo->EventDescriptor.Level             << std::endl;
        ss << L"Version: "     << (int)pInfo->EventDescriptor.Version           << std::endl;
        ss << L"Latency: "     << latency << "ms"  << std::endl;

        // dump all properties
        for (DWORD index = 0, offset=0; index < pInfo->TopLevelPropertyCount; index++) {
            PEVENT_PROPERTY_INFO prop = &pInfo->EventPropertyInfoArray[index];

            ss << L" +" << offset << L" #" << index << L": " << wCharAt(pInfo,  prop->NameOffset);

            switch (int type = prop->nonStructType.InType)
            {
                case TDH_INTYPE_UNICODESTRING:
                    ss <<  L" <UNICODE " << type << L">"
                        << L" :" << wCharAt(pEvent->UserData, offset) << std::endl;
                break;

                case TDH_INTYPE_ANSISTRING:
                    ss <<  L" <ANSI " << type << L">"
                        << L" :" << Unicode::to_wstring(std::string((PCHAR)(((PBYTE)pEvent->UserData) + offset))) << std::endl;
                break;

                case TDH_INTYPE_SID:
                    ss << L" <" << type << L">"  << std::endl;
                break;

                default:
                    ss << L" <" << type << L">"  << std::endl;
                break;
            }
            offset += GetPropertySizeTDH(pEvent, pInfo, prop->nonStructType.InType, wCharAt(pInfo,  prop->NameOffset));
        }

        log.Trace("\n%s\n", Unicode::to_string(ss.str()).c_str());
    }

}
#endif