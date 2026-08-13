#pragma once

#include <windows.h>

#include <cstddef>

namespace ACSEFilter
{

    struct ImportHookSpec
    {
        const char *functionName;
        void *hookFunction;
    };

    void PatchModuleImports(HMODULE module, const ImportHookSpec *hooks, size_t hookCount);

} // namespace ACSEFilter
