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