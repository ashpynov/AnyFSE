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
        bool HasScrollHotkeys() const { return m_scrollKeys[0] && m_scrollKeys[1]; }

    private:
        bool Open();
        void Close();
        static bool HasMouse(const Hid::StickMapping& stick);
        bool RegisterScrollHotkeys();
        void ReleaseScrollHotkeys();
        UINT m_scrollKeys[2] {};

        HANDLE m_device = INVALID_HANDLE_VALUE;
        bool m_deviceLost = false;
        std::optional<Hid::Mapping> m_original;
        std::optional<Hid::Mapping> m_applied;
    };
}
