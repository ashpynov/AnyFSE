#pragma once

#include <string>
#include <cstddef>

namespace AnyFSE::ToolsEx::Cabinet
{
    bool Extract(const std::wstring &archive, const std::wstring &destination);
    // The buffer must remain valid until this synchronous call returns.
    bool Extract(const void *data, size_t size, const std::wstring &destination);
}
