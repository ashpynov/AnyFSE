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
#include <string>

#include "Logging/LogManager.hpp"
#include "ToolsEx/TaskManager.hpp"
#include "Tools/Unicode.hpp"
#include "AppInstaller.hpp"


namespace AnyFSE
{
    namespace fs = std::filesystem;
    static Logger log = LogManager::GetLogger("PostMortem");


    std::wstring GetModuleVersion(const std::wstring &file)
    {
        wchar_t modulePath[MAX_PATH] = {0};
        wcsncpy_s(modulePath, file.c_str(), MAX_PATH);

        if (!GetModuleFileNameW(NULL, modulePath, MAX_PATH))
            return L"";

        DWORD dummy = 0;
        DWORD len = GetFileVersionInfoSizeW(modulePath, &dummy);
        if (len == 0)
            return L"";

        std::vector<char> data(len);
        if (!GetFileVersionInfoW(modulePath, 0, len, data.data()))
            return L"";

        VS_FIXEDFILEINFO *vinfo = nullptr;
        UINT size = 0;
        if (!VerQueryValueW(data.data(), L"\\", (LPVOID *)&vinfo, &size) || vinfo == nullptr)
            return L"";

        DWORD ms = vinfo->dwFileVersionMS;
        DWORD ls = vinfo->dwFileVersionLS;
        int major = HIWORD(ms);
        int minor = LOWORD(ms);
        int patch = HIWORD(ls);

        wchar_t buf[64];
        swprintf_s(buf, L"%d.%d.%d", major, minor, patch);
        return std::wstring(buf);
    }

    void CheckFiles(const std::wstring &base)
    {
        std::list<std::wstring> files
        {
            L"AnyFSE.exe",
            L"AnyFSE.Service.exe",
            L"AnyFSE.Settings.dll",
            L"unins000.exe",
            L"dumps",
        };

        for (auto &f : files)
        {
            fs::path path(base + L"\\" + f);
            log.Info("Check path '%s' - %s%s",
                path.string().c_str(),
                fs::exists(path) ? "Exists" : "NOT FOUND",
                (fs::exists(path) && fs::is_regular_file(path)) ? Unicode::to_string(L" v" + GetModuleVersion(path.wstring())).c_str() : ""
            );
        }
    };

    void CheckDumps(const std::wstring &base)
    {
        log.Info("Check dump files existance");
        fs::path path(base + L"\\dumps");
        if (fs::is_directory(path) && !fs::is_empty(path))
        {
            log.Error("Dumps folder is not empty:");

            for (const auto & entry : fs::directory_iterator(path))
            {
                log.Info(">>> %s (%ull bytes)", entry.path().string().c_str(), entry.file_size());
            }
        }
        else
        {
            log.Info("No dumps");
        }
    }

    void CheckTask(const std::wstring &path)
    {
        log.Info("Check task existance");
        std::wstring taskPath = ToolsEx::TaskManager::GetInstallPath();
        if (taskPath.empty() || _wcsicmp(taskPath.c_str(), path.c_str()))
        {
            log.Error("Task error: %s", taskPath.empty() ? "task is not found" : "task is not updated");
        }
        else
        {
            log.Info("Task exists");
        }
    }

    void AppInstaller::CollectPostMortemInfo(const std::wstring &path)
    {
        log.Info("Collecting post-mortem info");
        CheckFiles(path);
        CheckDumps(path);
        CheckTask(path);
    }
}