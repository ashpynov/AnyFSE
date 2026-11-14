#pragma once

#include <optional>
#include <windows.h>
#include "Ally/Hid.hpp"

namespace Ally
{
    // Owned by the HID listener; Start/Stop must be called on the same thread.
    class Controller
    {
    public:
        Controller() = default;
        ~Controller();
        Controller(const Controller&) = delete;
        Controller& operator=(const Controller&) = delete;

        bool Start(int mode);
        void Stop();

    private:
        bool Open();
        void Close();
        static bool HasMouse(const Hid::StickMapping& stick);

        HANDLE m_device = INVALID_HANDLE_VALUE;
        bool m_deviceLost = false;
        std::optional<Hid::Mapping> m_original;
        std::optional<Hid::Mapping> m_applied;
    };
}
