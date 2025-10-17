#include "Configuration/Config.hpp"
#include "Monitors/GamingExperience.hpp"
#include "Tools/Process.hpp"
#include "Logging/LogManager.hpp"
#include "Window/MainWindow.hpp"
#include "ManagerService.hpp"

namespace AnyFSE::Manager::State
{
    static Logger log = LogManager::GetLogger("Manager/Service");

    ManagerService::ManagerService() : ManagerCycle(true)
    {

    }

    ManagerService::~ManagerService()
    {
    }

    void ManagerService::ProcessEvent(StateEvent event)
    {
        switch (event)
        {
            case StateEvent::CONNECT:
            case StateEvent::XBOX_DETECTED:
            case StateEvent::OPEN_HOME:
            case StateEvent::OPEN_DEVICE_FORM:
                NotifyRemote(event);
                return;

            case StateEvent::XBOX_DENY:
                return OnXboxDeny.Notify();

            case StateEvent::XBOX_ALLOW:
                return OnXboxAllow.Notify();

            case StateEvent::MONITOR_REGISTRY:
                return OnMonitorRegistry.Notify();

            case StateEvent::EXIT_SERVICE:
                return OnExit.Notify();

            default:
                log.Info("Recieved event by service %d", event);
        }
    }
}