@echo off
setlocal
cd /d "%~dp0"
if /i "%~1"=="Release" goto release
if /i "%~1"=="Debug" goto debug
if not "%~1"=="" goto invalid
 echo.
 echo Compilar AnyFSE
 echo 1. Release (sem assinatura)
 echo 2. Debug
 echo 3. Sair
choice /c 123 /n /m "Escolha uma opcao [1-3]: "
if errorlevel 3 exit /b 0
if errorlevel 2 goto debug
:release
set "buildConfiguration=Release"
goto build
:debug
set "buildConfiguration=Debug"
:build
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Build-AnyFSE.ps1" -Configuration %buildConfiguration%
set "buildResult=%errorlevel%"
if "%~1"=="" pause
exit /b %buildResult%
:invalid
echo Uso: Compilar.bat [Release^|Debug]
exit /b 1
