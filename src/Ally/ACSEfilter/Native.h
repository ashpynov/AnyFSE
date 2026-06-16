#pragma once

#include <windows.h>

namespace ACSEFilter::Native
{

using ReadFileProc = BOOL(WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
using WaitForMultipleObjectsProc = DWORD(WINAPI*)(DWORD, const HANDLE*, BOOL, DWORD);

extern ReadFileProc ReadFile;
extern WaitForMultipleObjectsProc WaitForMultipleObjects;

bool ResolveKernelFunctions();

} // namespace ACSEFilter::Native
