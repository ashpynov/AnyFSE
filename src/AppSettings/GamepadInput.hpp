// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once
#include <windows.h>
#include <xinput.h>
#include <thread>
#include <atomic>
#include <memory>

#pragma comment(lib, "xinput.lib")

namespace AnyFSE::App::AppSettings::Input
{
    class GamepadInputListener
    {
    public:
        GamepadInputListener(HWND hDialog);
        ~GamepadInputListener();

        // Start listening for gamepad input
        bool Start();

        // Stop listening for gamepad input
        void Stop();

        // Check if listener is active
        bool IsActive() const { return m_isActive; }

    private:
        HWND m_hDialog;
        std::atomic<bool> m_isActive{false};
        std::unique_ptr<std::thread> m_listenerThread;

        // XInput state tracking - support up to 4 controllers
        static constexpr int MAX_CONTROLLERS = 4;
        WORD m_prevButtonState[MAX_CONTROLLERS]{};

        // Thread function
        void ListenerThreadFunc();

        // Input processing
        void ProcessGamepadInput(int controllerIndex, const XINPUT_STATE& state);

        // Helper to send key input to the dialog window
        void SendKeyInput(WORD vKey, bool isDown);

        // Button mapping helpers
        void HandleButtonStateChange(int controllerIndex, WORD buttonMask, WORD vKey, const XINPUT_STATE& state);
    };
}

using namespace AnyFSE::App::AppSettings::Input;
