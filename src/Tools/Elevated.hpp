#pragma once

#include <functional>
#include <string>

namespace AnyFSE::Tools::Elevated
{
    // Register handlers at startup, before calling CallHandler().
    void Register(const std::wstring &name, std::function<void()> handler);
    bool CallHandler();
    // Synchronous: returns after the scheduled task exits. Concurrent calls wait for the current call to finish.
    bool Call(const std::wstring &name);
    bool ElevatedStartupApps();
}

namespace Elevated = AnyFSE::Tools::Elevated;
