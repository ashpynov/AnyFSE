#include "Ally/Profile.hpp"
#include "App/Constants.hpp"
#include "Logging/LogManager.hpp"
#include "Tools/Registry.hpp"

namespace Ally::Profile
{
    namespace c = AnyFSE::App::Constants;
    static auto log = AnyFSE::Logging::LogManager::GetLogger("Profile");
    static const char* const modeNames[] = {"Auto", "Gamepad", "Desktop", "Pause"};

    void SetDesktop()
    {
        SetMode(Mode::Desktop);
    }

    void SetGamepad()
    {
        SetMode(Mode::Gamepad);
    }

    void SetAuto()
    {
        SetMode(Mode::Auto);
    }

    void SetMode(Mode mode)
    {
        if (mode > Mode::Desktop)
            return;

        DWORD mouseMode = Registry::ReadDWORD(c::GameInputRegKey, c::GameInputMouseModeActiveRegValue, -1);
        if (mouseMode == 1)
        {
            log.Debug("Profile switch ignored: MouseModeActive");
            return;
        }

        log.Debug("Set Profile mode to %s", modeNames[(DWORD)mode]);

        Registry::WriteDWORD(c::AllyGamepadConfigRegKey, c::AllyProfileModeRegValue, (DWORD)Mode::Pause);
        Registry::WriteDWORD(c::AllyGamepadConfigRegKey, c::AllyPreviousProfileModeRegValue, (DWORD)mode);

        if (mouseMode)
        {
            Registry::WriteDWORD(c::GameInputRegKey, c::GameInputMouseModeActiveRegValue, 0, REG_OPTION_VOLATILE);
        }

        Registry::DeleteValue(c::GameInputRegKey, c::GameInputMouseModeActiveRegValue);
    }

    void SetToggle()
    {
        const DWORD current = Registry::ReadDWORD(c::AllyGamepadConfigRegKey, c::AllyProfileModeRegValue, (DWORD)Mode::Auto);
        const Mode next = current == (DWORD)Mode::Gamepad ? Mode::Desktop : Mode::Gamepad;
        SetMode(next);
    }

    void SetNext()
    {
        DWORD current = Registry::ReadDWORD(c::AllyGamepadConfigRegKey, c::AllyProfileModeRegValue, (DWORD)Mode::Auto);
        current = current > (DWORD)Mode::Pause ? (DWORD)Mode::Auto : current;

        const Mode next = current >= (DWORD)Mode::Desktop ? Mode::Auto : (Mode)(current + 1);

        log.Debug("Cycle mode current: %s -> next: %s", modeNames[current], modeNames[(DWORD)next]);

        SetMode(next);
    }
}
