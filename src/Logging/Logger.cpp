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
        if (!code)
        {
            return std::runtime_error("");
        }

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