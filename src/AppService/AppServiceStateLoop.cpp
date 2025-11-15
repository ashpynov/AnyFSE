// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>

#include "Configuration/Config.hpp"
#include "Tools/Process.hpp"
#include "Logging/LogManager.hpp"
#include "AppService/AppServiceStateLoop.hpp"

namespace AnyFSE::App::AppService::StateLoop
{
    static Logger log = LogManager::GetLogger("Manager/Service");

    AppServiceStateLoop::AppServiceStateLoop() : AppStateLoop(true) {}

    AppServiceStateLoop::~AppServiceStateLoop() {}

    void AppServiceStateLoop::ProcessEvent(AppEvents event)
    {
        switch (event)
        {
            case AppEvents::CONNECT:
                log.Info("AppControl connected" );
            case AppEvents::XBOX_DETECTED:
            case AppEvents::OPEN_HOME:
            case AppEvents::OPEN_DEVICE_FORM:
                NotifyRemote(event);
                return;

            case AppEvents::XBOX_DENY:
                return OnXboxDeny.Notify();

            case AppEvents::XBOX_ALLOW:
                return OnXboxAllow.Notify();

            case AppEvents::MONITOR_REGISTRY:
                return OnMonitorRegistry.Notify();

            case AppEvents::EXIT_SERVICE:
                return OnExit.Notify();

            default:
                log.Trace("Recieved event by service %d", event);
        }
    }
}