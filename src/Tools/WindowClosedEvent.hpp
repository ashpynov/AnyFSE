#pragma once

#include <windows.h>
#include <atomic>

namespace AnyFSE::Tools
{
    class WindowClosedEvent final
    {
    public:
        explicit WindowClosedEvent(HWND window);
        ~WindowClosedEvent();

        WindowClosedEvent(const WindowClosedEvent&) = delete;
        WindowClosedEvent& operator=(const WindowClosedEvent&) = delete;
        // Borrowed handle; nullptr if the worker could not be started.
        HANDLE Handle() const { return m_thread; }

    private:
        HWND m_window = nullptr;
        DWORD m_processId = 0;
        DWORD m_windowThreadId = 0;
        HANDLE m_thread = nullptr;
        std::atomic<bool> m_stopRequested{false};

        static thread_local WindowClosedEvent* This;
        static DWORD WINAPI Run(void* context);
        DWORD WINAPI Run();
    };
}
