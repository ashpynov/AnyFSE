#pragma once
#include "Window/MainWindow.hpp"
#include "ManagerCycle.hpp"
#include "ManagerEvents.hpp"
#include "Tools/Event.hpp"

namespace AnyFSE::Manager::State
{

    class ManagerService: public Cycle::ManagerCycle
    {
        public:
            ManagerService();
            ~ManagerService();

            Event OnMonitorRegistry;
            Event OnXboxDeny;
            Event OnXboxAllow;
            Event OnExit;

        private:
            virtual void ProcessEvent(StateEvent event);
    };
}

using namespace AnyFSE::Manager::State;