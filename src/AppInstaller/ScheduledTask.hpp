#pragma once

#include <string>

namespace AnyFSE::ToolsEx::ScheduledTask
{
    void RegisterAnyFSETask(const std::wstring &installPath);
    void DeleteAnyFSETask();
}
