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
                OnStart.Notify();
                NotifyRemote(event);
                return;
            case AppEvents::RELOAD_CONFIG:
                return OnReloadConfig.Notify();

            case AppEvents::LAUNCHER_STOPPED:
            case AppEvents::XBOX_DETECTED:
            case AppEvents::OPEN_HOME:
            case AppEvents::OPEN_DEVICE_FORM:
                NotifyRemote(event);
                return;

            case AppEvents::XBOX_DENY:
                return OnXboxDeny.Notify();

            case AppEvents::XBOX_ALLOW:
                return OnXboxAllow.Notify();

            case AppEvents::SUSPEND_SERVICE:
                return OnSuspend.Notify();

            case AppEvents::RESTART_SERVICE:
                return OnRestart.Notify();

            default:
                log.Trace("Recieved event by service %d", event);
        }
    }
}