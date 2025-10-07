
#pragma once

namespace AnyFSE::Manager
{
    enum class StateEvent : int
    {
        START = 0,
        XBOX_DETECTED,
        PLAYNITE_LAUNCHED,
        GAMEMODE_ENTER,
        OPEN_HOME,
        OPEN_DEVICE_FORM
    };
}

using namespace AnyFSE::Manager;
