
#include <apiquery2.h>
#include <gamingexperience.h>
#include "Logging/LogManager.hpp"
#include "GamingExperience.hpp"

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "onecore.lib")

namespace AnyFSE::Monitors
{
    static Logger log = LogManager::GetLogger("GamingExperience");

    bool GamingExperience::ApiIsImplemented = IsApiSetImplemented("api-ms-win-gaming-experience-l1-1-0");

    bool GamingExperience::IsActive()
    {
        return ApiIsImplemented && IsGamingFullScreenExperienceActive();
    }

    GamingExperience::GamingExperience()
    {
        fseHandle = nullptr;
        if (ApiIsImplemented)
        {
            RegisterGamingFullScreenExperienceChangeNotification((VOID(CALLBACK *)(LPVOID))Callback, this, &fseHandle);
            if (fseHandle != nullptr)
            {
                log.Info(
                    "FullScreenExperienceChangeNotification is registered\n"
                    "Current mode is %s\n",
                    GamingExperience::IsActive() ? "Fullscreeen expirience" : "Windows Desktop"
                );
            }
            else
            {
                log.Error(log.APIError(), "FullScreenExperienceChangeNotification registation failed");
            }
        }
    }
    //static 
    void GamingExperience::Callback(GamingExperience * This)
    {
        This->OnExperienseChanged.Notify();
    }
    
    GamingExperience::~GamingExperience()
    {
        if (fseHandle != nullptr && ApiIsImplemented)
        {
            UnregisterGamingFullScreenExperienceChangeNotification(fseHandle);
        }
    }
}
