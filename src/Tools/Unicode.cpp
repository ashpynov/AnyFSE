// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include "Logging/LogManager.hpp"
#include "Unicode.hpp"
#include <algorithm>

namespace AnyFSE::Tools::Unicode
{
    std::string to_string(const std::wstring& wstr)
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";

        std::string result;
        result.resize(len - 1); // Exact size needed
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
        return result;
    }

    std::wstring to_wstring(const std::string& str)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";

        std::wstring result;
        result.resize(len - 1); // Exact size needed
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
        return result;
    }
    std::wstring to_lower(const std::wstring &str)
    {
        std::wstring wstr = str;
        std::transform(wstr.begin(), wstr.end(), wstr.begin(), ::towlower);
        return wstr;
    }
}
