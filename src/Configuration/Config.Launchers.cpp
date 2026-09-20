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

#include <filesystem>
#include "Config.hpp"
#include "Tools/Unicode.hpp"

namespace AnyFSE::Configuration
{
    static std::list<LauncherConfig> InitSupportedLaunchers()
    {
        std::list<LauncherConfig> result;
        {
            LauncherConfig PlayniteFullscreen = {};
            PlayniteFullscreen.Type = LauncherType::PlayniteFullscreen;
            PlayniteFullscreen.Name = L"Playnite Fullscreen";
            PlayniteFullscreen.URL = L"https://playnite.link/download/PlayniteInstaller.exe";
            PlayniteFullscreen.StartCommand = L"Playnite.FullscreenApp.exe";
            PlayniteFullscreen.StartArg = L"--hidesplashscreen";
            PlayniteFullscreen.ExStyle = WS_EX_APPWINDOW;
            PlayniteFullscreen.ProcessName = L"Playnite.FullscreenApp.exe"; // ProcessName
            PlayniteFullscreen.ExStyleAlt = WS_EX_APPWINDOW;
            PlayniteFullscreen.ProcessNameAlt = L"Playnite.DesktopApp.exe"; // ProcessNameAlt
            PlayniteFullscreen.ActivationProtocol = L"@";
            result.push_back(PlayniteFullscreen);
        }
        {
            LauncherConfig PlayniteDesktop = {};
            PlayniteDesktop.Type = LauncherType::PlayniteDesktop;
            PlayniteDesktop.Name = L"Playnite Desktop";
            PlayniteDesktop.URL = L"https://playnite.link/download/PlayniteInstaller.exe";
            PlayniteDesktop.StartCommand = L"Playnite.DesktopApp.exe";
            PlayniteDesktop.StartArg = L"--hidesplashscreen";
            PlayniteDesktop.ExStyle = WS_EX_APPWINDOW;
            PlayniteDesktop.ProcessName = L"Playnite.DesktopApp.exe";       // ProcessName
            PlayniteDesktop.ExStyleAlt = WS_EX_APPWINDOW;
            PlayniteDesktop.ProcessNameAlt = L"Playnite.FullscreenApp.exe"; // ProcessNameAlt
            PlayniteDesktop.ActivationProtocol = L"@";
            result.push_back(PlayniteDesktop);
        }
        {
            LauncherConfig SteamBigPicture = {};
            SteamBigPicture.Type = LauncherType::SteamBigPicture;
            SteamBigPicture.Name = L"Steam Big Picture";
            SteamBigPicture.URL = L"https://store.steampowered.com/about/";
            SteamBigPicture.StartCommand = L"Steam.exe";
            SteamBigPicture.StartArg = L"steam://open/bigpicture";
            SteamBigPicture.ExStyle = WS_EX_APPWINDOW;
            SteamBigPicture.NoStyle = WS_THICKFRAME;
            SteamBigPicture.ProcessName = L"steamwebhelper.exe";
            SteamBigPicture.ClassName = L"SDL_app";
            SteamBigPicture.IsTrayAggressive = true;
            result.push_back(SteamBigPicture);
        }
        {
            LauncherConfig Steam = {};
            Steam.Type = LauncherType::Steam;
            Steam.Name = L"Steam Desktop";
            Steam.URL = L"https://store.steampowered.com/about/";
            Steam.StartCommand = L"Steam.exe";
            Steam.StartArg = L"";
            Steam.ExStyle = WS_EX_APPWINDOW;
            Steam.ProcessName = L"steamwebhelper.exe";
            Steam.ClassName = L"SDL_app";
            Steam.IsTrayAggressive = true;
            result.push_back(Steam);
        }
        {
            LauncherConfig BigBox = {};
            BigBox.Type = LauncherType::BigBox;
            BigBox.Name = L"BigBox";
            BigBox.URL = L"https://www.launchbox-app.com/download";
            BigBox.StartCommand = L"BigBox.exe";
            BigBox.ExStyle = WS_EX_APPWINDOW;
            BigBox.ProcessName = L"BigBox.exe";
            result.push_back(BigBox);
        }
        {
            LauncherConfig OneGameLauncher = {};
            OneGameLauncher.Type = LauncherType::OneGameLauncher;
            OneGameLauncher.Name = L"One Game Launcher";
            OneGameLauncher.URL = L"https://ogl.app/";
            OneGameLauncher.StartCommand = L"ogl://";
            OneGameLauncher.NoStyle = WS_VISIBLE;
            OneGameLauncher.ProcessName = L"OneGameLauncher.exe";
            OneGameLauncher.ClassName = L"Windows.UI.Core.CoreWindow";
            OneGameLauncher.ActivationProtocol = L"@";
            OneGameLauncher.AppUserModelID = L"62269AlexShats.OneGameLauncher_gghb1w55myjr2!App";
            result.push_back(OneGameLauncher);
        }
        {
            LauncherConfig PocketDeck = {};
            PocketDeck.Type = LauncherType::PocketDeck;
            PocketDeck.Name = L"PocketDeck";
            PocketDeck.URL = L"https://www.pocketdeckapp.com/";
            PocketDeck.StartCommand = L"PocketDeck.exe";
            PocketDeck.ProcessName = L"PocketDeck.exe";
            result.push_back(PocketDeck);
        }
        {
            LauncherConfig RetroBat = {};
            RetroBat.Type = LauncherType::RetroBat;
            RetroBat.Name = L"RetroBat";
            RetroBat.URL = L"https://www.retrobat.org/download/";
            RetroBat.StartCommand = L"RetroBat.exe";
            RetroBat.ProcessName = L"emulationstation.exe";
            RetroBat.ClassName = L"SDL_app";
            RetroBat.ProcessNameAlt = L"retrobat.exe";
            result.push_back(RetroBat);
        }
        {
            LauncherConfig ArmouryCrate = {};
            ArmouryCrate.Type = LauncherType::ArmouryCrate;
            ArmouryCrate.Name = L"Armoury Crate SE";
            ArmouryCrate.URL = L"https://armoury-crate.com/#download";
            ArmouryCrate.StartCommand = L"asusac://";
            ArmouryCrate.NoStyle = WS_VISIBLE;
            ArmouryCrate.ProcessName = L"ArmouryCrateSe.exe";
            ArmouryCrate.ClassName = L"Windows.UI.Core.CoreWindow";
            ArmouryCrate.ActivationProtocol = L"@";
            ArmouryCrate.AppUserModelID = L"B9ECED6F.ArmouryCrateSE_qmba6cd70vzyy!App";
            result.push_back(ArmouryCrate);
        }
        {
            LauncherConfig Kodi = {};
            Kodi.Type = LauncherType::Kodi;
            Kodi.Name = L"Kodi";
            Kodi.URL = L"https://kodi.tv/download/windows/";
            Kodi.StartCommand = L"kodi.exe";
            Kodi.ProcessName = L"kodi.exe";
            Kodi.ClassName = L"Kodi";
            result.push_back(Kodi);
        }
        {
            LauncherConfig Cortex = {};
            Cortex.Type = LauncherType::Cortex;
            Cortex.Name = L"Razer Cortex";
            Cortex.URL = L"https://www.razer.com/cortex";
            Cortex.StartCommand = L"RazerCortex.Shell.exe";
            Cortex.ProcessName = L"RazerCortex.Shell.exe";
            Cortex.ClassName = L"RazerCortexMainWnd";
            result.push_back(Cortex);
        }
        // {
        //     LauncherConfig Xbox = {};
        //     Xbox.Type = LauncherType::Native;
        //     Xbox.Name = L"Xbox";
        //     Xbox.AppUserModelID = L"Microsoft.GamingApp_8wekyb3d8bbwe!Microsoft.Xbox.App";
        //     Xbox.StartCommand = Xbox.AppUserModelID;
        //     result.push_back(Xbox);
        // }
        return result;
    }

    std::list<LauncherConfig> Config::LauncherConfigs = InitSupportedLaunchers();

    LauncherType Config::GetConfiguredLauncher(const std::wstring& startCommand, const std::wstring& startArg)
    {
        if (startCommand.empty())
            return LauncherType::None;

        const auto command = Unicode::to_lower(startCommand);
        const auto filename = Unicode::to_lower(std::filesystem::path(startCommand).filename().wstring());
        const auto arguments = Unicode::to_lower(startArg);
        LauncherType fallback = LauncherType::Custom;

        for (const auto& launcher : LauncherConfigs)
        {
            const auto presetCommand = Unicode::to_lower(launcher.StartCommand);
            if (presetCommand != command && (filename.empty() || presetCommand != filename))
                continue;

            // Keep the existing preset when arguments are missing or customized.
            if (fallback == LauncherType::Custom)
                fallback = launcher.Type;
            if (Unicode::to_lower(launcher.StartArg) == arguments)
                return launcher.Type;
        }
        return fallback;
    }

    LauncherConfig Config::GetLauncherDefaults(LauncherType type, const std::wstring& path)
    {
        LauncherConfig launcher;
        GetLauncherDefaults(type, path, launcher);
        return launcher;
    }

    bool Config::GetLauncherDefaults(LauncherType type, const std::wstring& path, LauncherConfig& out)
    {
        out = LauncherConfig();
        out.Type = type;
        if (type == LauncherType::None)
        {
            out.Name = L"None";
            return true;
        }

        bool recognized = false;
        for (const auto& launcher : LauncherConfigs)
        {
            if (launcher.Type == type && (type != LauncherType::Native
                || Unicode::to_lower(launcher.StartCommand) == Unicode::to_lower(path)))
            {
                out = launcher;
                recognized = true;
                break;
            }
        }

        if (type == LauncherType::Custom)
        {
            out.StartCommand = path;
            out.IsCustom = true;
            out.Name = GetApplicationName(path);
            out.ProcessName = std::filesystem::path(path).filename().wstring();
        }

        // Only file-based presets use an installation directory. Custom commands and native IDs are already complete.
        if (!path.empty() && recognized && type != LauncherType::Native
            && out.StartCommand.find(L"://") == std::wstring::npos)
            out.StartCommand = (std::filesystem::path(path) / out.StartCommand).wstring();
        if (out.IconFile.empty())
            out.IconFile = out.AppUserModelID.empty() ? out.StartCommand : L"@" + out.AppUserModelID;
        return recognized;
    }
}
