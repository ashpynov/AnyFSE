#include "IATHook.h"

#include <cstring>

namespace ACSEFilter
{
    bool PatchImport(HMODULE module, const char *functionName, void *hookFunction)
    {
        if (!module || !functionName || !hookFunction)
        {
            return false;
        }

        BYTE *base = (BYTE *)module;
        const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        {
            return false;
        }

        const IMAGE_NT_HEADERS *nt = (const IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
        {
            return false;
        }

        const IMAGE_DATA_DIRECTORY &directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!directory.VirtualAddress || !directory.Size)
        {
            return false;
        }

        bool patched = false;
        const IMAGE_IMPORT_DESCRIPTOR *import = (const IMAGE_IMPORT_DESCRIPTOR *)(base + directory.VirtualAddress);
        for (; import->Name; ++import)
        {
            if (!import->OriginalFirstThunk || !import->FirstThunk)
            {
                continue;
            }

            const IMAGE_THUNK_DATA *names = (const IMAGE_THUNK_DATA *)(base + import->OriginalFirstThunk);
            IMAGE_THUNK_DATA *thunk = (IMAGE_THUNK_DATA *)(base + import->FirstThunk);
            for (; thunk->u1.Function; ++thunk, ++names)
            {
                if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))
                {
                    continue;
                }

                const IMAGE_IMPORT_BY_NAME *name = (const IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData);
                if (std::strcmp((const char *)name->Name, functionName) != 0)
                {
                    continue;
                }

                void **slot = (void **)&thunk->u1.Function;
                if (*slot == hookFunction)
                {
                    patched = true;
                    continue;
                }

                DWORD oldProtect = 0;
                if (!VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &oldProtect))
                {
                    continue;
                }

                InterlockedExchangePointer(slot, hookFunction);

                DWORD ignored = 0;
                VirtualProtect(slot, sizeof(void *), oldProtect, &ignored);
                patched = true;
            }
        }

        return patched;
    }

} // namespace ACSEFilter
