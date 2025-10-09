#include "Event.hpp"
#include "Logging/LogManager.hpp"

namespace AnyFSE::Tools
{
    static Logger log = LogManager::GetLogger("Event");

    void Event::operator+=(std::function<void()> callback)
    {
        log.Info("Added notification handler");
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