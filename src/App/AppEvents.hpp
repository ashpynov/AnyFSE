
#pragma once

namespace AnyFSE::App
{
    enum class AppEvents : int
    {
        CONNECT = 0,
        START,
        XBOX_DETECTED,
        PLAYNITE_LAUNCHED,
        GAMEMODE_ENTER,
        GAMEMODE_EXIT,
        OPEN_HOME,
        OPEN_DEVICE_FORM,
        XBOX_DENY,
        XBOX_ALLOW,
        MONITOR_REGISTRY,
        EXIT_SERVICE,
        QUERY_END_SESSION,
        END_SESSION
    };
}

using namespace AnyFSE::App;
