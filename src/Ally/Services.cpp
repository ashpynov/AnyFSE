#include <filesystem>
#include "Services.hpp"
#include "ServiceControl.hpp"
#include "Tools/Paths.hpp"
#include "Tools/Process.hpp"
#include "App/Constants.hpp"
#include "Logging/LogManager.hpp"

namespace Ally::Services
{
    namespace c = AnyFSE::App::Constants;
    namespace Tools = AnyFSE::Tools;
    static Logger log = LogManager::GetLogger("AllyServices");

    bool EnableInjectorService()
    {
        const std::wstring injectorExe = std::filesystem::path(Tools::Paths::GetInstallPath()).append(c::InjectorExe).wstring();
        const bool result = ServiceControl::CreateInjector(injectorExe);
        if (!result) log.Error(log.APIError(), "Create injector service failed");
        return result;
    }

    bool DisableInjectorService()
    {
        const bool result = ServiceControl::RemoveInjector();
        if (!result) log.Error(log.APIError(), "Remove injector service failed");
        return result;
    }

    bool EnableAsusOptimizationService()
    {
        if (!Process::FindFirstByExe(c::ArmouryCrateServiceProcess) || Process::FindFirstByExe(c::AsusOptimizationProcess)) return true;
        const bool result = ServiceControl::EnableAsusOptimization();
        if (!result) log.Error(log.APIError(), "Enable ASUS Optimization service failed");
        return result;
    }
}
