# AnyFSE Home Application
![DownloadCountTotal](https://img.shields.io/github/downloads/ashpynov/AnyFSE/total?label=total%20downloads&style=plastic) ![DownloadCountLatest](https://img.shields.io/github/downloads/ashpynov/AnyFSE/latest/total?style=plastic) ![LatestVersion](https://img.shields.io/github/v/tag/ashpynov/AnyFSE?label=Latest%20version&style=plastic) ![License](https://img.shields.io/github/license/ashpynov/AnyFSE?style=plastic)

The AnyFSE Home application aims to give users the ability to use their favorite launchers as Home applications for Gaming Full Screen Experience mode on modern Windows.

[Latest Release](https://github.com/ashpynov/AnyFSE/releases/latest)

[Help and Discussions](https://discord.gg/AfkERzTEut)

Currently, there is no official way available for third-party developers to allow their users to choose custom launchers like Playnite, Steam Big Picture mode, LaunchBox, etc. as Home Applications for Fullscreen Experience mode.
However, I hope that Microsoft will change their mind and provide an official API to do so.


## Install and Uninstall

To install AnyFSE, simply extract content to any desired location and launch it. In the settings dialog, choose the application to use and press 'Save'.

If you change your mind and decide to move the file to another location, just put it there, run it, and press Save.

If AnyFSE appears buggy, glitchy, heavy, or you have any other reason to uninstall it, simply open settings and press uninstall.

### Manual Uninstallation
To manually remove AnyFSE, you need to Open Task Scheduler (press the Windows key and type Task Scheduler) and delete the task 'AnyFSE home application' from Task Scheduler Library.


### User Account Control
Since adding tasks to Task Scheduler and changing the registry require escalated privileges (running as Administrator), you will be prompted for administrator permissions when opening settings.

> [!WARNING]
> ### Microsoft Defender flag
> The tool randomly may triggers a Defender detection: Trojan:Win32/Sabsik.FL.A!ml or Program:Script/Wacapew.A!ml, or other *!ml signatures (depends from the Moon phase).
>
> As soon as tool working from behalf of SYSTEM user, monitors access to registry, kill some applications and start other - some silly Machine Learning signatures may consider as it harmfull trojan. Even it it does not contain any network related code.
>
> But you don’t have to trust me — feel free to review the code and build it yourself.
>
> Anyway, even builded yourself binary will trigger . So it is better to create dedicated folder
> for AnyFSE, and add this folder to MS Defender exclusions before installation.
>
> P.S. If you know how to deal with this  - let me know.

## Splash Videos
AnyFSE may show shuffled video as splash during your launcher is loading.

To do this, Create folder 'splash' next to AnyFSE.exe and put there you favourite mp4 or webm videos. files will be shuffled each time splash screen is shown.

For sure it will be good idea to suppress native splash screens of launchers, to do so enable custom settings and add startup argument to prevent native splash (for Playnite it is ```--nosplash``` option).

### Splash Settings
There are few option to configure splash look and feel can be changed via json configuration file and file name of video:
- ```/Splash/Video/Path``` - **absolute** path to folder with splashes or your single video
- ```/Splash/Video/Loop``` - loop video if it is too short (false/true), by default video will be 'paused' on last frame
- ```/Splash/Video/Mute``` - mute video sound (false/true)

### Filename control
You can specify custom position of loop via filename. To do this - name should contain additional part before extension like:

```splash.m4000.mp4``` or ```other_splash.5000.webm``` here is ***m4000*** and ***5000*** instructions to
- 'm' or 'M' - mute video during loop
- '4000' and '5000' position in milliseconds from start of video to rewind to during loop.

# Limitations
### Xbox flashes for seconds
As soon as this tool does not actually ***replace*** XboxApp and does not so deeply integrated to system (e.g as driver) it cannot actually ***prevent*** XboxApp execution, and just track process appearence and kill it - there can be short 'flash' of Xbox.

### Xbox is sometimes executed
For sure I do not catch every case how Xbox may start in FSE way. And sometimes it bypass my filter (e.g. I saw how it is bypass via Update execution after Hibernating).

There is '***Agressive mode***' in my backlog to forcefully prevent **Any** XboxPcApp appearence, but in will make impossible to work with Xbox at all.


# Technical Description

AnyFSE works with 2 instances:

## Service Instance
This instance is executed by Task Scheduler at system startup under the SYSTEM user with administrator privileges. This instance monitors user session execution (user logon) to start the second 'Application' instance. This is the earliest possible way to run an application, even before the Shell starts, without using dirty registry hacks.

The main goals of the service instance are to monitor 3 things:

- XboxPcApp.exe process execution
- GamingHomeApp registry key access by Explorer.exe
- DeviceForm registry key access by Explorer.exe

When any of these are detected, it notifies the second 'Application' instance via an IPC channel.

This instance also has privileged permissions, so it receives commands from the Application instance to prevent XboxPcApp process startup.

## Application (Control) Instance

This instance is executed by the service instance on user logon (or can be executed ad-hoc by the user for configuration).

This instance runs with normal user privileges, so it cannot prevent XboxPcApp directly. However, it contains:

- A splash screen to show while the desired launcher is starting (otherwise the shell will continuously try to start Xbox)
- All logic for reacting to events and commanding the Service
- Settings page

## Logic

The logic is simple: detect Xbox App => kill it and start the Launcher instead. However, reality is a bit different due to:

* Starting the launcher takes time, especially on first launch, particularly while Windows is starting other processes. The Launcher window may appear after 10-30 seconds.

* The shell will restart Xbox if there are no other windows visible.

* We also want to have the ability to start Xbox when desired.

* There are 'Home' / 'Library' buttons on the Game bar to start the Launcher.

The actual logic is:

1. Detect DeviceForm and GamingHomeApp registry access. The shell checks DeviceForm just before GamingHomeApp in case it needs to show it in Task Switcher and doesn't check it before executing the Home App from the Game Bar.

2. During launcher startup and for 3 seconds after - prevent Xbox app execution by killing it, and show the splash window to prevent the shell from restarting it.

3. 'Launcher has been started' is detected when a process with the specified name is present and a window with a specific title owned by this process is visible. To support Playnite's two modes, pairs of names are tracked for both fullscreen and desktop modes.


4. If Xbox is executed without reading GamingHomeApp first - allow it and also skip the next access to GamingHomeApp for 2 seconds (Xbox checks it during startup too).
