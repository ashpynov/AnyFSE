#pragma once
#include <vector>
#include <string>
#include <functional>
#include <Windows.h>

namespace Ally::Handlers
{
    struct HandlersDefinition
    {
        std::wstring code;
        std::wstring name;
        std::function<void()> handler;
    };

    extern std::vector<HandlersDefinition> KnownHandlers;
    std::function<void()> GetByName(const std::wstring &handleName);

    void SendKeyInput(const std::vector<WORD> &inputs, int delay = 50);

    void TakeScreenShoot();
    void ToggleRecord();
    void ShowKeyboard();
    void ToggleMicrophone();

    void OpenArmouryCrate();
    void OpenAnyFSESettings();
    void OpenGameBar();
    void OpenSteamOverlay();
    void OpenSteamQuickMenu();
    void OpenGameBarComandCenter();
    void OpenComandCenter();
    void OpenComandCenterF24();
    void OpenLibrary();
    void OpenTaskSwitcher();
    void OpenTaskSwitcherAlt();
}