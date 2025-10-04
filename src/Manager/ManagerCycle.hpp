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

#include "ManagerEvents.hpp"

namespace AnyFSE::Manager::Cycle
{

    class ManagerCycle
    {
        friend class ManagerState;
    public:
        // Event enum - you can extend this as needed

        ManagerCycle();
        virtual ~ManagerCycle();

        // Delete copy constructor and assignment operator
        ManagerCycle(const ManagerCycle &) = delete;
        ManagerCycle &operator=(const ManagerCycle &) = delete;

        // Event management - public interface
        void Notify(StateEvent event);

        // Main control functions
        void Start();
        void Stop();

    protected:
        // Timer management - only available to derived classes in event handlers
        uint64_t SetTimer(std::chrono::milliseconds timeout, std::function<void()> callback, bool recurring = false);
        void CancelTimer(uint64_t id);

        // Pure virtual function to handle events - must be implemented by derived class
        virtual void ProcessEvent(StateEvent event) = 0;

    private:
        // Thread-safe queue for events
        std::queue<StateEvent> eventQueue;
        std::mutex queueMutex;
        std::condition_variable queueCondition;

        // Timer management - only accessed from processing thread
        struct TimerInfo
        {
            std::chrono::steady_clock::time_point expirationTime;
            std::function<void()> callback;
            bool isRecurring;
            std::chrono::milliseconds interval;
            bool isInvalid;
        };

        std::map<uint64_t, TimerInfo> timers;
        uint64_t nextTimerId{1};

        // Control flags
        std::atomic<bool> isRunning{false};
        std::thread processingThread;

        // Private methods
        void ProcessingCycle();
        std::optional<std::chrono::steady_clock::time_point> GetNextTimeout();
        void CheckExpiredTimers();
    };

} // namespace AnyFSE::Manager

