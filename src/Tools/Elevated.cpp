#include "Elevated.hpp"

#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#include <wrl/client.h>
#include <memory>
#include <map>
#include <stdexcept>
#include <utility>
#include "App/Constants.hpp"
#include "Logging/LogManager.hpp"

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace AnyFSE::Tools::Elevated
{
    namespace c = App::Constants;
    namespace wrl = Microsoft::WRL;

    namespace
    {
        std::map<std::wstring, std::function<void()>> s_handlers;
        Logger log = LogManager::GetLogger("Elevated");

        struct CloseHandleDeleter
        {
            void operator()(void *handle) const { CloseHandle(handle); }
        };
        typedef std::unique_ptr<void, CloseHandleDeleter> Handle;

        void Check(HRESULT result)
        {
            if (FAILED(result))
                throw std::runtime_error("Elevated task error: " + std::to_string(static_cast<unsigned long>(result)));
        }

        class ComScope
        {
            HRESULT m_result;
        public:
            ComScope() : m_result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))
            {
                if (m_result != RPC_E_CHANGED_MODE)
                    Check(m_result);
            }
            ~ComScope() { if (SUCCEEDED(m_result)) CoUninitialize(); }
        };

        std::wstring EventName(const std::wstring &name)
        {
            if (name.empty() || name.find(L'\\') != std::wstring::npos)
                throw std::invalid_argument("Invalid elevated handler name");
            return std::wstring(c::ElevatedEventPrefix) + name;
        }
    }

    void Register(const std::wstring &name, std::function<void()> handler)
    {
        EventName(name);
        if (!handler)
            throw std::invalid_argument("Empty elevated handler");
        s_handlers.insert_or_assign(name, std::move(handler));
    }

    bool CallHandler()
    {
        try
        {
            for (const auto &[name, handler] : s_handlers)
            {
                Handle event(OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, EventName(name).c_str()));
                if (!event)
                {
                    const DWORD error = GetLastError();
                    if (error == ERROR_FILE_NOT_FOUND)
                        continue;
                    Check(HRESULT_FROM_WIN32(error));
                }
                const DWORD state = WaitForSingleObject(event.get(), 0);
                if (state == WAIT_FAILED)
                    Check(HRESULT_FROM_WIN32(GetLastError()));
                if (state != WAIT_OBJECT_0)
                    continue;

                handler();
                // A cleared request confirms that the handler returned successfully.
                if (!ResetEvent(event.get()))
                    Check(HRESULT_FROM_WIN32(GetLastError()));
                return true;
            }
            log.Error("No elevated command was requested");
        }
        catch (const std::exception &error)
        {
            log.Error("Elevated handler failed: %s", error.what());
        }
        return false;
    }

    bool ElevatedStartupApps()
    {
        return Call(c::ElevatedStartupApps);
    }

    bool Call(const std::wstring &name)
    {
        try
        {
            const std::wstring eventName = EventName(name);
            // Holding this object reserves the single request slot, including across threads/processes.
            Handle call(CreateEventW(nullptr, TRUE, FALSE, c::ElevatedCallEvent));
            const DWORD callError = GetLastError();
            if (!call)
                Check(HRESULT_FROM_WIN32(callError));
            if (callError == ERROR_ALREADY_EXISTS)
                throw std::runtime_error("An elevated call is already in progress");

            ComScope com;
            wrl::ComPtr<ITaskService> service;
            Check(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&service)));
            Check(service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()));
            wrl::ComPtr<ITaskFolder> folder;
            Check(service->GetFolder(_bstr_t(c::TaskSchedulerRoot), &folder));
            wrl::ComPtr<IRegisteredTask> task;
            Check(folder->GetTask(_bstr_t(c::AnyFseTaskName), &task));
            TASK_STATE state;
            Check(task->get_State(&state));
            if (state != TASK_STATE_READY)
                throw std::runtime_error("Elevated scheduled task is not ready");

            Handle event(CreateEventW(nullptr, TRUE, TRUE, eventName.c_str()));
            const DWORD eventError = GetLastError();
            if (!event)
                Check(HRESULT_FROM_WIN32(eventError));
            if (eventError == ERROR_ALREADY_EXISTS)
                throw std::runtime_error("Elevated request already exists");

            DWORD sessionId = 0;
            if (!ProcessIdToSessionId(GetCurrentProcessId(), &sessionId))
                Check(HRESULT_FROM_WIN32(GetLastError()));
            wrl::ComPtr<IRunningTask> running;
            Check(task->RunEx(_variant_t(), TASK_RUN_USE_SESSION_ID, sessionId, nullptr, &running));
            if (!running)
                throw std::runtime_error("Elevated scheduled task did not start");

            for (;;)
            {
                const HRESULT result = running->Refresh();
                if (result == SCHED_E_TASK_NOT_RUNNING)
                    break;
                Check(result);
                Check(running->get_State(&state));
                if (state != TASK_STATE_RUNNING && state != TASK_STATE_QUEUED)
                    break;
                Sleep(100);
            }

            const DWORD result = WaitForSingleObject(event.get(), 0);
            if (result == WAIT_FAILED)
                Check(HRESULT_FROM_WIN32(GetLastError()));
            if (result != WAIT_TIMEOUT)
                throw std::runtime_error("Elevated command was not completed");
            return true;
        }
        catch (const std::exception &error)
        {
            log.Error("Elevated call failed: %s", error.what());
            return false;
        }
    }
}
