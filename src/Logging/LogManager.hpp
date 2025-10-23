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
        static const char * LogLevelToString(LogLevels level);
        static void WriteMessage(LogLevels level, const std::string &loggerName, const char* format, va_list args);

    public:
        static void Initialize(const std::string &appName, LogLevels level = LogLevels::Trace, const std::wstring &filePath = L"");
        static Logger GetLogger(const std::string &loggerName = "");
    };
}

using namespace AnyFSE::Logging;
