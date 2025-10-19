#include <apiquery2.h>
#include <libloaderapi.h>
#include <libloaderapi2.h>
#include <gamingexperience.h>
#include "Logging/LogManager.hpp"
#include "GamingExperience.hpp"

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "onecore.lib")

namespace AnyFSE::Monitors
{
    static Logger log = LogManager::GetLogger("GamingExperience");

    bool isApiSetImplemented( const std::string& api )
    {
        return  ::IsApiSetImplemented(api.c_str())
            && QueryOptionalDelayLoadedAPI(
                GetModuleHandle(NULL),
                (api + ".dll").c_str(),
                "RegisterGamingFullScreenExperienceChangeNotification",
                0
            );
    }

    bool GamingExperience::ApiIsAvailable = isApiSetImplemented("api-ms-win-gaming-experience-l1-1-0");

    bool GamingExperience::IsActive()
    {
        return ApiIsAvailable && IsGamingFullScreenExperienceActive();
    }

    GamingExperience::GamingExperience()
    {
        m_fseHandle = nullptr;
        if (ApiIsAvailable)
        {
            RegisterGamingFullScreenExperienceChangeNotification((VOID(CALLBACK *)(LPVOID))Callback, this, &m_fseHandle);
            if (m_fseHandle != nullptr)
            {
                log.Debug(
                    "FullScreenExperienceChangeNotification is registered\n"
                    "Current mode is %s\n",
                    GamingExperience::IsActive() ? "Fullscreeen experience" : "Windows Desktop"
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
        if (m_fseHandle != nullptr && ApiIsAvailable)
        {
            UnregisterGamingFullScreenExperienceChangeNotification(m_fseHandle);
        }
    }
}
