// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include "Event.hpp"
#include "Logging/LogManager.hpp"

namespace AnyFSE::Tools
{
    static Logger log = LogManager::GetLogger("Event");

    void Event::operator=(std::function<void()> callback)
    {
        callbacks.clear();
        callbacks.push_back(callback);
    }

    void Event::operator+=(std::function<void()> callback)
    {
        callbacks.push_back(callback);
    }

    // Remove callback (equivalent to C# -=)
    void Event::operator-=(const std::function<void()>& callback)
    {
        callbacks.erase(
            std::remove_if(callbacks.begin(), callbacks.end(),
                [&callback](const std::function<void()>& func) {
                    // Compare the target addresses (limited but works for some cases)
                    return func.target_type() == callback.target_type() &&
                           func.target<void()>() == callback.target<void()>();
                }),
            callbacks.end()
        );
    }

    void Event::Notify()
    {
        for (auto& callback : callbacks)
        {
            if (callback)
            {
                callback();
            }
        }
    }

}