// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>

#pragma once

#ifdef _TRACE
#define TRACE log.Trace
#else
#define TRACE(...)
#endif

#include <string>
#include <windows.h>
#include <exception>


namespace AnyFSE::Logging
{
    enum class LogLevels
    {
        Disabled,
        Critical,
        Error,
        Warn,
        Info,
        Debug,
        Trace,
        Max
    };

    class Logger
    {
        friend class LogManager;
    private:
        std::string m_loggerName;
        void WriteMessage(LogLevels level, const char * format, va_list args = NULL);

    public:
        Logger(const std::string& name);
        static std::exception APIError(DWORD errorCode = 0, const char * prefix = "");
        static std::exception APIError(const char * prefix);
        void Trace(const char * format, ...);
        void Trace(const std::exception &exception, const char * format, ...);
        void Debug(const char * format, ...);
        void Debug(const std::exception &exception, const char * format, ...);
        void Info(const char * format, ...);
        void Info(const std::exception &exception, const char * format, ...);
        void Warn(const char * format, ...);
        void Warn(const std::exception &exception, const char * format, ...);
        void Error(const char * format, ...);
        void Error(const std::exception &exception, const char * format, ...);
        void Critical(const char * format, ...);
        void Critical(const std::exception &exception, const char * format, ...);
    };
}

using namespace AnyFSE::Logging;
