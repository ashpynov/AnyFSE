#pragma once

#if 1

#include <windows.h>
#include <strsafe.h>

#include <cstdarg>

namespace ACSEFilter::Debug
{

    inline void Log(const wchar_t *format, ...)
    {
        wchar_t output[560] = {};
        wchar_t *messageStart = nullptr;
        size_t messageCapacity = 0;

        StringCchCopyExW(
            output,
            ARRAYSIZE(output),
            L"[ACSEFilter] ",
            &messageStart,
            &messageCapacity,
            0);

        if (format && messageStart && messageCapacity)
        {
            va_list args;
            va_start(args, format);
            const HRESULT result = StringCchVPrintfW(messageStart, messageCapacity, format, args);
            va_end(args);

            if (FAILED(result) && result != STRSAFE_E_INSUFFICIENT_BUFFER)
            {
                StringCchCopyW(messageStart, messageCapacity, L"<log formatting failed>");
            }
        }

        StringCchCatW(output, ARRAYSIZE(output), L"\n");
        OutputDebugStringW(output);
    }

} // namespace ACSEFilter::Debug

#define DEBUG(...) ::ACSEFilter::Debug::Log(__VA_ARGS__)

#else

#define DEBUG(...) ((void)0)

#endif
