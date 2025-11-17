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
