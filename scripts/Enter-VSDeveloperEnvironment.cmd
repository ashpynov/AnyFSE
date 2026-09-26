@echo off
set "ANYFSE_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%ANYFSE_VSWHERE%" (
    echo Visual Studio Installer not found. Install Visual Studio 2022 C++ Build Tools. >&2
    exit /b 1
)
set "ANYFSE_VSINSTALL="
for /f "usebackq tokens=*" %%i in (`call "%ANYFSE_VSWHERE%" -latest -products * -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "ANYFSE_VSINSTALL=%%i"
if not defined ANYFSE_VSINSTALL (
    echo Visual Studio 2022 with MSVC x64 tools not found. >&2
    exit /b 1
)
call "%ANYFSE_VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1
rem Override the legacy Community-only fallback in the project files.
set "VCTargetsPath=%ANYFSE_VSINSTALL%\MSBuild\Microsoft\VC\v170\"
exit /b 0
