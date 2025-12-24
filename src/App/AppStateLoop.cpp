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

#include "App/AppStateLoop.hpp"
#include "Logging/LogManager.hpp"
#undef max
#include <chrono>
#include "AppStateLoop.hpp"

namespace AnyFSE::App::StateLoop
{

    AppStateLoop::AppStateLoop(bool isServer)
        : m_ipcChannel(L"AnyFSEPipe", isServer)
        , _log(LogManager::GetLogger("StateLoop"))
        , m_hReadPipe(nullptr)
        , m_hWritePipe(nullptr)
    {
        m_hQueueCondition = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_ipcChannel.SetCancelEvent(m_hQueueCondition);
    }

    AppStateLoop::~AppStateLoop()
    {
        Stop();
        DeleteObject(m_hQueueCondition);
        Wait(0);
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
        message.ticks = GetTickCount64();
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
        m_isRunning.exchange(false);
        SetEvent(m_hQueueCondition);
    }

    void AppStateLoop::Wait(DWORD timeout)
    {
        if (m_isRunning.exchange(false))
        {
            SetTimer(std::chrono::milliseconds(timeout), [this]() { SetEvent(m_hQueueCondition); }, false);
        }

        if (m_thread.joinable())
        {
            m_thread.join();
        }

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

                bool result = true;
                try
                {
                    if (!m_ipcChannel.Wait(timeoutMs) && m_ipcChannel.IsDisconnected())
                    {
                        Notify(AppEvents::DISCONNECT);
                    }
                }
                catch (const std::exception& ex)
                {
                    _log.Error(ex, "IPCChannel wait failed, exiting processing loop");
                    Notify(AppEvents::DISCONNECT);
                }

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
            while (m_ipcChannel.Read(&ipcMessage))
            {
                if (ipcMessage.ticks > (LONGLONG) GetTickCount64() - MESSAGES_EOL)
                {
                    lock.unlock();
                    _log.Trace("Got message: %s(%d) (delay %ldms)", AppEventsName(ipcMessage.event), ipcMessage.event, GetTickCount64()-ipcMessage.ticks);

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