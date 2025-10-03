#pragma once
#include <vector>
#include <functional>
#include <gamingexperience.h>
#include "Event.hpp"

namespace AnyFSE::Monitors
{
    class GamingExperience
    {
    private:
        GAMING_FULL_SCREEN_EXPERIENCE_REGISTRATION fseHandle;

        static void Callback(GamingExperience * This);

    public:
        static bool ApiIsImplemented;
        static bool IsActive();
        GamingExperience();
        ~GamingExperience();

        Event OnExperienseChanged;

    };
}

using namespace AnyFSE::Monitors;