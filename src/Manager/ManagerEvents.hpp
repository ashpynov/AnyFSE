
#pragma once

namespace AnyFSE::Manager
{
    enum class StateEvent : int
    {
        START = 0,
        XBOX_DETECTED,
        PLAYNITE_LAUNCHED,
        GAMEMODE_ENTER,
        OPEN_HOME
    };
}

using namespace AnyFSE::Manager;
