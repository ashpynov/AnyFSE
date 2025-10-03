#include "ETWMonitor.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Tools.hpp"
#include <tdh.h>
#include <iostream>
#include <sstream>
#include <string>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "tdh.lib")

namespace AnyFSE::Monitors
{

    static Logger log = LogManager::GetLogger("ETWMonitor_Parser");  
   
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
        uint8_t count = UInt8At(eventRecord->UserData, offset);     // should be 1
        return offset + sizeof(uint32_t) + 8 + 4 * count;                               
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
            int offset = GetImageNameOffset(eventRecord);
            PWSTR processFullName = wCharAt(eventRecord->UserData, offset);
            
            PWSTR pFileName = NULL;
                
            for (PWSTR p = processFullName; *p; p++) {
                if (*p == L'\\') {
                    pFileName = p;
                }
            }
        
            pFileName = pFileName ? (pFileName + 1) : processFullName;

            //log.Info("Process filename = %s", Tools::to_string(pFileName).c_str());
            if (std::wstring(pFileName)==m_processName)
            {
                LARGE_INTEGER timestamp = eventRecord->EventHeader.TimeStamp;
                LARGE_INTEGER currentTime;
                QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
            
                // Calculate latency (rough approximation)
                LONGLONG latency = (currentTime.QuadPart - timestamp.QuadPart) / 10000; // Convert to milliseconds
                log.Info("Process %s is detected, Latency: %lldms", Tools::to_string(pFileName).c_str(), latency);
                OnProcessExecuted.Notify();
            }                
        } 

        return;
    }
} // namespace AnyFSE::Monitors
