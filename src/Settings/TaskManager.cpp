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
        ILogonTrigger *pLogonTrigger = NULL;
        IActionCollection *pActionCollection = NULL;
        IAction *pAction = NULL;
        IExecAction *pExecAction = NULL;
        ITaskSettings* pSettings = NULL;

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
            TRY( pTask->get_Triggers(&pTriggerCollection));                     // Create logon trigger for any user
            TRY( pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger));    // Trigger on any user logon:cite[2]
            TRY( pTrigger->QueryInterface(IID_ILogonTrigger, (void **)&pLogonTrigger));
            TRY( pLogonTrigger->put_Id(_bstr_t(L"LogonTrigger")));
            TRY( pLogonTrigger->put_UserId(_bstr_t(L"")));                      // Empty string = any user:cite[2]
            TRY( pTask->get_Actions(&pActionCollection));
            TRY( pActionCollection->Create(TASK_ACTION_EXEC, &pAction));
            TRY( pAction->QueryInterface(IID_IExecAction, (void **)&pExecAction));
            TRY( pExecAction->put_Path(_bstr_t(modulePath)));                   // Use current process path:cite[2]

            TRY( pRootFolder->RegisterTaskDefinition(                           // Register the task
                    _bstr_t(L"AnyFSE Launcher"),  // Task name
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
        FREE(pSettings)
        FREE(pService)
        FREE(pRootFolder)
        FREE(pTask)
        FREE(pRegisteredTask)
        FREE(pPrincipal)
        FREE(pTriggerCollection)
        FREE(pTrigger)
        FREE(pLogonTrigger)
        FREE(pActionCollection)
        FREE(pAction)
        FREE(pExecAction)

        CoUninitialize();

        return result;
    }
}