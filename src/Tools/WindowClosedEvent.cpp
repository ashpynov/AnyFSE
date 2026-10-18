#include "WindowClosedEvent.hpp"
#include "Logging/LogManager.hpp"

namespace AnyFSE::Tools
{
    static Logger log = LogManager::GetLogger("WindowClosedEvent");
    thread_local WindowClosedEvent* WindowClosedEvent::This = nullptr;

    WindowClosedEvent::WindowClosedEvent(HWND window)
        : m_thread(nullptr)
        , m_window(nullptr)
        , m_windowThreadId(0)
        , m_processId(0)
    {
        if (!window)
            return;

        m_windowThreadId = GetWindowThreadProcessId(window, &m_processId);

        if (!m_windowThreadId)
            return;

        m_window = window;

        m_thread = CreateThread(nullptr, 0, Run, this, 0, nullptr);
        if (!m_thread)
            log.Error(log.APIError(), "Can't start window close monitor");
    }

    WindowClosedEvent::~WindowClosedEvent()
    {
        if (m_thread)
        {
            m_stopRequested = true;
            PostThreadMessage(GetThreadId(m_thread), WM_QUIT, 0, 0);
            WaitForSingleObject(m_thread, INFINITE);
            CloseHandle(m_thread);
        }
    }

    DWORD WINAPI WindowClosedEvent::Run(void* context)
    {
        return static_cast<WindowClosedEvent *>(context)->Run();
    }

    DWORD WINAPI WindowClosedEvent::Run()
    {
        MSG message;
        PeekMessage(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
        // Destruction may precede creation of the worker's message queue.
        if (m_stopRequested)
            return 0;

        This = this;
        HWINEVENTHOOK hook = SetWinEventHook(EVENT_OBJECT_HIDE, EVENT_OBJECT_HIDE, nullptr,
            [](HWINEVENTHOOK, DWORD event, HWND window, LONG objectId, LONG childId, DWORD, DWORD)
            {
                if ( window == This->m_window
                     && (event == EVENT_OBJECT_HIDE || event == EVENT_OBJECT_HIDE)
                     && objectId == OBJID_WINDOW
                     && childId == CHILDID_SELF)
                    PostQuitMessage(0);
            },
            m_processId, m_windowThreadId, WINEVENT_OUTOFCONTEXT);

        if (!hook)
        {
            log.Error("Can't install window close hook");
            This = nullptr;
            return 0;
        }

        DWORD processId = 0;
        if ( GetWindowThreadProcessId(m_window, &processId) != m_windowThreadId || processId != m_processId)
        {
            log.Error("The window have disappeared before the hook was installed.");
            UnhookWinEvent(hook);
            This = nullptr;
            return 0;
        }

        while (GetMessage(&message, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        UnhookWinEvent(hook);
        This = nullptr;
        return 0;
    }
}
