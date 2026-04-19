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

#include "GamepadInput.hpp"
#include "Logging/LogManager.hpp"

namespace AnyFSE::App::AppSettings::Input
{
    static Logger log = LogManager::GetLogger("GamepadInput");

    GamepadInputListener::GamepadInputListener(HWND hDialog)
        : m_hDialog(hDialog)
    {
    }

    GamepadInputListener::~GamepadInputListener()
    {
        Stop();
    }

    bool GamepadInputListener::Start()
    {
        if (m_isActive)
        {
            return true;
        }

        m_isActive = true;
        m_listenerThread = std::make_unique<std::thread>(&GamepadInputListener::ListenerThreadFunc, this);

        if (!m_listenerThread)
        {
            log.Error("Failed to create listener thread");
            m_isActive = false;
            return false;
        }

        log.Info("Gamepad input listener started (XInput)");
        return true;
    }

    void GamepadInputListener::Stop()
    {
        if (!m_isActive)
        {
            return;
        }

        m_isActive = false;

        if (m_listenerThread && m_listenerThread->joinable())
        {
            m_listenerThread->join();
            m_listenerThread.reset();
        }

        log.Info("Gamepad input listener stopped");
    }

    void GamepadInputListener::ListenerThreadFunc()
    {
        XINPUT_STATE state{};
        DWORD dwResult;

        while (m_isActive)
        {
            // Poll all 4 possible controllers
            for (int controllerIndex = 0; controllerIndex < MAX_CONTROLLERS; ++controllerIndex)
            {
                dwResult = XInputGetState(controllerIndex, &state);

                if (dwResult == ERROR_SUCCESS)
                {
                    ProcessGamepadInput(controllerIndex, state);
                }
                else if (dwResult == ERROR_DEVICE_NOT_CONNECTED)
                {
                    // Controller disconnected - reset state for this controller
                    m_prevButtonState[controllerIndex] = 0;
                }
            }

            // Poll at ~60 Hz
            Sleep(16);
        }
    }

    void GamepadInputListener::ProcessGamepadInput(int controllerIndex, const XINPUT_STATE& state)
    {
        WORD currentButtonState = state.Gamepad.wButtons;

        // Handle D-pad buttons
        HandleButtonStateChange(controllerIndex, XINPUT_GAMEPAD_DPAD_UP, VK_UP, state);
        HandleButtonStateChange(controllerIndex, XINPUT_GAMEPAD_DPAD_DOWN, VK_DOWN, state);
        HandleButtonStateChange(controllerIndex, XINPUT_GAMEPAD_DPAD_LEFT, VK_LEFT, state);
        HandleButtonStateChange(controllerIndex, XINPUT_GAMEPAD_DPAD_RIGHT, VK_RIGHT, state);

        // Handle action buttons
        // A button -> Space
        HandleButtonStateChange(controllerIndex, XINPUT_GAMEPAD_A, VK_SPACE, state);

        // B button -> Escape
        HandleButtonStateChange(controllerIndex, XINPUT_GAMEPAD_B, VK_ESCAPE, state);

        m_prevButtonState[controllerIndex] = currentButtonState;
    }

    void GamepadInputListener::HandleButtonStateChange(int controllerIndex, WORD buttonMask, WORD vKey, const XINPUT_STATE& state)
    {
        WORD currentButtonState = state.Gamepad.wButtons;
        bool isCurrentlyPressed = (currentButtonState & buttonMask) != 0;
        bool wasPreviouslyPressed = (m_prevButtonState[controllerIndex] & buttonMask) != 0;

        // Button pressed (transition from not pressed to pressed)
        if (isCurrentlyPressed && !wasPreviouslyPressed)
        {
            SendKeyInput(vKey, true);
        }
        // Button released (transition from pressed to not pressed)
        else if (!isCurrentlyPressed && wasPreviouslyPressed)
        {
            SendKeyInput(vKey, false);
        }
    }

    void GamepadInputListener::SendKeyInput(WORD vKey, bool isDown)
    {
        // Create a keyboard input event and send it
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vKey;
        input.ki.dwFlags = isDown ? 0 : KEYEVENTF_KEYUP;
        input.ki.time = 0;

        SendInput(1, &input, sizeof(INPUT));
    }
}
