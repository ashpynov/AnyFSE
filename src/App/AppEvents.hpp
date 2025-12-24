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

#pragma once

namespace AnyFSE::App
{
    enum class AppEvents : int
    {
        CONNECT = 0,
        START,
        START_APPS,
        XBOX_DETECTED,
        LAUNCHER_STOPPED,
        GAMEMODE_ENTER,
        GAMEMODE_EXIT,
        OPEN_HOME,
        OPEN_DEVICE_FORM,
        XBOX_DENY,
        XBOX_ALLOW,
        SUSPEND_SERVICE,
        EXIT_SERVICE,
        QUERY_END_SESSION,
        END_SESSION,
        RESTART_SERVICE,
        DISCONNECT,
        RELOAD_SERVICE,
        SESSION_LOCK,
        SESSION_UNLOCK
    };

    static const char * AppEventsName(AppEvents code)
    {
        switch (code)
        {
            case AppEvents::CONNECT:            return "CONNECT";
            case AppEvents::START:              return "START";
            case AppEvents::START_APPS:         return "START_APPS";
            case AppEvents::XBOX_DETECTED:      return "XBOX_DETECTED";
            case AppEvents::LAUNCHER_STOPPED:   return "LAUNCHER_STOPPED";
            case AppEvents::GAMEMODE_ENTER:     return "GAMEMODE_ENTER";
            case AppEvents::GAMEMODE_EXIT:      return "GAMEMODE_EXIT";
            case AppEvents::OPEN_HOME:          return "OPEN_HOME";
            case AppEvents::OPEN_DEVICE_FORM:   return "OPEN_DEVICE_FORM";
            case AppEvents::XBOX_DENY:          return "XBOX_DENY";
            case AppEvents::XBOX_ALLOW:         return "XBOX_ALLOW";
            case AppEvents::SUSPEND_SERVICE:    return "SUSPEND_SERVICE";
            case AppEvents::EXIT_SERVICE:       return "EXIT_SERVICE";
            case AppEvents::QUERY_END_SESSION:  return "QUERY_END_SESSION";
            case AppEvents::END_SESSION:        return "END_SESSION";
            case AppEvents::RESTART_SERVICE:    return "RESTART_SERVICE";
            case AppEvents::DISCONNECT:         return "DISCONNECT";
            case AppEvents::RELOAD_SERVICE:     return "RELOAD_SERVICE";
            case AppEvents::SESSION_LOCK:       return "SESSION_LOCK";
            case AppEvents::SESSION_UNLOCK:     return "SESSION_UNLOCK";
        }
        return "UNKNOWN";
    }
}

using namespace AnyFSE::App;
