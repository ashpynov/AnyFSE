#pragma once

#include <mutex>
#include <iostream>
#include <sstream>
#include <fstream>
#include "Logger.h"

namespace AnyFSE::Logging
{
    class LogManager
    {
        friend class Logger;

    private:
        static std::ofstream logWriter;
        static std::mutex writeLock;
        static LogLevel logLevel;
        static std::string applicationName;
        static bool logToConsole;

        LogManager() {};
        ~LogManager();

        static std::string FormatString(const char* format, va_list args);
        // Helper methods
        static const char * LogLevelToString(LogLevel level);
        static void WriteMessage(LogLevel level, const std::string &loggerName, const char* format, va_list args);

    public:
        static void Initialize(const std::string &appName, LogLevel level, const std::string &filePath = "");
        static Logger GetLogger(const std::string &loggerName = "");
    };
}

using namespace AnyFSE::Logging;
