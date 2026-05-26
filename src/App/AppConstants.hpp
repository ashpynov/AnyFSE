#pragma once

namespace AnyFSE::AppConstants
{
    // Package / identity
    inline constexpr wchar_t PackageFamilyName[] = L"ArtemShpynov.AnyFSE_by4wjhxmygwn4";
    inline constexpr wchar_t AppUserModelId[] = L"ArtemShpynov.AnyFSE_by4wjhxmygwn4!App";
    inline constexpr wchar_t PackageAtomName[] = L"ArtemShpynov.AnyFSE_by4wjhxmygwn4";
    inline constexpr wchar_t WaitingExitMutex[] = L"ArtemShpynov.AnyFSE_WaitingExitFSE";

    // Product files
    inline constexpr wchar_t AnyFseSettingsDll[] = L"AnyFSE.Settings.dll";
    inline constexpr wchar_t InstallerExe[] = L"AnyFSE.Installer.exe";
    inline constexpr wchar_t InjectorExe[] = L"AnyFSE.ACSEFilterInjector.exe";

    // ASUS / ACSE integration
    inline constexpr wchar_t InjectorServiceName[] = L"ACSEFilterInjector";
    inline constexpr wchar_t AsusOptimizationService[] = L"ASUSOptimization";
    inline constexpr wchar_t AsusOptimizationProcess[] = L"AsusOptimization.exe";
    inline constexpr wchar_t ArmouryCrateServiceProcess[] = L"ArmouryCrateSE.Service.exe";

    // Registry
    inline constexpr wchar_t UninstallAnyFseRegKey[] = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AnyFSE";
    inline constexpr wchar_t GamingHomeAppRegKey[] = L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\GamingConfiguration";
    inline constexpr wchar_t GamingHomeAppRegValue[] = L"GamingHomeApp";

    // Installer / updater assets
    inline constexpr wchar_t PublisherCertFile[] = L"Artem.Shpynov.cer";
    inline constexpr wchar_t AppxFilePrefix[] = L"AnyFSE-";
    inline constexpr wchar_t TempInstallDirName[] = L"AnyFSE_install";
    inline constexpr wchar_t GitHubReleaseRoot[] = L"https://github.org/ashpynov/AnyFSE/releases/download/v";
    inline constexpr wchar_t CodebergReleaseRoot[] = L"https://codeberg.org/ashpynov/AnyFSE/releases/download/v";
    inline constexpr wchar_t UpdaterCommandMessage[] = L"AnyFSE.Updater.Command";
    inline constexpr wchar_t AnyFseProtocolSettings[] = L"anyfse://settings";
    inline constexpr wchar_t AnyFseProtocolAllyHid[] = L"anyfse://AllyHid";
    inline constexpr wchar_t MainWindowClass[] = L"AnyFSE";
    inline constexpr wchar_t SettingsDialogClass[] = L"AnyFSESettingsDialogClass";
    inline constexpr wchar_t SettingsLineClass[] = L"AnyFSE_SettingsLineClass";
    inline constexpr wchar_t UpdaterNotifyWindowClass[] = L"AnyFSE_Updater_NotifyWnd";
    inline constexpr wchar_t UpdaterUserAgentHeader[] = L"User-Agent: AnyFSE-Updater\r\n";
    inline constexpr wchar_t UpdaterGitHubAcceptHeader[] = L"Accept: application/vnd.github.v3+json\r\n";
    inline constexpr wchar_t UpdaterSessionUserAgent[] = L"AnyFSE-Updater/1.0";
    inline constexpr wchar_t UpdaterTempExePrefix[] = L"AnyFSE.";
    inline constexpr wchar_t UpdaterTempExeSuffix[] = L".Update.exe";

}
