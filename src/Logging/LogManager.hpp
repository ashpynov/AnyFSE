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

#include <mutex>
#include <iostream>
#include <sstream>
#include <fstream>
#include "Logger.hpp"

namespace AnyFSE::Logging
{
    class LogManager
    {
        friend class Logger;

    private:
        static std::ofstream LogWriter;
        static std::mutex WriteLock;
        static LogLevels Level;
        static std::string ApplicationName;
        static bool LogToConsole;

        LogManager() {};
        ~LogManager();

        static std::string FormatString(const char* format, va_list args);
        // Helper methods
        static void WriteMessage(LogLevels level, const std::string &loggerName, const char* format, va_list args);

    public:
        static void Initialize(const std::string &appName, LogLevels level = LogLevels::Trace, const std::wstring &filePath = L"");
        static Logger GetLogger(const std::string &loggerName = "");
        static const char * LogLevelToString(LogLevels level);
    };
}

using namespace AnyFSE::Logging;
