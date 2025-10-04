#include "ManagerCycle.hpp"
#include "Logging/LogManager.hpp"
#undef max
#include <chrono>

namespace AnyFSE::Manager::Cycle
{

    Logger log = LogManager::GetLogger("Manager/State");

    ManagerCycle::ManagerCycle() = default;

    ManagerCycle::~ManagerCycle()
    {
        Stop();
    }

    void ManagerCycle::Notify(StateEvent event)
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            eventQueue.push(event);
        }
        queueCondition.notify_one();
    }

    void ManagerCycle::Start()
    {
        if (isRunning.exchange(true))
        {
            return; // Already running
        }

        processingThread = std::thread(&ManagerCycle::ProcessingCycle, this);
    }

    void ManagerCycle::Stop()
    {
        if (!isRunning.exchange(false))
        {
            return; // Already stopped
        }

        queueCondition.notify_all();
        if (processingThread.joinable())
        {
            processingThread.join();
        }

        // Clear timers - no lock needed since thread is joined
        timers.clear();
    }

    uint64_t ManagerCycle::SetTimer(std::chrono::milliseconds timeout, std::function<void()> callback, bool recurring)
    {
        // This can only be called from the processing thread during event handling
        uint64_t id = nextTimerId++;
        timers[id] = TimerInfo{
            std::chrono::steady_clock::now() + timeout,
            std::move(callback),
            recurring,
            timeout,
            false};
        queueCondition.notify_one(); // Wake up to recalculate wait time
        return id;
    }

    void ManagerCycle::CancelTimer(uint64_t id)
    {
        // This can only be called from the processing thread during event handling
        TimerInfo& timer = timers[id];
        // do not delete here, mark as invalid
        timer.isInvalid = true;
        queueCondition.notify_one(); // Wake up to recalculate wait time
    }

    void ManagerCycle::ProcessingCycle()
    {
        log.Info("Entering StateManager loop");

        while (isRunning)
        {
            // Check for the next timer expiration
            auto nextTimeout = GetNextTimeout();

            std::unique_lock<std::mutex> lock(queueMutex);

            if (eventQueue.empty())
            {
                // Wait for events or timer expiration
                if (nextTimeout.has_value())
                {
                    // Wait with timeout for the next timer
                    queueCondition.wait_until(lock, nextTimeout.value());
                }
                else
                {
                    // Wait indefinitely for events
                    queueCondition.wait(lock);
                }
            }

            // Process all available events
            while (!eventQueue.empty() && isRunning)
            {
                StateEvent event = eventQueue.front();
                eventQueue.pop();

                // Release lock while processing event to allow new events
                lock.unlock();
                ProcessEvent(event);
                lock.lock();
            }

            // Check and process expired timers
            CheckExpiredTimers();
        }
    }

    std::optional<std::chrono::steady_clock::time_point> ManagerCycle::GetNextTimeout()
    {
        if (timers.empty())
        {
            return std::nullopt;
        }

        auto next = std::chrono::steady_clock::time_point::max();
        for (auto it = timers.begin(); it != timers.end();)
        {
            if (it->second.isInvalid)
            {
                it = timers.erase(it);
                continue;
            } 
            else if (it->second.expirationTime < next)
            {
                next = it->second.expirationTime;
            }
            ++it;
        }
        if (timers.empty())
        {
            return std::nullopt;
        }

        return next;
    }

    void ManagerCycle::CheckExpiredTimers()
    {
        auto now = std::chrono::steady_clock::now();

        for (auto it = timers.begin(); it != timers.end();)
        {
            if (it->second.expirationTime <= now)
            {
                // Execute callback
                if (it->second.callback)
                {
                    it->second.callback();
                }

                if (it->second.isRecurring && !it->second.isInvalid)
                {
                    // Reschedule recurring timer
                    it->second.expirationTime = now + it->second.interval;
                    ++it;
                }
                else
                {
                    // Remove one-shot timer
                    it = timers.erase(it);
                }
            }
            else
            {
                ++it;
            }
        }
    }

} // namespace AnyFSE::Manager