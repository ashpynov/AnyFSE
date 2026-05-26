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

    void PatchAllModuleImports(HMODULE selfModule, const ImportHookSpec *hooks, size_t hookCount);

} // namespace ACSEFilter
