// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>

#include <string>
#include <memory>
#include <cstdarg>
#include <vector>
#include <exception>
#include <Windows.h>

#include "Logger.hpp"
#include "LogManager.hpp"

namespace AnyFSE::Logging
{
    Logger::Logger(const std::string &name)
    {
        m_loggerName = name;
    }
    std::exception Logger::APIError(const char * prefix)
    {
        return APIError(0, prefix);
    }
    std::exception Logger::APIError(DWORD code, const char * prefix)
    {
        DWORD errorCode = code ? code : GetLastError();

        LPSTR messageBuffer = nullptr;
        DWORD size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&messageBuffer,
            0,
            NULL);

        std::string message;
        if (size > 0)
        {
            message.assign(messageBuffer, size);
            LocalFree(messageBuffer);
        }
        else
        {
            message = "Unknown error code: " + std::to_string(errorCode);
        }

        return std::runtime_error(std::string(prefix) + message + " (Error " + std::to_string(errorCode) + ")");
    }

    #define LOGGER(__level__) \
    void Logger::__level__(const char * format, ...) \
    {\
        va_list args;\
        va_start(args, format);\
        WriteMessage(LogLevels::__level__, format, args);\
        va_end(args);\
    }\
    void Logger::__level__(const std::exception &exception, const char * format, ...) \
    {\
        va_list args;\
        va_start(args, format);\
        WriteMessage(LogLevels::__level__, format, args);\
        va_end(args);\
        WriteMessage(LogLevels::__level__, exception.what());\
    }

    LOGGER(Trace)
    LOGGER(Debug)
    LOGGER(Info)
    LOGGER(Warn)
    LOGGER(Error)
    LOGGER(Critical)

    void Logger::WriteMessage(LogLevels level, const char * format, va_list args)
    {
        LogManager::WriteMessage(level, m_loggerName, format, args);
    }
}