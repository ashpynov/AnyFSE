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
        void Wait(DWORD timeout);
        bool IsRunning() { return m_isRunning.load(); }

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
            bool isRecurring = false;
            std::chrono::milliseconds interval;
            bool isInvalid = false;
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

