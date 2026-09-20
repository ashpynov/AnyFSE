#pragma once

#include <functional>
#include <string>

namespace AnyFSE::Tools::Elevated
{
    // Register handlers at startup, before calling CallHandler().
    void Register(const std::wstring &name, std::function<void()> handler);
    bool CallHandler();
    // Synchronous: returns after the scheduled task exits. Concurrent calls are rejected.
    bool Call(const std::wstring &name);
}

namespace Elevated = AnyFSE::Tools::Elevated;