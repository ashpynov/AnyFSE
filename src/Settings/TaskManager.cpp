// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#define _WIN32_DCOM
#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#include <string>
#include "Logging/LogManager.hpp"

#pragma comment(lib, "taskschd.lib")
#include "TaskManager.hpp"

namespace AnyFSE::Settings::TaskManager
{

    #define TRY(hr) if (FAILED((hr))) throw std::exception((std::string("Can't register task at ") + std::to_string(__LINE__)).c_str());
    #define FREE(f) if(f) f->Release();

    static const std::wstring TaskName = L"AnyFSE home application";
    Logger log = LogManager::GetLogger("TaskManager");

    bool CreateTask()
    {

        bool result = false;

        // Get current executable path
        wchar_t modulePath[MAX_PATH];
        GetModuleFileNameW(NULL, modulePath, MAX_PATH);

        ITaskService *pService = NULL;
        ITaskFolder *pRootFolder = NULL;
        ITaskDefinition *pTask = NULL;
        IRegisteredTask *pRegisteredTask = NULL;
        IPrincipal *pPrincipal = NULL;
        ITriggerCollection *pTriggerCollection = NULL;
        ITrigger *pTrigger = NULL;
        IBootTrigger *pBootTrigger = NULL;
        IActionCollection *pActionCollection = NULL;
        IAction *pAction = NULL;
        IExecAction *pExecAction = NULL;
        ITaskSettings* pSettings = NULL;
        IRegistrationInfo* pRegInfo = NULL;

        try
        {
            TRY( CoInitializeEx(NULL, COINIT_MULTITHREADED));
            TRY( CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void **)&pService));
            TRY( pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()));
            TRY( pService->GetFolder(_bstr_t(L"\\"), &pRootFolder));            // Get root task folder
            TRY( pService->NewTask(0, &pTask));                                 // Create task definition
            TRY( pTask->get_Settings(&pSettings));                              // Get task settings to configure power and execution time limits
            TRY( pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE));     // Disable "Start only on AC power" - allow running on battery
            TRY( pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S")));          // Disable "Stop task if it runs longer than 3 hours" "PT0S" means no time limit
            TRY( pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE));         // Also set other related power settings
            TRY( pTask->get_Principal(&pPrincipal));                            // Set principal for elevated privileges
            TRY( pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST));              // Run with highest privileges:cite[5]
            TRY( pPrincipal->put_UserId(_bstr_t(L"SYSTEM")));                   // Run as SYSTEM user
            TRY( pPrincipal->put_LogonType(TASK_LOGON_SERVICE_ACCOUNT));        // Service account logon type

            TRY( pTask->get_Triggers(&pTriggerCollection));
            TRY( pTriggerCollection->Create(TASK_TRIGGER_BOOT, &pTrigger));     // Trigger on system boot
            TRY( pTrigger->QueryInterface(IID_IBootTrigger, (void **)&pBootTrigger));
            TRY( pBootTrigger->put_Id(_bstr_t(L"BootTrigger")));
            TRY( pTask->get_Actions(&pActionCollection));
            TRY( pActionCollection->Create(TASK_ACTION_EXEC, &pAction));
            TRY( pAction->QueryInterface(IID_IExecAction, (void **)&pExecAction));
            TRY( pExecAction->put_Path(_bstr_t(modulePath)));                   // Use current process path:cite[2]
            TRY( pExecAction->put_Arguments(_bstr_t(L"--service")));

            TRY(pTask->get_RegistrationInfo(&pRegInfo));
            TRY(pRegInfo->put_Author(_bstr_t(L"Artem Shpynov")));
            TRY(pRegInfo->put_Description(_bstr_t(L"Starts AnyFSE home application as a system service on Windows boot")));

            TRY( pRootFolder->RegisterTaskDefinition(                           // Register the task
                    _bstr_t(TaskName.c_str()),      // Task name
                    pTask,
                    TASK_CREATE_OR_UPDATE,
                    _variant_t(),                 // User credentials (empty for current user)
                    _variant_t(),                 // Password
                    TASK_LOGON_INTERACTIVE_TOKEN, // Logon type:cite[5]
                    _variant_t(L""),              // SDDL
                    &pRegisteredTask));

            result = true;
        }
        catch(const std::exception& e)
        {
            log.Error(log.APIError(), "Cannot create task: %s", e.what());
        }

        // Cleanup
        FREE(pRegInfo)
        FREE(pSettings)
        FREE(pService)
        FREE(pRootFolder)
        FREE(pTask)
        FREE(pRegisteredTask)
        FREE(pPrincipal)
        FREE(pTriggerCollection)
        FREE(pTrigger)
        FREE(pBootTrigger)
        FREE(pActionCollection)
        FREE(pAction)
        FREE(pExecAction)

        CoUninitialize();

        return result;
    }

    bool RemoveTask()
    {
        ITaskService *pService = NULL;
        ITaskFolder *pRootFolder = NULL;

        try
        {  
            TRY( CoInitializeEx(NULL, COINIT_MULTITHREADED));
            TRY( CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void **)&pService));
            TRY( pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()));
            TRY( pService->GetFolder(_bstr_t(L"\\"), &pRootFolder));
            pRootFolder->DeleteTask(_bstr_t(TaskName.c_str()), 0);
        }
        catch (HRESULT hr)
        {
            printf("Error removing task: 0x%08X\n", hr);
        }
        catch (...){}

        FREE(pRootFolder);
        FREE(pService);
        CoUninitialize();
        return true;
    }
}