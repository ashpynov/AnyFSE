#pragma once

#if 1

#include <windows.h>
#include <strsafe.h>

#include <cstdarg>
#include <string>

namespace ACSEFilter::Debug
{
    inline std::string FormatError(DWORD error)
    {
        char *message = nullptr;
        const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        const DWORD length = FormatMessageA(flags, nullptr, error, 0, reinterpret_cast<LPSTR>(&message), 0, nullptr);

        std::string result;
        if (length && message)
        {
            result.assign(message, length);
            while (!result.empty() && (result.back() == '\r' || result.back() == '\n'))
            {
                result.pop_back();
            }
        }
        else
        {
            result = "error " + std::to_string(error);
        }

        if (message)
        {
            LocalFree(message);
        }

        return result;
    }

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

    inline void LogError(const char *format, ...)
    {
        const DWORD error = GetLastError();
        char message[420] = {};

        if (format)
        {
            va_list args;
            va_start(args, format);
            const HRESULT result = StringCchVPrintfA(message, ARRAYSIZE(message), format, args);
            va_end(args);

            if (FAILED(result) && result != STRSAFE_E_INSUFFICIENT_BUFFER)
            {
                StringCchCopyA(message, ARRAYSIZE(message), "<log formatting failed>");
            }
        }

        char output[560] = {};
        StringCchPrintfA(output, ARRAYSIZE(output), "[ACSEFilter] %s: %s\n", message, FormatError(error).c_str());
        OutputDebugStringA(output);
        SetLastError(error);
    }

} // namespace ACSEFilter::Debug

#define LOG(...) ::ACSEFilter::Debug::Log(__VA_ARGS__)
#define LOG_ERROR(...) ::ACSEFilter::Debug::LogError(__VA_ARGS__)

#else

#define LOG(...) ((void)0)
#define LOG_ERROR(...) ((void)0)

#endif
