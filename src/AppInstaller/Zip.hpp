#pragma once

#include <string>
#include <cstddef>

namespace AnyFSE::ToolsEx::Zip
{
    bool Extract(const std::wstring &archive, const std::wstring &destination);
    bool Extract(const void *data, size_t size, const std::wstring &destination);
}
