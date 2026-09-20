#include "ScheduledTask.hpp"

#include <windows.h>
#include <taskschd.h>
#include <sddl.h>
#include <comdef.h>
#include <wrl/client.h>
#include <filesystem>
#include <stdexcept>
#include "App/Constants.hpp"

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "advapi32.lib")

namespace AnyFSE::ToolsEx::ScheduledTask
{
    namespace c = App::Constants;
    namespace wrl = Microsoft::WRL;

    namespace
    {
        void Check(HRESULT result)
        {
            if (FAILED(result))
            {
                throw std::runtime_error("AnyFSE scheduled task error: " + std::to_string(static_cast<unsigned long>(result)));
            }
        }

        class ComScope
        {
            HRESULT m_result;

        public:
            ComScope() : m_result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))
            {
                if (m_result != RPC_E_CHANGED_MODE)
                {
                    Check(m_result);
                }
            }

            ~ComScope()
            {
                if (SUCCEEDED(m_result))
                {
                    CoUninitialize();
                }
            }
        };

        wrl::ComPtr<ITaskService> Connect()
        {
            wrl::ComPtr<ITaskService> service;
            Check(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&service)));
            Check(service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()));
            return service;
        }

        std::wstring CurrentUserSid()
        {
            HANDLE token = nullptr;
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
            {
                Check(HRESULT_FROM_WIN32(GetLastError()));
            }

            // TOKEN_USER plus the largest possible SID fits in this fixed buffer.
            alignas(TOKEN_USER) BYTE buffer[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE] = {};
            DWORD size = 0;
            const BOOL success = GetTokenInformation(token, TokenUser, buffer, sizeof(buffer), &size);
            const DWORD error = success ? ERROR_SUCCESS : GetLastError();
            CloseHandle(token);
            Check(HRESULT_FROM_WIN32(error));

            LPWSTR sidText = nullptr;
            if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(buffer)->User.Sid, &sidText))
            {
                Check(HRESULT_FROM_WIN32(GetLastError()));
            }
            const std::wstring sid(sidText);
            LocalFree(sidText);
            return sid;
        }
    }

    void RegisterAnyFSETask(const std::wstring &installPath)
    {
        ComScope com;
        auto service = Connect();
        wrl::ComPtr<ITaskFolder> folder;
        Check(service->GetFolder(_bstr_t(c::TaskSchedulerRoot), &folder));

        wrl::ComPtr<ITaskDefinition> task;
        Check(service->NewTask(0, &task));
        const std::wstring userSid = CurrentUserSid();

        wrl::ComPtr<IPrincipal> principal;
        Check(task->get_Principal(&principal));
        Check(principal->put_UserId(_bstr_t(userSid.c_str())));
        Check(principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN));
        Check(principal->put_RunLevel(TASK_RUNLEVEL_HIGHEST));

        // No triggers: this task only runs on demand in the installing user's session.
        wrl::ComPtr<ITaskSettings> settings;
        Check(task->get_Settings(&settings));
        Check(settings->put_Enabled(VARIANT_TRUE));
        Check(settings->put_AllowDemandStart(VARIANT_TRUE));
        Check(settings->put_DisallowStartIfOnBatteries(VARIANT_FALSE));
        Check(settings->put_StopIfGoingOnBatteries(VARIANT_FALSE));
        Check(settings->put_MultipleInstances(TASK_INSTANCES_IGNORE_NEW));

        wrl::ComPtr<IActionCollection> actions;
        Check(task->get_Actions(&actions));
        wrl::ComPtr<IAction> action;
        Check(actions->Create(TASK_ACTION_EXEC, &action));
        wrl::ComPtr<IExecAction> exec;
        Check(action.As(&exec));
        const std::wstring executable = (std::filesystem::path(installPath) / c::AnyFseExe).wstring();
        Check(exec->put_Path(_bstr_t(executable.c_str())));
        Check(exec->put_Arguments(_bstr_t(c::AnyFseTaskArgument)));
        Check(exec->put_WorkingDirectory(_bstr_t(installPath.c_str())));

        // The same user may read/run the task with a filtered token, but only administrators may change it.
        const std::wstring security = std::wstring(c::AnyFseTaskSecurityPrefix) + userSid + L")";
        wrl::ComPtr<IRegisteredTask> registered;
        Check(folder->RegisterTaskDefinition(_bstr_t(c::AnyFseTaskName), task.Get(), TASK_CREATE_OR_UPDATE | TASK_DONT_ADD_PRINCIPAL_ACE,
            _variant_t(userSid.c_str()), _variant_t(), TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(security.c_str()), &registered));
    }

    void DeleteAnyFSETask()
    {
        ComScope com;
        auto service = Connect();
        wrl::ComPtr<ITaskFolder> folder;
        Check(service->GetFolder(_bstr_t(c::TaskSchedulerRoot), &folder));
        const HRESULT result = folder->DeleteTask(_bstr_t(c::AnyFseTaskName), 0);
        if (result != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            Check(result);
        }
    }
}
