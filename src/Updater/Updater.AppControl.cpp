
#include <windows.h>
#include <winhttp.h>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include "Updater.hpp"

#include "Updater/Updater.hpp"
#include "Updater/Updater.Command.hpp"

#include "Tools/Unicode.hpp"
#include "Tools/Paths.hpp"

#include "Configuration/nlohmann/json.hpp"
#include "Configuration/Config.hpp"

#include "Logging/LogManager.hpp"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "urlmon.lib")

using json = nlohmann::json;

namespace AnyFSE::Updater
{
    Logger log = LogManager::GetLogger("Updater");

    // No global updater instance here; prefer per-instance Updater passed to callers.
    const wchar_t *GITHUB_API_HOST = L"api.github.com";
    const std::wstring REPO_PATH = L"/repos/ashpynov/AnyFSE"; // adjust if repository differs
    const std::wstring RELEASE_PAGE = L"https://github.com/ashpynov/AnyFSE/releases/tag/";

    static bool m_bThreadExecuted = false;
    static std::mutex m_readMutex;
    static UpdateInfo m_lastUpdateInfo{UpdaterState::Idle};
    static std::thread m_thread;

    static std::list<SUBSCRIPTION> m_subscribes;
    struct SemVer
    {
        int major = 0, minor = 0, patch = 0;
        std::string pre;
    };

    static void AddSubscribe(HWND hWnd, UINT uMsg);
    static void ClearSubscriptions();
    static void NotifySubscribers();
    static UpdateInfo CheckUpdate(bool includePreRelease);
    static std::string WinHttpGet(const std::wstring& path);
    static SemVer ParseSemVer(const std::string& tag);
    static bool SemVerGreater(const SemVer& a, const SemVer& b);
    static std::wstring GetModuleVersion();
    static std::string PickAssetDownloadUrl(const json& release);

    bool UpdaterHandleCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
    {
        COMMAND command = (COMMAND)wParam;
        switch (command)
        {
            case COMMAND::SUBSCRIBE:
                {
                    SUBSCRIPTION *ps = (SUBSCRIPTION *)lParam;
                    Updater::Subscribe(ps->hWnd, ps->uMsg);
                }
                return FALSE;

            case COMMAND::CHECK:
                {
                    UPDATECHECK *puc = (UPDATECHECK *)lParam;
                    Updater::CheckUpdateAsync(puc->bIncludePreRelease, puc->hWnd, puc->uMsg);
                }
                return FALSE;

            case COMMAND::UPDATE:
                {
                    UPDATEUPDATE *puu = (UPDATEUPDATE *)lParam;
                    Updater::UpdateAsync(puu->newVersion, puu->bSilent, puu->hWnd, puu->uMsg);
                }
                return FALSE;

            case COMMAND::GETUPDATEINFO:
                {
                    UPDATEINFO * pui = (UPDATEINFO *)lParam;
                    pui->put(Updater::GetLastUpdateInfo());
                }
                return FALSE;
            case COMMAND::NOTIFYCONFIGUPDATED:
                {
                    CONFIGUPDATED *pcu = (CONFIGUPDATED *)lParam;
                    Config::UpdateCheckInterval = pcu->iCheckInterval;
                    Config::UpdatePreRelease = pcu->bPreRelease;
                    Config::UpdateNotifications = pcu->bNotifications;
                }
                return TRUE;
        }
        return FALSE;
    }

    std::string WinHttpGet(const std::wstring &path)
    {
        log.Debug("Getting url: %s", Unicode::to_string(path).c_str());

        HINTERNET hSession = WinHttpOpen(L"AnyFSE-Updater/1.0",
                                         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                         WINHTTP_NO_PROXY_NAME,
                                         WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession)
        {
            log.Error(log.APIError(), "WinHttpOpen failed");
            throw std::runtime_error("WinHttpOpen failed");
        }

        HINTERNET hConnect = WinHttpConnect(hSession, GITHUB_API_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            log.Error(log.APIError(), "WinHttpConnect failed");
            throw std::runtime_error("WinHttpConnect failed");
        }

        std::wstring wpath = path;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wpath.c_str(), NULL,
                                                WINHTTP_NO_REFERER,
                                                WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                WINHTTP_FLAG_SECURE);
        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            log.Error(log.APIError(), "WinHttpOpenRequest failed");
            throw std::runtime_error("WinHttpOpenRequest failed");
        }

        // GitHub requires User-Agent
        LPCWSTR headers = L"User-Agent: AnyFSE-Updater\r\nAccept: application/vnd.github.v3+json\r\n";

        BOOL bSend = WinHttpSendRequest(hRequest, headers, (DWORD)wcslen(headers),
                                        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (!bSend || !WinHttpReceiveResponse(hRequest, NULL))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            log.Error(log.APIError(), "WinHttp request failed");
            throw std::runtime_error("WinHttp request failed");
        }

        std::string result;
        DWORD dwSize = 0;
        do
        {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                break;

            if (dwSize == 0)
                break;

            std::vector<char> buffer(dwSize + 1);
            DWORD dwRead = 0;
            if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwRead))
                break;

            buffer[dwRead] = 0;
            result.append(buffer.data(), dwRead);
        } while (dwSize > 0);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        log.Debug("Recieved %d bytes", result.size());

        return result;
    }

    // Use Tools::Unicode::to_wstring / to_string for conversions

    SemVer ParseSemVer(const std::string &tag)
    {
        // accept tags like v1.2.3 or 1.2.3-alpha
        std::string t = tag;
        if (!t.empty() && t[0] == 'v')
            t.erase(0, 1);

        SemVer v;
        size_t pos = t.find('-');
        if (pos != std::string::npos)
        {
            v.pre = t.substr(pos + 1);
            t = t.substr(0, pos);
        }

        std::replace(t.begin(), t.end(), '.', ' ');
        std::istringstream iss(t);
        iss >> v.major >> v.minor >> v.patch;
        return v;
    }

    bool SemVerGreater(const SemVer &a, const SemVer &b)
    {
        if (a.major != b.major)
            return a.major > b.major;
        if (a.minor != b.minor)
            return a.minor > b.minor;
        if (a.patch != b.patch)
            return a.patch > b.patch;
        // if numeric equal: non-prerelease is greater than prerelease
        if (a.pre.empty() && !b.pre.empty())
            return true;
        if (!a.pre.empty() && b.pre.empty())
            return false;
        return a.pre > b.pre; // lexical compare for prerelease labels
    }

    std::wstring GetModuleVersion()
    {
        std::wstring modulePath = Tools::Paths::GetExeFileName();

        if (modulePath.empty())
            return L"0.0.0";

        DWORD dummy = 0;
        DWORD len = GetFileVersionInfoSizeW(modulePath.c_str(), &dummy);
        if (len == 0)
            return L"0.0.0";

        std::vector<char> data(len);
        if (!GetFileVersionInfoW(modulePath.c_str(), 0, len, data.data()))
            return L"0.0.0";

        VS_FIXEDFILEINFO *vinfo = nullptr;
        UINT size = 0;
        if (!VerQueryValueW(data.data(), L"\\", (LPVOID *)&vinfo, &size) || vinfo == nullptr)
            return L"0.0.0";

        DWORD ms = vinfo->dwFileVersionMS;
        DWORD ls = vinfo->dwFileVersionLS;
        int major = HIWORD(ms);
        int minor = LOWORD(ms);
        int patch = HIWORD(ls);

        wchar_t buf[64];
        swprintf_s(buf, L"%d.%d.%d", major, minor, patch);
        return std::wstring(buf);
    }

    std::string PickAssetDownloadUrl(const json &release)
    {
        if (!release.contains("assets") || !release["assets"].is_array())
            return {};
        std::string downloadUrl;
        for (auto &a : release["assets"])
        {
            std::string name = a.value("name", "");
            std::string url = a.value("browser_download_url", "");
            if (name.empty() || url.empty())
                continue;
            // Prefer exact installer filename AnyFSE.Installer.exe
            if (!_stricmp(name.c_str(), "AnyFSE.Installer.exe"))
            {
                log.Debug("Installer at %s", url.c_str());
                return url;
            }
        }
        return downloadUrl;
    }

    UpdateInfo CheckUpdate(bool includePreRelease)
    {
        // New API: return UpdateInfo with newVersion and downloadPath (browser_download_url)
        UpdateInfo info;

        std::string curVerUtf8 = Tools::Unicode::to_string(GetModuleVersion());

        SemVer current = ParseSemVer(curVerUtf8);

        if (!includePreRelease)
        {
            std::string resp = WinHttpGet(REPO_PATH + L"/releases/latest");
            auto j = json::parse(resp);
            std::string tag = j.value("tag_name", "");
            if (tag.empty())
            {
                log.Debug("releases/latest Tag not found");
                return info; // empty
            }
            SemVer remote = ParseSemVer(tag);
            if (SemVerGreater(remote, current))
            {
                log.Debug("Found latest version %s", tag.c_str());

                info.newVersion = Tools::Unicode::to_wstring(tag);
                std::string url = PickAssetDownloadUrl(j);
                info.downloadPath = Tools::Unicode::to_wstring(url);
                return info;
            }
            return info;
        }

        std::string resp = WinHttpGet(REPO_PATH + L"/releases");
        auto j = json::parse(resp);
        if (!j.is_array())
        {
            log.Error(log.APIError(), "Invalid releases response");
            throw std::runtime_error("Invalid releases response");
        }

        SemVer best = current;
        std::string bestTag;
        std::string bestUrl;

        for (auto &item : j)
        {
            std::string tag = item.value("tag_name", "");
            if (tag.empty())
                continue;
            SemVer sv = ParseSemVer(tag);
            if (SemVerGreater(sv, best))
            {
                best = sv;
                bestTag = tag;
                bestUrl = PickAssetDownloadUrl(item);
            }
        }

        if (!bestTag.empty())
        {
            log.Debug("Found version %s", bestTag.c_str());
            info.newVersion = Tools::Unicode::to_wstring(bestTag);
            info.downloadPath = Tools::Unicode::to_wstring(bestUrl);
        }
        return info;
    }

    void Subscribe(HWND hWnd, UINT uMsg)
    {
        std::lock_guard<std::mutex> lock(m_readMutex);
        AddSubscribe(hWnd, uMsg);
    }

    void AddSubscribe(HWND hWnd, UINT uMsg)
    {
        for ( auto& s : m_subscribes)
        {
            if (s.hWnd == hWnd && s.uMsg == uMsg)
            {
                return;
            }
        }

        m_subscribes.push_back(SUBSCRIPTION{hWnd, uMsg});

    }

    void ClearSubscriptions()
    {
        m_subscribes.clear();
    }

    bool Update(const std::wstring &tag, bool silentUpdate)
    {
        if (tag.empty())
            throw std::invalid_argument("tag is empty");

        // Query release by tag
        std::wstring path = REPO_PATH + L"/releases/tags/" + tag;
        std::string resp = WinHttpGet(path);
        auto j = json::parse(resp);

        // find suitable asset
        std::string downloadUrl;
        std::string assetName;
        if (j.contains("assets") && j["assets"].is_array())
        {
            for (auto &a : j["assets"])
            {
                std::string name = a.value("name", "");
                std::string url = a.value("browser_download_url", "");
                if (name.empty() || url.empty())
                    continue;
                // prefer exact AnyFSE.Installer.exe
                if (!_stricmp(name.c_str(), "AnyFSE.Installer.exe"))
                {
                    downloadUrl = url;
                    assetName = name;
                    break;
                }
            }
        }

        if (downloadUrl.empty())
            throw std::runtime_error("No downloadable asset found for tag");


        // convert assetName to wstring
        std::wstring wname = Tools::Unicode::to_wstring(assetName);
        std::wstring localPath = Tools::Paths::GetDataPath() + L"\\AnyFSE." + tag + L".Update.exe";

        // download
        std::wstring wurl = Tools::Unicode::to_wstring(downloadUrl);
        HRESULT hr = URLDownloadToFileW(NULL, wurl.c_str(), localPath.c_str(), 0, NULL);
        if (FAILED(hr))
        {
            throw std::runtime_error("Download failed");
        }
        Sleep(1000);
        {
            std::lock_guard<std::mutex> lock(m_readMutex);
            m_lastUpdateInfo.uiState = UpdaterState::ReadyForUpdate;
            NotifySubscribers();
        }
        SHELLEXECUTEINFOW sei = {0};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.hwnd = NULL;
        sei.lpVerb = L"runas";
        sei.lpFile = localPath.c_str();
        sei.lpParameters = L"/autoupdate";
        sei.nShow = SW_SHOWNORMAL;

        if (!ShellExecuteExW(&sei))
        {
            std::lock_guard<std::mutex> lock(m_readMutex);
            m_lastUpdateInfo.uiState = UpdaterState::Idle;
            DeleteFile(localPath.c_str());
            return false;
        }

        // do not wait; return true when launched


        if (sei.hProcess)
        {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }

        return true;
    }

    UpdateInfo GetLastUpdateInfo()
    {
        std::lock_guard<std::mutex> lock(m_readMutex);
        return m_lastUpdateInfo;
    }

    void CheckUpdateAsync(bool includePreRelease, HWND hWnd, UINT uMsg)
    {
        log.Debug("CheckUpdateAsync");

        std::lock_guard<std::mutex> lock(m_readMutex);

        AddSubscribe(hWnd, uMsg);

        if (m_bThreadExecuted)
        {
            return;
        }

        m_bThreadExecuted = true;
        m_lastUpdateInfo.uiState = UpdaterState::CheckingUpdate;
        m_lastUpdateInfo.lCommandAge = GetTickCount64();

        NotifySubscribers();

        m_thread = std::thread([includePreRelease]()
        {
            UpdateInfo info;
            try
            {
                info = CheckUpdate(includePreRelease);
                info.uiState = UpdaterState::Done;
            }
            catch (...)
            {
                info.uiState = UpdaterState::NetworkFailed;
            }
            {
                std::lock_guard<std::mutex> lock(m_readMutex);
                info.lCommandAge = m_lastUpdateInfo.lCommandAge;
                info.uiCommand   = m_lastUpdateInfo.uiState;

                m_lastUpdateInfo = info;
                m_bThreadExecuted = false;
                for (auto s: m_subscribes)
                {
                    PostMessage(s.hWnd, s.uMsg, 0, 0);
                }
                m_subscribes.clear();
            }
        });

        m_thread.detach();
    }

    void UpdateAsync(const std::wstring &tag, bool silentUpdate, HWND hWnd, UINT uMsg)
    {
        std::lock_guard<std::mutex> lock(m_readMutex);

        AddSubscribe(hWnd, uMsg);

        if (m_bThreadExecuted)
        {
            return;
        }

        m_bThreadExecuted = true;
        m_lastUpdateInfo.newVersion = tag;
        m_lastUpdateInfo.uiState = UpdaterState::Downloading;
        m_lastUpdateInfo.uiCommand = UpdaterState::Downloading;
        m_lastUpdateInfo.lCommandAge = GetTickCount64();

        NotifySubscribers();

        m_thread = std::thread([tag, silentUpdate]()
        {
            try
            {
                 Update(tag, silentUpdate);
                 m_lastUpdateInfo.uiState = UpdaterState::Idle;
            }
            catch (...)
            {
                std::lock_guard<std::mutex> lock(m_readMutex);
                m_lastUpdateInfo.uiState =UpdaterState::NetworkFailed;
            }
            {
                std::lock_guard<std::mutex> lock(m_readMutex);
                m_bThreadExecuted = false;
                NotifySubscribers();
            }
        });

        m_thread.detach();
    }

    void NotifySubscribers()
    {
        for (auto s : m_subscribes)
        {
            PostMessage(s.hWnd, s.uMsg, 0, 0);
        }
        // Only clear after all PostMessages are sent
        // The list should be re-populated for the next update cycle
        ClearSubscriptions();
    }

    int GetSince(const std::wstring& lastCheck)
    {
        if (lastCheck.empty())
        {
            return INT_MAX;
        }

        const long SECOND = 10000000;
        SYSTEMTIME sCheckTime{0};

        int p[6];

        if (swscanf_s(lastCheck.c_str(), L"%04d-%02d-%02dT%02d:%02d:%02d",
            &p[0], &p[1], &p[2], &p[3], &p[4], &p[5]) != 6)
        {
            return INT_MAX;
        }

        sCheckTime.wYear   = (WORD) p[0];
        sCheckTime.wMonth  = (WORD) p[1];
        sCheckTime.wDay    = (WORD) p[2];
        sCheckTime.wHour   = (WORD) p[3];
        sCheckTime.wMinute = (WORD) p[4];
        sCheckTime.wSecond = (WORD) p[5];

        ULONGLONG lCheckTime;
        SystemTimeToFileTime(&sCheckTime, (FILETIME*)&lCheckTime);

        SYSTEMTIME sNow;
        GetLocalTime(&sNow);

        ULONGLONG lNow;
        SystemTimeToFileTime(&sNow, (FILETIME*)&lNow);

        return (int)((lNow - lCheckTime) / SECOND);
    }

    int ScheduledCheckAsync(const std::wstring& lastCheck, int checkInterval, bool includePreRelease, HWND hWnd, UINT uMsg)
    {
        if (checkInterval <= 0)
        {
            return checkInterval;
        }

        int since = GetSince(lastCheck);

        log.Debug("Check schedule interval: lastChecked=%s (%ds/%.2fh ago)", Unicode::to_string(lastCheck).c_str(), since, (float)since / 3600);

        if (since < checkInterval * 3600)
        {
            return checkInterval * 3600 - since;
        }

        log.Debug("Run check");

        CheckUpdateAsync(includePreRelease, hWnd, uMsg);
        return checkInterval * 3600;
    }
}
