# Build instructions

## Prerequisites

- Visual Studio 2022 with Desktop development with C++, MSVC toolset `v143`, and Windows SDK `10.0.26100.0`.
- A valid code-signing certificate with a private key in `Cert:\CurrentUser\My` for the existing EXE/DLL signing targets.
- Use `.vscode/tasks.json` as the source of truth for build commands, Visual Studio developer environment setup, configuration, and dependencies.

## Binary signing

EXE/DLL signing uses the exact subject `CN=$(BinarySigningCN)` (currently `CN=Artem Shpynov`), Code Signing usage, a private key, and a current validity period. If several certificates qualify, the certificate with the latest expiry is selected.

There is no APPX/MSIX archive to build or sign, and no package certificate to export, copy, or install. The manifest Publisher defines the application identity independently of `BinarySigningCN`.

For local testing, create a binary-signing certificate as the build user:

```powershell
New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=Artem Shpynov' `
    -CertStoreLocation 'Cert:\CurrentUser\My' -KeyAlgorithm RSA -KeyLength 4096 `
    -HashAlgorithm SHA256 -KeyUsage DigitalSignature -NotAfter (Get-Date).AddYears(1)
```

Keep private keys and any exported PFX files private. The installer does not import the binary-signing certificate.

## Application identity

`AppxManifest.xml` is a static manifest copied directly into the installer payload together with `CustomCapability.SCCD` and `Assets`. No separate registration project, MakeAppx, MakePri, or generated resource index is used. Its registration version is fixed at `1.0.0.0`, independently of the binary version; `ForceUpdateFromAnyVersion` is enabled when registering updated files.

Keep `Identity Name`, `Identity Publisher`, and `Application Id` stable for upgrades. If the identity changes, update `PackageFamilyName`, `AppUserModelId`, and `PackageAtomName` in `src\App\Constants.hpp`. A changed identity also changes the per-user package data directory.

The EXE no longer embeds the sparse-package `msix` identity manifest. Windows supplies identity when it launches the registered application; direct EXE launches use the existing fallback paths under the same package data directory.

For a fork, update `AssemblyCompany`, `AssemblyCopyright`, and `BinarySigningCN` in `AnyFSE.Version.props`. Update the manifest display strings separately. Also review release URLs and updater settings.

After registration, inspect the actual identity with:

```powershell
Get-AppxPackage -Name 'ArtemShpynov.AnyFSE' |
    Select-Object Name, Publisher, PackageFamilyName, PackageFullName, InstallLocation, IsDevelopmentMode
```

## Build the installer

Run `Build AnyFSE.Installer Offline` from `.vscode/tasks.json`. Project references in `AnyFSE.Installer.vcxproj` build the runtime binaries and signed uninstaller before creating the payload; the tasks do not orchestrate these dependencies. Use `Build AnyFSE.Installer Release` for the online installer, or `Build AnyFSE.Installer Debug` for Debug.

Expected outputs under `build\Release`:

- `AnyFSE.<version>.zip`
- `AnyFSE.<version>-<revision>.pdb.zip`
- `AnyFSE.Installer.Offline.<version>-<revision>.exe`

`AssemblyVersion` and `VersionRevision` in `AnyFSE.Version.props` determine binary and installer versions. After successfully building and signing the installer, `VersionRevision` is incremented for the next build. Both installer variants write `DisplayVersion` using the revision read at the start of the build.

## Installer payload and registration

`CreateInstallerArchives` recreates `build\<Configuration>\staging` and archives only that directory. After a successful Build, `CleanInstallerStaging` removes it. A failed build leaves it available for inspection.

The payload contains the five explicitly listed runtime binaries, `AppxManifest.xml`, `CustomCapability.SCCD`, assets, localizations, and `resources.pri`. MakePri generates the resource index from the staging directory so Windows can select icon sizes, scale and unplated variants, and theme variants for JumpList icons. No APPX/MSIX archive or package certificate is created. Required missing inputs stop the build; stale output files are not used to discover payload contents.

The installer copies these files into `%PROGRAMFILES%\AnyFSE` and registers `AppxManifest.xml` directly through the Windows Runtime API `PackageManager.RegisterPackageByUriAsync`, with `RegisterPackageOptions.DeveloperMode` and `ForceUpdateFromAnyVersion` enabled. Registration does not launch PowerShell or any other child process.

Developer Mode is enabled for registration if needed and restored to its previous state afterwards, including on failure. This permits the development SCCD used for `gamingHome`. Registration checks the deployment result and reports Windows deployment errors to the installer.

Full uninstall removes the current user's registration before deleting application files. The current uninstaller's `/u` mode removes the loose registration with `RemovalOptions::PreserveApplicationData`, preserves package data, and deletes installation files. It skips delayed self-deletion so the next installer can reuse the same installation path. `/u` is honored with or without `/s`. This also allows an older APPX installer to run after the loose registration has been removed. Older uninstallers skip package removal during updates.

The installer reads the installed package version through `Packages::GetPackageVersion` and compares it with `0.90.26` using the updater functions `ParseSemVer` and `SemVerGreater`, shared in `src\Updater\Version.hpp`. An absent package returns an empty version and skips migration.

The installer calls `Config::Load()` before removing the old version. For migration, it removes the old package, copies the application files, and registers the new manifest through Windows Runtime. After successful registration it calls `Config::Save()` to restore the loaded settings in `LocalState`. The installer and uninstaller do not access certificate stores.

When adding or renaming a runtime binary, update `ApplicationFiles` in `AnyFSE.Installer.vcxproj`. ZIP archives are created by MSBuild's `ZipDirectory`; the installer extracts them with vendored miniz 3.1.2. Offline installation reads the embedded RCDATA directly from memory.

## Microsoft Store listing

`XPFM6KZ5SD704G` is the product ID of the EXE installer listing, separate from the application identity. Rebuilding locally does not update the Store listing; publish a submission in Partner Center to update its installer URL.

## Scan the offline installer with VirusTotal

Set your VirusTotal API v3 key in the environment before starting VS Code, so the integrated terminal and tasks inherit it:

```powershell
$env:VIRUSTOTAL_API_KEY = '<your API key>'
code .
```

Run the `Check AnyFSE.Installer Offline with VirusTotal` task. It builds the offline installer, uploads the newest matching file from
`build\Release`, waits for the analysis, and fails if any engine reports it as `malicious` or `suspicious`.

The task uses the public VirusTotal file-upload API. Uploaded files may be shared with VirusTotal partners; do not use it for confidential builds.
The free public API is rate-limited, so the scan can take several minutes. The script also accepts an explicit file when run manually:

```powershell
.\scripts\Test-VirusTotal.ps1 -FilePath .\build\Release\AnyFSE.Installer.Offline.0.90.18-56.exe
```

## Disable update checks

For a private/offline build, disable update checks in code instead of relying only on user settings.

1. In `src\Configuration\Config.cpp`, make the default settings manual and silent:

```cpp
int  Config::UpdateCheckInterval = -2;     // Manual
bool Config::UpdateNotifications = false;  // No update notifications
```

In the same file, also keep the `/Update/CheckInterval` load fallback as `-2` and change the `/Update/Notifications` fallback to `false`.

2. In `src\AppSettings\SettingsDialog.Update.cpp`, comment out the manual button network trigger:

```cpp
void SettingsDialog::OnCheckUpdate()
{
    // Update checks are disabled for offline/private builds.
    // Updater::CheckUpdateAsync(Config::UpdatePreRelease, m_hDialog, WM_UPDATE_NOTIFICATION);
    // UpdateVersionStatusDelay(10);
}
```

3. In `src\AppSettings\SettingsPages\UpdatePage.cpp`, comment out the "On settings open" trigger:

```cpp
void UpdatePage::UpdateSettingsChangedCheck()
{
    UpdateSettingsChanged();

    // Update checks are disabled for offline/private builds.
    // if (Config::UpdateCheckInterval == -1)
    // {
    //     Updater::CheckUpdateAsync(Config::UpdatePreRelease, m_dialog.GetHwnd(), SettingsDialog::WM_UPDATE_NOTIFICATION);
    // }
}
```

4. In `src\Updater\Updater.cpp`, guard the scheduled/timed trigger as a no-op:

```cpp
int ScheduledCheckAsync(const std::wstring& lastCheck, int checkInterval, bool includePreRelease, HWND hWnd, UINT uMsg)
{
    // Update checks are disabled for offline/private builds.
    return checkInterval;

    /*
    if (checkInterval <= 0)
    {
        return checkInterval;
    }

    int since = GetSince(lastCheck);

    log.Debug("Check schedule interval: lastChecked=%s (%ds/%.2fh ago)", Unicode::to_string(lastCheck).c_str(), since, (float)since / 3600);

    if (since < checkInterval * 3600)
    {
        return checkInterval * 3600 - since;
    }

    log.Debug("Run check");

    CheckUpdateAsync(includePreRelease, hWnd, uMsg);
    return checkInterval * 3600;
    */
}
```

5. Optional cleanup: update or clear the release URLs in `src\App\Constants.hpp` and `src\Updater\Updater.cpp` so accidental future calls do not contact the upstream project.

Runtime/user setting for existing installs:

- Open Settings -> Update settings.
- Set Check period to Manual.
- Disable update notifications.
