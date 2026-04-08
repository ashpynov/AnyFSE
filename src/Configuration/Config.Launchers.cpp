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
            result.push_back(PlayniteDesktop);
        }
        {
            LauncherConfig Steam = {};
            Steam.Type = LauncherType::Steam;
            Steam.Name = L"Steam Big Picture Mode";
            Steam.URL = L"https://store.steampowered.com/about/";
            Steam.StartCommand = L"Steam.exe";
            Steam.StartArg = L"steam://open/bigpicture";
            Steam.ExStyle = WS_EX_APPWINDOW;
            Steam.NoStyle = WS_THICKFRAME;
            Steam.ProcessName = L"steamwebhelper.exe";
            Steam.WindowTitle = L"SDL_app";
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

    bool Config::GetLauncherDefaults(const std::wstring& path, LauncherConfig& out)
    {
        namespace fs = std::filesystem;
        out = LauncherConfig();

        if (path.empty())
        {
            out.Type = None;
            out.Name = L"None";
            return true;
        }

        std::wstring exe = Unicode::to_lower(fs::path(path).filename().wstring());
        if (exe.empty())
        {
            exe = Unicode::to_lower(path);
        }
        for (auto it = Config::LauncherConfigs.begin(); it != Config::LauncherConfigs.end(); ++it)
        {
            if (Unicode::to_lower(it->StartCommand) == exe)
            {
                out = *it;

                out.StartCommand = path;
                if (out.IconFile.empty())
                {
                    out.IconFile = out.AppUserModelID.empty() ? path : L"@" + out.AppUserModelID;
                }
                return true;
            }
        }

        out.Name = Config::GetApplicationName(path);
        out.IconFile = path;
        out.StartCommand = path;
        out.ProcessName = fs::path(path).filename().wstring();
        out.IsCustom = true;
        out.Type = Custom;

        return false;

    }
}
