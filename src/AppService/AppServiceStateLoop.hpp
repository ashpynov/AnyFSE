// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>

#pragma once

#include "App/AppEvents.hpp"
#include "App/AppStateLoop.hpp"
#include "Tools/Event.hpp"

namespace AnyFSE::App::AppService::StateLoop
{

    class AppServiceStateLoop: public App::StateLoop::AppStateLoop
    {
        public:
            AppServiceStateLoop();
            ~AppServiceStateLoop();

            Event OnMonitorRegistry;
            Event OnXboxDeny;
            Event OnXboxAllow;
            Event OnExit;

        private:
            virtual void ProcessEvent(AppEvents event);
    };
}

using namespace AnyFSE::App::AppService::StateLoop;