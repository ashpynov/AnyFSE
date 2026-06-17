#include <filesystem>
#include "Tools/Unicode.hpp"
#include "Tools/Packages.hpp"
#include "Tools/Process.hpp"
#include "App/AppConstants.hpp"
#include "Logging/LogManager.hpp"

namespace Ally::Services
{
    namespace fs = std::filesystem;
    namespace c = AnyFSE::AppConstants;
    namespace Tools = AnyFSE::Tools;

    static Logger log = LogManager::GetLogger("AllyServices");

    bool EnableInjectorService()
    {
        log.Info("Found package at %s", std::filesystem::path(Tools::Packages::GetAppxInstallLocation(c::PackageFamilyName)).string().c_str());

        const std::wstring injectorExe =
            std::filesystem::path(Tools::Packages::GetAppxInstallLocation(c::PackageFamilyName))
            .append(c::InjectorExe)
            .wstring();

         std::wstring commandLine = std::wstring(L"/c")
            + L" sc create " + c::InjectorServiceName
            + L" binPath=\"\\\"" + injectorExe + L"\\\" --service\""
            + L" start=auto "
            + L" DisplayName=\"" + c::InjectorServiceDisplayName + L"\""
            + L" & sc description " + c::InjectorServiceName + L" \"" + c::InjectorServiceDescription + L"\""
            + L" & sc failure " + c::InjectorServiceName + L" reset=0 actions=restart/5000/restart/5000/restart/5000"
            + L" & sc start " + c::InjectorServiceName;

        HINSTANCE result = ShellExecuteW(
            nullptr,
            L"runas",
            L"cmd.exe",
            commandLine.c_str(),
            nullptr,
            SW_HIDE);

        log.Info("Starting injector as %s %s returned %d", "cmd", Unicode::to_string(commandLine).c_str(), "--install", result);

        return reinterpret_cast<INT_PTR>(result) > 32;
    }

    bool DisableInjectorService()
    {
        const std::wstring commandLine = std::wstring(L"/c sc stop ") + c::InjectorServiceName
                                                    + L" & sc delete " + c::InjectorServiceName;

        HINSTANCE result = ShellExecuteW(
            nullptr,
            L"runas",
            L"cmd.exe",
            commandLine.c_str(),
            nullptr,
            SW_HIDE);

        return reinterpret_cast<INT_PTR>(result) > 32;
    }

    bool EnableAsusOptimizationService()
    {
        if (!Process::FindFirstByExe(c::ArmouryCrateServiceProcess) || Process::FindFirstByExe(c::AsusOptimizationProcess))
        {
            return true;
        }

        const std::wstring commandLine = std::wstring(L"/c sc config ") + c::AsusOptimizationService +
            L" start=auto & sc start " + c::AsusOptimizationService;

        HINSTANCE result = ShellExecuteW(
            nullptr,
            L"open",
            L"cmd.exe",
            commandLine.c_str(),
            nullptr,
            SW_HIDE);

        return reinterpret_cast<INT_PTR>(result) > 32;
    }
}