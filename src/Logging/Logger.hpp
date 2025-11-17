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
