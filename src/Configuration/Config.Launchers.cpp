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
    const std::map<LauncherType, LauncherConfig> Config::LauncherConfigsMap = std::map<LauncherType, LauncherConfig>{
        {
            LauncherType::PlayniteFullscreen, {
                LauncherType::PlayniteFullscreen,
                L"Playnite Fullscreen",
                L"Playnite.FullscreenApp.exe",
                L"--hidesplashscreen",
                WS_EX_APPWINDOW, 0,                 // ExStyle, NoStyle
                L"Playnite.FullscreenApp.exe",      // ProcessName
                L"",                                // ClassName
                L"",                                // WindowsTitle
                WS_EX_APPWINDOW, 0,                 // ExStyleAlt, NoStyleAlt
                L"Playnite.DesktopApp.exe",         // ProcessNameAlt
                L"",                                // ClassNameAlt
                L"",                                // WindowTitleAlt
            }
        },
        {
            LauncherType::PlayniteDesktop, {
                LauncherType::PlayniteDesktop,
                L"Playnite Desktop",
                L"Playnite.DesktopApp.exe",
                L"--hidesplashscreen",
                WS_EX_APPWINDOW, 0,
                L"Playnite.DesktopApp.exe",
                L"",
                L"",
                WS_EX_APPWINDOW, 0,
                L"Playnite.FullscreenApp.exe",
                L"",
                L"",
            }
        },
        {
            LauncherType::Steam, {
                LauncherType::Steam,
                L"Steam Big Picture Mode",
                L"Steam.exe",
                L"-gamepadui",
                WS_EX_APPWINDOW, WS_THICKFRAME,
                L"steamwebhelper.exe",
                L"SDL_app",
                L"",
                0, 0,
                L"",
                L"",
                L"",
                L"",
                true
            }
        },
        {
            LauncherType::BigBox, {
                LauncherType::BigBox,
                L"BigBox",
                L"BigBox.exe",
                L"",
                WS_EX_APPWINDOW, 0,
                L"BigBox.exe"
            }
        },
        {
            LauncherType::OneGameLauncher, {
                LauncherType::OneGameLauncher,
                L"One Game Launcher",
                L"ogl://",
                L"",
                0, WS_VISIBLE,
                L"OneGameLauncher.exe",
                L"Windows.UI.Core.CoreWindow",
                L"",
                0, 0,
                L"",
                L"",
                L"",
                L"@62269AlexShats.OneGameLauncher_gghb1w55myjr2\\Assets\\StoreLogo.png",
                false,
                L"@",
            }
        },
        {
            LauncherType::RetroBat, {
                LauncherType::RetroBat,
                L"RetroBat",
                L"RetroBat.exe",
                L"",
                0, 0,
                L"emulationstation.exe",
                L"SDL_app",
                L"",
                0, 0,
                L"retrobat.exe"
            }
        },
        {
            LauncherType::ArmouryCrate, {
                LauncherType::ArmouryCrate,
                L"Armoury Crate",
                L"asusac://",
                L"",
                0, WS_VISIBLE,
                L"ArmouryCrateSe.exe",
                L"Windows.UI.Core.CoreWindow",
                L"",
                0, WS_VISIBLE,
                L"",
                L"",
                L"",
                L"@B9ECED6F.ArmouryCrateSE_qmba6cd70vzyy\\Assets\\AppIcon\\Square44x44Logo.scale-400.png",
                false,
                L"@"
            }
        },
        {
            LauncherType::Xbox, {
                LauncherType::Xbox,
                L"Xbox",
                L"XBoxPcApp.exe"
            }
        },

    };

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
        for (auto it = Config::LauncherConfigsMap.begin(); it != Config::LauncherConfigsMap.end(); ++it)
        {
            if (Unicode::to_lower(it->second.StartCommand) == exe)
            {
                out = it->second;

                out.StartCommand = path;
                if (out.IconFile.empty())
                {
                    out.IconFile = path;
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
