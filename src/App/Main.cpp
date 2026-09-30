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

#include <windows.h>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include "resource.h"
#include <tchar.h>
#include <commctrl.h>
#include <strsafe.h>

#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Process.hpp"
#include "Tools/Elevated.hpp"
#include "Tools/Notification.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Localization.hpp"
#include "Tools/Unicode.hpp"

#include "App/App.hpp"
#include "App/CmdLine.hpp"
#include "App/App.hpp"
#include "App/Constants.hpp"
#include "App/GamingExperience.hpp"
#include "App/ExitFSE.hpp"
#include "App/MainWindow.hpp"
#include "App/Launchers.hpp"
#include "App/JumpList.hpp"
#include "Ally/Ally.hpp"
#include "Ally/Handlers.hpp"

#include "Tools/Minidump.hpp"

using namespace AnyFSE;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    Tools::InstallUnhandledExceptionHandler();

    Logger log = LogManager::GetLogger("Main");

    Config::Load();

    LogManager::Initialize("AnyFSE", Config::LogLevel, Config::LogPath);
    log.Debug("\nApplication is started args: [%s]\n", lpCmdLine);

    int exitCode = 0;

    if (
           CmdLine::Elevated(lpCmdLine, exitCode)
        || CmdLine::HidListenerJob(lpCmdLine, exitCode)
        || CmdLine::HidListener(lpCmdLine, exitCode)
        || !App::ApiIsAvailable(hInstance)
        || GamingExperience::RestoreEnterFSEConfirmation()
        || Ally::EnsureListener()
        || !Localization::Initialize(Config::Locale)
        || JumpList::RegisterJumpList()
        || CmdLine::FSE(lpCmdLine, exitCode)
        || CmdLine::Settings(lpCmdLine, exitCode)
        || Window::IsRegistered()
    )
    {
        log.Debug("Early exit args: [%s] with exit code %d", lpCmdLine, exitCode);
        return exitCode;
    }

    bool isRestarting = !App::IsFirstLaunch() && App::IsRestarted();

    if (GamingExperience::IsFullscreenMode() && App::IsFirstLaunch())
    {
        App::RunStartupApps();
    }

    if (Launchers::IsLauncherActiveOrMinimized())
    {
        Launchers::FocusLauncher();
        ExitFSE::WaitHomeAppExit();
        return 0;
    }

    if (ExitFSE::WaitExitFSEMode())
    {
        return 0;
    }

    if (!isRestarting)
    {
        Config::Launcher.OnBoot.Notify();
        Launchers::StartLauncher(Config::AsAdmin);
    }

    {
        MainWindow mainWindow(Constants::MainWindowClass, hInstance);

        do
        {
            if (isRestarting)
            {
                Config::Launcher.OnRestating.Notify();

                if (Config::SplashShowVideo && Config::SplashSimplifiedSwitch)
                {
                    Config::SplashShowVideo = false;
                    Config::SplashShowLogo = true;
                    Config::SplashShowAnimation = true;
                    Config::SplashShowText = Config::SplashSimplifiedSwitchText;
                }
            }

            exitCode = mainWindow.Run(TranslateF(L"splashIsLaunchingFmt", Config::Launcher.Name.c_str()));

            isRestarting = ExitFSE::WaitHomeAppExit();

            if (isRestarting)
            {
                log.Debug("Launcher restart detected, re-loop window");
            }
        } while (isRestarting);
    }

    log.Debug("AnyFSE going to finish");

    if (exitCode)
    {
        log.Warn(log.APIError(exitCode), "Exiting with code: (%d) error", exitCode);
    }
    else
    {
        log.Debug("AnyFSE Job is done! \n\n");
    }

    GamingExperience::PreventPopuppers();
    return (int)exitCode;
}
