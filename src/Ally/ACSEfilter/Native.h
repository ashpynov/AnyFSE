#pragma once

#include <windows.h>

namespace ACSEFilter::Native
{

using ReadFileProc = BOOL(WINAPI*)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
using GetOverlappedResultProc = BOOL(WINAPI*)(HANDLE, LPOVERLAPPED, LPDWORD, BOOL);
using GetOverlappedResultExProc = BOOL(WINAPI*)(HANDLE, LPOVERLAPPED, LPDWORD, DWORD, BOOL);
using WaitForMultipleObjectsProc = DWORD(WINAPI*)(DWORD, const HANDLE*, BOOL, DWORD);

extern ReadFileProc ReadFile;
extern GetOverlappedResultProc GetOverlappedResult;
extern GetOverlappedResultExProc GetOverlappedResultEx;
extern WaitForMultipleObjectsProc WaitForMultipleObjects;

bool ResolveKernelFunctions();

} // namespace ACSEFilter::Native
