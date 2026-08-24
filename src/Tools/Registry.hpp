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


#include <windows.h>
#include <string>

namespace AnyFSE::Tools::Registry
{
    std::wstring ReadString(const std::wstring &subKey, const std::wstring &valueName, const std::wstring &defaultValue = L"");
    DWORD ReadDWORD(const std::wstring &subKey, const std::wstring &valueName, DWORD defaultValue = 0);
    bool ReadBool(const std::wstring &subKey, const std::wstring &valueName, bool defaultValue = false);
    bool ValueExists(const std::wstring &subKey, const std::wstring &valueName);

    bool WriteString(const std::wstring &subKey, const std::wstring &valueName, const std::wstring &value);
    bool WriteDWORD(const std::wstring &subKey, const std::wstring &valueName, DWORD value);
    bool WriteBool(const std::wstring &subKey, const std::wstring &valueName, bool value);
    bool WriteBinary(const std::wstring &subKey, const std::wstring &valueName, const BYTE* data, DWORD size);

    // Utility methods
    bool DeleteValue(const std::wstring &subKey, const std::wstring &valueName);
    bool DeleteKey(const std::wstring &subKey);

    HKEY GetRootKey(const std::wstring& subKey, std::wstring& actualPath);
}

namespace Registry = AnyFSE::Tools::Registry;
