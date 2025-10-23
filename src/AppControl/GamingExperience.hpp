#pragma once
#include <vector>
#include <functional>
#include <gamingexperience.h>
#include "Tools/Event.hpp"

namespace AnyFSE::App::AppControl
{
    class GamingExperience
    {
    private:
        GAMING_FULL_SCREEN_EXPERIENCE_REGISTRATION m_fseHandle;

        static void Callback(GamingExperience * This);

    public:
        static bool ApiIsAvailable;
        static bool IsActive();
        GamingExperience();
        ~GamingExperience();

        Event OnExperienseChanged;

    };
}

using namespace AnyFSE::App::AppControl;