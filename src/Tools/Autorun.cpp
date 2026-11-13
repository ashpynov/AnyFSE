#include "Autorun.hpp"
#include <filesystem>
#include "App/Constants.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Paths.hpp"
#include "Tools/Registry.hpp"

namespace c = AnyFSE::App::Constants;

namespace AnyFSE::Tools::Autorun
{
    void UpdateHidListenerRegistration()
    {
        // Keep the original preference while Xbox startup is temporarily enabled for a reboot.
        const bool startup = Registry::ReadBool(c::GamingHomeAppRegKey, c::StartupToGamingHomeRestoreRegValue,
            Registry::ReadBool(c::GamingHomeAppRegKey, c::StartupToGamingHomeRegValue));
        if (Config::IsHidListenerEnabled() && !startup)
        {
            const std::wstring command = L"\"" + (std::filesystem::path(Paths::GetInstallPath()) / c::AnyFseExe).wstring()
                + L"\" " + c::HidListenerArgument;
            Registry::WriteString(c::HidListenerAutorunKey, c::HidListenerAutorunValue, command);
        }
        else
            Registry::DeleteValue(c::HidListenerAutorunKey, c::HidListenerAutorunValue);
    }
}
