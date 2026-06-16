#include "Native.h"

namespace ACSEFilter::Native
{
    namespace
    {

        FARPROC ResolveKernelFunction(const char *name)
        {
            if (HMODULE kernelBase = GetModuleHandleW(L"KernelBase.dll"))
            {
                if (FARPROC proc = GetProcAddress(kernelBase, name))
                {
                    return proc;
                }
            }

            if (HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll"))
            {
                return GetProcAddress(kernel32, name);
            }

            return nullptr;
        }

    } // namespace

    ReadFileProc ReadFile = nullptr;
    WaitForMultipleObjectsProc WaitForMultipleObjects = nullptr;

    bool ResolveKernelFunctions()
    {
        ReadFile = reinterpret_cast<ReadFileProc>(ResolveKernelFunction("ReadFile"));
        WaitForMultipleObjects = reinterpret_cast<WaitForMultipleObjectsProc>(ResolveKernelFunction("WaitForMultipleObjects"));

        return ReadFile && WaitForMultipleObjects;
    }

} // namespace ACSEFilter::Native
