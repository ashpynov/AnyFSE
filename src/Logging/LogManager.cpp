#include <sysinfoapi.h>
#include <debugapi.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include "Logger.hpp"
#include "LogManager.hpp"
#include <iomanip>
#include <cstdarg>
#include <vector>
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"

using namespace std;

using namespace AnyFSE::Logging;


namespace AnyFSE::Logging
{
    std::ofstream LogManager::LogWriter;
    std::mutex LogManager::WriteLock;
    LogLevels LogManager::Level;
    std::string LogManager::ApplicationName;
    bool LogManager::LogToConsole;

    void LogManager::Initialize(const string &appName, LogLevels level, const std::wstring& filePath)
    {
        if (!ApplicationName.empty())
        {
            return;
        }

        LogToConsole = IsDebuggerPresent() != 0;
        ApplicationName = appName;
        Level = level;

        if (!LogToConsole && !filePath.empty() && level != LogLevels::Disabled)
        {
            LogWriter.open(filePath, ios::app | ios::out);
        }
    }

    LogManager::~LogManager()
    {
        LogWriter.close();
    }

    // Helper methods
    const char *LogManager::LogLevelToString(LogLevels level)
    {
        switch (level)
        {
        case LogLevels::Trace:
            return "Trace";
        case LogLevels::Debug:
            return "Debug";
        case LogLevels::Info:
            return "Info";
        case LogLevels::Warn:
            return "Warn";
        case LogLevels::Error:
            return "Error";
        case LogLevels::Critical:
            return "Critical";
        default:
            return "Unknown";
        }
    }

    std::string LogManager::FormatString(const char *format, va_list args)
    {
        va_list args_copy;
        va_copy(args_copy, args);

        int size = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (size < 0)
        {
            return format;
        }

        std::vector<char> buffer(size + 1);
        vsnprintf(buffer.data(), buffer.size(), format, args);

        return std::string(buffer.data(), size);
    }
    // Main logging method
    void LogManager::WriteMessage(LogLevels level, const string &loggerName, const char *format, va_list args)
    {
        if (level < Level || !(LogToConsole || LogWriter.is_open()))
        {
            return;
        }
        SYSTEMTIME systemTime;
        GetLocalTime(&systemTime);

        // Format timestamp: yyyy-MM-dd HH:mm:ss.fff
        char timestamp[24];
        snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                 systemTime.wYear, systemTime.wMonth, systemTime.wDay,
                 systemTime.wHour, systemTime.wMinute, systemTime.wSecond,
                 systemTime.wMilliseconds);

        // Format console time: HH:mm:ss.fff
        char consoleTime[13];
        snprintf(consoleTime, sizeof(consoleTime), "%02d:%02d:%02d.%03d",
                 systemTime.wHour, systemTime.wMinute, systemTime.wSecond,
                 systemTime.wMilliseconds);

        // Convert log level to lowercase string
        string levelText = LogLevelToString(level);
        transform(levelText.begin(), levelText.end(), levelText.begin(), ::tolower);

        string prefix = string(timestamp) + " [" + levelText + "] [" + loggerName + "]";
        string consolePrefix = string(consoleTime) + " [" + levelText + "] [" + ApplicationName + "/" + loggerName + "]";

        string message = FormatString(format, args);

        // Write to file
        if (LogWriter.is_open())
        {
            string fullLogMessage = prefix + " " + message;
            lock_guard<mutex> lock(WriteLock);
            LogWriter << fullLogMessage << endl;
            LogWriter.flush();
        }

        // Write to console if debugger attached
        if (LogToConsole)
        {
            vector<string> lines;
            stringstream ss(message);
            string line;

            while (getline(ss, line, '\n'))
            {
                lines.push_back(line);
            }

            for (const auto &line : lines)
            {
                cout << left << setw(48) << consolePrefix << "| " << line << endl;
            }
            std::cout.flush();
        }
    }

    Logger LogManager::GetLogger(const std::string &loggerName)
    {
        return Logger(loggerName);
    }
}