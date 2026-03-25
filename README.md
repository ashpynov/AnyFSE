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

## Kudos

Way how to create home app was inspired by @driver1998 work https://github.com/driver1998/FullScreenExperienceShell. Also thanks to discord user 'silicon' who show me that project.

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

> [!WARNING]
> ### Microsoft Defender flag
> The AnyFSE.Installer.exe randomly may triggers a Defender detection: Trojan:Win32/Sabsik.FL.A!ml or Program:Script/Wacapew.A!ml, or other *!ml signatures (depends from the Moon phase).
>
> As soon as software packaged as a portable .exe or a .zip file that extracts and runs itself can be flagged because this is a common malware delivery method. The act of self-extraction is seen as suspicious.
>
> So You may add it to exclusion list or proceed to "Manual installation" section.
>
> P.S. If you know how to deal with this  - let me know.

### How to install

Just Launch AnyFSE.Installer.exe. Wait few seconds to complete and Configure.

You launcher should be installed additionally.

Please note: that AnyFSE work only when it is selected as home application in Settings->Gaming->Full screen experience.

> [!IMPORTANT]
> As soon as package should be signed to be selectable as full screen home application, and I have not ability to got code signin certificate from trusted authorities. So I had to use self-signed certificate.
> Installer will add it to trusted authorities section automatically.
> If you would like to revoke it:
> 1. Run Certlm.msc.
> 2. Open Trusted Root Certification Authorities -> Certificates
> 3. Find "DDCC7751-898D-4BC9-B80C-4AA73E5D5762" (Artem Shpynov Code signin) certificate -> press right mouse button -> Delete

### Manual installation

In case if your antivirus still blames on AnyFSE.Installer.exe file - you may install package manualy. (Same action as installer do):

1. Uninstall AnyFSE pre-0.90 version to avoid conflicting.
2. Install Artem Shpynov Root certificate:
- Download certificate: https://github.com/ashpynov/AnyFSE/releases/download/v0.90.1/Artem.Shpynov.cer and open it.
- Press 'Install certificate',
- Select store location 'Local Machine',
- Press 'Next'
- Allow changes
- Select 'Place all certificates in the folowing store'
- Press 'Browse' and select 'Trusted Root Certificate Authorities' and press OK and then Next.
- Certificate installation is Done. It is enougth to do it once.
3. Allow installation of packages in developer mode:
- Open Settings -> System -> Advanced and turn on 'Developer Mode'
- You can disable it after installation
4. Now you can dowload and install appx package https://github.com/ashpynov/AnyFSE/releases/download/v*version*/AnyFSE-*version*.appx
5. After installing you may turn off developer mode.


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
