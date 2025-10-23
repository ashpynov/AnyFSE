#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <map>
#include <functional>
#include <optional>
#include <windows.h>

#include "Logging/LogManager.hpp"
#include "App/IPCChannel.hpp"
#include "App/AppStateLoop.hpp"

namespace AnyFSE::App::StateLoop
{

    class AppStateLoop
    {
        friend class AppControlStateLoop;
        static const int MESSAGES_EOL = 5000;

    protected:
        AnyFSE::App::IPCChannel m_ipcChannel;

    public:
        // Event enum - you can extend this as needed

        AppStateLoop(bool isServer);
        virtual ~AppStateLoop();

        // Delete copy constructor and assignment operator
        AppStateLoop(const AppStateLoop &) = delete;
        AppStateLoop &operator=(const AppStateLoop &) = delete;

        // Event management - public interface
        void Notify(AppEvents event);
        bool NotifyRemote(AppEvents event, DWORD timeout = 5000);

        // Main control functions
        void Start();
        void Stop();

    protected:
        // Timer management - only available to derived classes in event handlers
        uint64_t SetTimer(std::chrono::milliseconds timeout, std::function<void()> callback, bool recurring = false);
        void CancelTimer(uint64_t id);

        // Pure virtual function to handle events - must be implemented by derived class
        virtual void ProcessEvent(AppEvents event) = 0;

    private:
        // Thread-safe queue for events
        std::queue<AppEvents> m_eventQueue;
        std::mutex m_queueMutex;

        HANDLE m_hQueueCondition;

        HANDLE m_hReadPipe;
        HANDLE m_hWritePipe;

        Logger _log;
        // Timer management - only accessed from processing thread
        struct TimerInfo
        {
            std::chrono::steady_clock::time_point expirationTime;
            std::function<void()> callback;
            bool isRecurring;
            std::chrono::milliseconds interval;
            bool isInvalid;
        };

        std::map<uint64_t, TimerInfo> m_timersMap;
        uint64_t m_nextTimerId{1};

        // Control flags
        std::atomic<bool> m_isRunning{false};
        std::thread m_thread;

        // Private methods
        void ProcessingCycle();
        std::optional<std::chrono::steady_clock::time_point> GetNextTimeout();
        void CheckExpiredTimers();
    };

} // namespace AnyFSE::App

