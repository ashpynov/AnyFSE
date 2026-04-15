#pragma once
#include <vector>
#include <Windows.h>

namespace Handlers
{
    void SendKeyInput(const std::vector<WORD> &inputs);

    void TakeScreenShoot();
    void ToggleRecord();
    void ShowKeyboard();
    void ToggleMicrophone();

    void OpenComandCenter();
    void OpenLibrary();
    void OpenTaskSwitcher();
}