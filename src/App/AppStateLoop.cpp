#include "App/AppStateLoop.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Unicode.hpp"
#undef max
#include <chrono>

namespace AnyFSE::App::StateLoop
{

    AppStateLoop::AppStateLoop(bool isServer)
        : m_ipcChannel(L"AnyFSEPipe", isServer)
        , _log(LogManager::GetLogger(isServer ? "Manager/SrvCycle" :  "Manager/AppCycle" ))
    {
        m_hQueueCondition = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_ipcChannel.SetCancelEvent(m_hQueueCondition);
    }

    AppStateLoop::~AppStateLoop()
    {
        Stop();
        DeleteObject(m_hQueueCondition);
    }

    void AppStateLoop::Notify(AppEvents event)
    {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_eventQueue.push(event);
        }
        SetEvent(m_hQueueCondition);
    }

    bool AppStateLoop::NotifyRemote(AppEvents event, DWORD timeout)
    {
        AnyFSE::App::Message message;
        message.event = event;
        message.ticks = GetTickCount();
        return m_ipcChannel.Write(&message, timeout);
    }

    void AppStateLoop::Start()
    {
        if (m_isRunning.exchange(true))
        {
            return; // Already running
        }

        m_thread = std::thread(&AppStateLoop::ProcessingCycle, this);
    }

    void AppStateLoop::Stop()
    {
        if (!m_isRunning.exchange(false))
        {
            return; // Already stopped
        }

        SetEvent(m_hQueueCondition);
        if (m_thread.joinable())
        {
            m_thread.join();
        }

        // Clear timers - no lock needed since thread is joined
        m_timersMap.clear();
    }

    uint64_t AppStateLoop::SetTimer(std::chrono::milliseconds timeout, std::function<void()> callback, bool recurring)
    {
        // This can only be called from the processing thread during event handling
        uint64_t id = m_nextTimerId++;
        m_timersMap[id] = TimerInfo{
            std::chrono::steady_clock::now() + timeout,
            std::move(callback),
            recurring,
            timeout,
            false};
        SetEvent(m_hQueueCondition);
        return id;
    }

    void AppStateLoop::CancelTimer(uint64_t id)
    {
        // This can only be called from the processing thread during event handling
        TimerInfo& timer = m_timersMap[id];
        // do not delete here, mark as invalid
        timer.isInvalid = true;
        SetEvent(m_hQueueCondition);
    }

    void AppStateLoop::ProcessingCycle()
    {
        _log.Info("Entering StateManager loop");

        while (m_isRunning)
        {
            // Check for the next timer expiration
            auto nextTimeout = GetNextTimeout();

            std::unique_lock<std::mutex> lock(m_queueMutex);

            if (m_eventQueue.empty())
            {
                DWORD timeoutMs = INFINITE;
                if (nextTimeout.has_value())
                {
                    auto now = std::chrono::steady_clock::now();
                    if (nextTimeout.value() > now)
                    {
                        timeoutMs = (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(
                                        nextTimeout.value() - now).count();
                    }
                    else
                    {
                        timeoutMs = 0; // Already expired
                    }
                }
                // Wait for events or timer expiration
                lock.unlock();
                m_ipcChannel.Wait(timeoutMs);
                lock.lock();
            }

            // Process all available events
            while (!m_eventQueue.empty() && m_isRunning)
            {
                AppEvents event = m_eventQueue.front();
                m_eventQueue.pop();

                // Release lock while processing event to allow new events
                lock.unlock();
                ProcessEvent(event);
                lock.lock();
            }

            AnyFSE::App::Message ipcMessage;
            LONGLONG eol = GetTickCount64() - MESSAGES_EOL;
            while (m_ipcChannel.Read(&ipcMessage))
            {
                if (ipcMessage.ticks > eol)
                {
                    lock.unlock();
                    #ifdef _TRACE
                        _log.Trace("Got message: %d (%ldms)", ipcMessage.event, GetTickCount64()-ipcMessage.ticks);
                    #endif
                    ProcessEvent(ipcMessage.event);
                    lock.lock();
                }
            }

            // Check and process expired timers
            CheckExpiredTimers();
        }
    }

    std::optional<std::chrono::steady_clock::time_point> AppStateLoop::GetNextTimeout()
    {
        if (m_timersMap.empty())
        {
            return std::nullopt;
        }

        auto next = std::chrono::steady_clock::time_point::max();
        for (auto it = m_timersMap.begin(); it != m_timersMap.end();)
        {
            if (it->second.isInvalid)
            {
                it = m_timersMap.erase(it);
                continue;
            }
            else if (it->second.expirationTime < next)
            {
                next = it->second.expirationTime;
            }
            ++it;
        }
        if (m_timersMap.empty())
        {
            return std::nullopt;
        }

        return next;
    }

    void AppStateLoop::CheckExpiredTimers()
    {
        auto now = std::chrono::steady_clock::now();

        for (auto it = m_timersMap.begin(); it != m_timersMap.end();)
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
                    it = m_timersMap.erase(it);
                }
            }
            else
            {
                ++it;
            }
        }
    }

} // namespace AnyFSE::App