#include <apiquery2.h>
#include <libloaderapi.h>
#include <libloaderapi2.h>
#include <gamingexperience.h>
#include "Logging/LogManager.hpp"
#include "GamingExperience.hpp"


#pragma comment(lib, "delayimp.lib")
#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "onecore.lib")

namespace AnyFSE::App::AppControl
{
    static Logger log = LogManager::GetLogger("GamingExperience");

    bool isApiSetImplemented( const std::string& api )
    {
        HMODULE hDll = NULL;
        bool result = ::IsApiSetImplemented(api.c_str())
            && (hDll = LoadLibraryA((api + ".dll").c_str()))
            && GetProcAddress(hDll, "RegisterGamingFullScreenExperienceChangeNotification");
        if (hDll)
        {
            FreeLibrary(hDll);
        }
        return result;
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
                    "FullScreenExperienceChangeNotification is registered. "
                    "Current mode is %s",
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
