# AnyFSE Home Application
![DownloadCountTotal](https://img.shields.io/github/downloads/ashpynov/AnyFSE/total?label=total%20downloads&style=plastic) ![DownloadCountLatest](https://img.shields.io/github/downloads/ashpynov/AnyFSE/latest/total?style=plastic) ![LatestVersion](https://img.shields.io/github/v/tag/ashpynov/AnyFSE?label=Latest%20version&style=plastic) ![License](https://img.shields.io/github/license/ashpynov/AnyFSE?style=plastic)

The AnyFSE Home application aims to give users the ability to use their favorite launchers as Home applications for Gaming Full Screen Experience mode on modern Windows.

[Latest Release](https://github.com/ashpynov/AnyFSE/releases/latest)

[Help and Discussions](https://discord.gg/AfkERzTEut)

AnyFSE can be selected as Home application for full screen experience and will execute users favourite launchers like Playnite, Steam Big Picture mode, LaunchBox, etc. in full screen experience mode.

Now AnyFSE able to support (alpabetical):

- LaunchBox BigBox
- One Game Launcher
- Playnite Fullscreen
- RetroBat
- Steam Big Picture Mode

Some other launchers potentially can be supported too with minor customizations

## How it is works

If AnyFSE is selected as home application:

1. Windows starts AnyFSE as full screen home application.
2. AnyFSE read configuration and start launcher selected by user.
3. Show splash screen (text or video).
4. Wait till launcher executed (try to detect it main window).
5. Close splash screen and exit

So no background services and processes. No Xbox app intersection etc etc.

Same for cases when AnyFSE executed from gamebar.


## Install, Configure and Uninstall

### How to install

Just Launch AnyFSE.Installer.exe. Wait few seconds to complete and Configure.

You launcher should be installed additionally.

Please note: that AnyFSE work only when it is selected as home application in Settings->Gaming->Full screen experience.

> [!IMPORTANT]
> As soon as package should be signed to be selectable as full screen home application, and I have not ability to got code signin certificate from trusted authorities - I had to use own signed certificate.
> Installer will add it to trusted authorities section automatically without ability to uninstall
> If you would like to revoke it:
> 1. Run Certlm.msc.
> 2. Open Trusted Root Certification Authorities -> Certificates
> 3. Find "Artem Shpynov" certificate -> press right mouse button -> Delete


### How to launch and configure

In start menu find AnyFSE application. Press right mouse key and choose 'Configure' task.

### How to uninstall it

Open Settings -> Apps -> Instaled apps.

Find AnyFSE and select "Uninstall"

> [!IMPORTANT]
> To revoke installed certificate:
> 1. Run Certlm.msc.
> 2. Open Trusted Root Certification Authorities -> Certificates
> 3. Find "Artem Shpynov" certificate -> press right mouse button -> Delete

## Splash Videos
AnyFSE may show shuffled video as splash during your launcher is loading.

To do this, Create folder 'splash' in configuration folder (c:\ProgramData\AnyFSE) and put there you favourite mp4 or webm videos. Files will be shuffled each time splash screen is shown.

For sure it will be good idea to suppress native splash screens of launchers, to do so enable custom settings and add startup argument to prevent native splash (for Playnite it is ```--hidesplashscreen``` option).


### Filename control
You can specify custom position of loop via filename. To do this - name should contain additional part before extension like:

```splash.m4000.mp4``` or ```other_splash.5000.webm``` here is ***m4000*** and ***5000*** instructions to
- 'm' or 'M' - mute video during loop
- '4000' and '5000' position in milliseconds from start of video to rewind to during loop.

# Limitations

As soon as new version do not monitor any application execution and even do not run in background it is not possible to implement some features like 'Exit full screen experience on launcher exit' it is disabled for now.

Also application is require singned msix package. Currently I use self signed certifacate to be installed into trusted root authorities certificate storage.
