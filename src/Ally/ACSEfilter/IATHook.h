#pragma once

#include <windows.h>

namespace ACSEFilter
{

    bool PatchImport(HMODULE module, const char *functionName, void *hookFunction);

} // namespace ACSEFilter
