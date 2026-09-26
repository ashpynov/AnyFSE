#Requires -RunAsAdministrator
param([ValidateSet('Release', 'Debug-Isolated')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
$source = Join-Path $workspace ('build\' + $Configuration)
$destination = Join-Path $env:ProgramFiles 'AnyFSE'
$executable = Join-Path $destination 'AnyFSE.exe'
$resultFile = Join-Path $workspace 'build\installation-repair-result.txt'
$files = @('AnyFSE.exe', 'AnyFSE.Settings.dll', 'AnyFSE.ACSEFilterHook.dll', 'AnyFSE.ACSEFilterInjector.exe',
    'Localization\en_US.json', 'Localization\pt_BR.json')
$backup = Join-Path $workspace ('build\InstalledBackup-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$copied = @()
try {
    if (-not (Test-Path -LiteralPath $executable)) { throw 'Installed AnyFSE executable not found.' }
    foreach ($file in $files) {
        if (-not (Test-Path -LiteralPath (Join-Path $source $file))) { throw "Build artifact missing: $file" }
    }
    $boost = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\AnyFSE\GameBoost' -ErrorAction SilentlyContinue
    $services = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\AnyFSE\ServiceRestore' -ErrorAction SilentlyContinue
    if ($null -ne $boost.JournalV1) { throw 'GameBoost recovery is pending. Return to desktop and restore before replacing the monitor.' }
    foreach ($name in @('WSearch','SysMain','DiagTrack','MapsBroker')) {
        if ($null -ne $services.$name) { throw "Service recovery is pending: $name. Restore before replacing the monitor." }
    }
    $processes = @(Get-CimInstance Win32_Process -Filter "Name='AnyFSE.exe'" |
        Where-Object { $_.ExecutablePath -eq $executable })
    foreach ($process in $processes) {
        if ($process.CommandLine -notmatch '\s/OptimizationMonitor\s*$') {
            throw 'Close the AnyFSE settings/splash window before installing the repair.'
        }
    }
    foreach ($file in $files) {
        $old = Join-Path $destination $file
        if (Test-Path -LiteralPath $old) {
            $save = Join-Path $backup $file
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $save) | Out-Null
            Copy-Item -LiteralPath $old -Destination $save
        }
    }
    # Only stop the verified background monitor, and only with no pending recovery.
    foreach ($process in $processes) {
        Stop-Process -Id $process.ProcessId -ErrorAction Stop
        Wait-Process -Id $process.ProcessId -Timeout 10 -ErrorAction SilentlyContinue
    }
    foreach ($file in $files) {
        $target = Join-Path $destination $file
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Copy-Item -LiteralPath (Join-Path $source $file) -Destination $target -Force
        $copied += $file
        if ((Get-FileHash -LiteralPath $target).Hash -ne (Get-FileHash -LiteralPath (Join-Path $source $file)).Hash) {
            throw "Installed artifact verification failed: $file"
        }
    }
    # Match src/AppInstaller/ScheduledTask.cpp: current interactive user, highest
    # privileges, no triggers, no stored password, read/run-only access for that user.
    $sid = [Security.Principal.WindowsIdentity]::GetCurrent().User.Value
    $scheduler = New-Object -ComObject Schedule.Service
    $scheduler.Connect()
    $folder = $scheduler.GetFolder('\')
    $definition = $scheduler.NewTask(0)
    $definition.Principal.UserId = $sid
    $definition.Principal.LogonType = 3
    $definition.Principal.RunLevel = 1
    $definition.Settings.Enabled = $true
    $definition.Settings.AllowDemandStart = $true
    $definition.Settings.DisallowStartIfOnBatteries = $false
    $definition.Settings.StopIfGoingOnBatteries = $false
    $definition.Settings.MultipleInstances = 2
    $action = $definition.Actions.Create(0)
    $action.Path = $executable
    $action.Arguments = '/task'
    $action.WorkingDirectory = $destination
    $security = 'D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGX;;;' + $sid + ')'
    $folder.RegisterTaskDefinition('AnyFSE', $definition, 22, $sid, $null, 3, $security) | Out-Null
    "SUCCESS: Updated binaries and registered AnyFSE on-demand elevated task. Backup: $backup" |
        Set-Content -LiteralPath $resultFile -Encoding UTF8
    Write-Output "Repair completed. Open AnyFSE again to restart its monitor. Backup: $backup"
}
catch {
    foreach ($file in $copied) {
        $saved = Join-Path $backup $file
        if (Test-Path -LiteralPath $saved) {
            Copy-Item -LiteralPath $saved -Destination (Join-Path $destination $file) -Force -ErrorAction Continue
        }
    }
    "FAILED: $($_.Exception.Message)" | Set-Content -LiteralPath $resultFile -Encoding UTF8
    throw
}
