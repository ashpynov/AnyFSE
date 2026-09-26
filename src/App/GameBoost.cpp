// Reversible adaptation of OpenGameBoost (MIT), copyright (c) 2025 OpenGameBoost.
// See docs/OpenGameBoost-LICENSE.txt for the upstream license.
#include <windows.h>
#include <powrprof.h>
#include <sddl.h>
#include <algorithm>
#include <cstring>
#include <set>
#include "GameBoost.hpp"
#include "GamingExperience.hpp"
#include "OptimizationConstants.hpp"
#include "OptimizationSession.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Unicode.hpp"
#include "Tools/nlohmann/json.hpp"
#include "Logging/LogManager.hpp"

#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "advapi32.lib")

namespace AnyFSE::App::GameBoost
{
    namespace c = AnyFSE::App::Constants::Optimization;
    namespace
    {
        Logger log = LogManager::GetLogger("GameBoost");

        template<class T> std::vector<std::uint8_t> Pack(const T& value)
        {
            const auto bytes = reinterpret_cast<const std::uint8_t*>(&value);
            return {bytes, bytes + sizeof(T)};
        }
        template<class T> bool Unpack(const std::vector<std::uint8_t>& bytes, T& value)
        {
            if (bytes.size() != sizeof(T)) return false;
            std::memcpy(&value, bytes.data(), sizeof(T));
            return true;
        }

        std::vector<std::string> Actions()
        {
            std::vector<std::string> result{c::PowerPlan, c::CpuMinimum, c::UsbSuspend, c::PciPower};
            for (const auto& setting : c::RegistrySettings) result.emplace_back(setting.id);
            result.emplace_back(c::Mouse);
            return result;
        }

        const c::RegistrySetting* FindSetting(const std::string& id)
        {
            for (const auto& setting : c::RegistrySettings) if (id == setting.id) return &setting;
            return nullptr;
        }

        struct PlanSnapshot { GUID original; GUID target; };
        struct PowerSnapshot { GUID plan; DWORD value; DWORD supported; };
        struct MouseSnapshot { int values[3]; };

        bool PowerSetting(const std::string& id, const GUID*& group, const GUID*& setting, DWORD& desired)
        {
            if (id == c::CpuMinimum) { group = &c::CpuGroup; setting = &c::CpuMin; desired = 100; }
            else if (id == c::UsbSuspend) { group = &c::UsbGroup; setting = &c::UsbSetting; desired = 0; }
            else if (id == c::PciPower) { group = &c::PciGroup; setting = &c::PciSetting; desired = 0; }
            else return false;
            return true;
        }

        std::string CurrentUser()
        {
            HANDLE token = nullptr;
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return {};
            DWORD size = 0;
            GetTokenInformation(token, TokenUser, nullptr, 0, &size);
            std::vector<BYTE> data(size);
            const bool success = GetTokenInformation(token, TokenUser, data.data(), size, &size) != FALSE;
            CloseHandle(token);
            if (!success) return {};
            LPWSTR sid = nullptr;
            if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(data.data())->User.Sid, &sid)) return {};
            const auto result = Unicode::to_string(sid);
            LocalFree(sid);
            return result;
        }

        bool ActivePlan(GUID& result)
        {
            GUID* plan = nullptr;
            if (PowerGetActiveScheme(nullptr, &plan) != ERROR_SUCCESS) return false;
            result = *plan;
            LocalFree(plan);
            return true;
        }

        bool SelectPlan(GUID& target)
        {
            bool high = false;
            for (ULONG index = 0;; ++index)
            {
                GUID candidate{};
                DWORD size = sizeof(candidate);
                const DWORD error = PowerEnumerate(nullptr, nullptr, nullptr, ACCESS_SCHEME, index,
                    reinterpret_cast<BYTE*>(&candidate), &size);
                if (error == ERROR_NO_MORE_ITEMS) break;
                if (error != ERROR_SUCCESS) return false;
                if (candidate == c::Ultimate) { target = candidate; return true; }
                if (candidate == c::High) high = true;
            }
            if (high) target = c::High;
            else log.Info("No High/Ultimate plan available; keeping current plan (no plan is created)");
            return true;
        }

        class Backend
        {
            Optimization::State& m_state;
            std::string m_owner;
        public:
            Backend(Optimization::State& state, std::string owner) : m_state(state), m_owner(std::move(owner)) {}

            bool Load()
            {
                Registry::Value stored;
                if (!Registry::ReadValue(c::StateKey, c::Journal, stored)) return false;
                if (!stored.exists) return true;
                if (stored.type != REG_BINARY) return false;
                const auto json = nlohmann::json::parse(stored.data);
                if (json.at("version") != 1 || json.at("owner").get<std::string>() != m_owner)
                {
                    log.Error("GameBoost journal belongs to another user or version; leaving it intact");
                    return false;
                }
                m_state.restoring = json.at("restoring").get<bool>();
                const auto actions = Actions();
                std::set<std::string> seen;
                for (const auto& item : json.at("entries"))
                {
                    Optimization::Entry entry;
                    entry.id = item.at("id").get<std::string>();
                    entry.applied = item.at("applied").get<bool>();
                    entry.before = item.at("before").get<std::vector<std::uint8_t>>();
                    if (std::find(actions.begin(), actions.end(), entry.id) == actions.end() || !seen.insert(entry.id).second)
                        return false;
                    // Validate all records before performing any action.
                    if (!Valid(entry)) return false;
                    m_state.entries.push_back(std::move(entry));
                }
                return true;
            }

            bool Save(const Optimization::State& state)
            {
                nlohmann::json json{{"version", 1}, {"owner", m_owner}, {"restoring", state.restoring}};
                json["entries"] = nlohmann::json::array();
                for (const auto& entry : state.entries)
                    json["entries"].push_back({{"id", entry.id}, {"before", entry.before}, {"applied", entry.applied}});
                if (state.entries.empty() && !state.restoring)
                {
                    Registry::Value absent;
                    return Registry::RestoreValue(c::StateKey, c::Journal, absent) && Registry::Flush(c::StateKey);
                }
                const auto encoded = json.dump();
                // Keep the journal within Registry::ReadValue's recoverable size limit.
                if (encoded.size() > 1024 * 1024) return false;
                return Registry::WriteBinary(c::StateKey, c::Journal, reinterpret_cast<const BYTE*>(encoded.data()),
                    static_cast<DWORD>(encoded.size())) && Registry::Flush(c::StateKey);
            }

            bool ShouldApply() { return GamingExperience::IsFullscreenMode(); }

            bool Valid(const Optimization::Entry& entry)
            {
                if (entry.id == c::PowerPlan) return entry.before.size() == sizeof(PlanSnapshot);
                if (entry.id == c::Mouse) return entry.before.size() == sizeof(MouseSnapshot);
                if (FindSetting(entry.id)) return entry.before.size() >= 1 + sizeof(DWORD) && entry.before[0] <= 1;
                PowerSnapshot snapshot{};
                return Unpack(entry.before, snapshot) && snapshot.supported <= 1;
            }

            bool Capture(Optimization::Entry& entry)
            {
                if (const auto setting = FindSetting(entry.id))
                {
                    Registry::Value value;
                    if (!Registry::ReadValue(setting->key, setting->name, value)) return false;
                    entry.before = {static_cast<std::uint8_t>(value.exists)};
                    const auto type = Pack(value.type);
                    entry.before.insert(entry.before.end(), type.begin(), type.end());
                    entry.before.insert(entry.before.end(), value.data.begin(), value.data.end());
                    return true;
                }
                if (entry.id == c::Mouse)
                {
                    MouseSnapshot mouse{};
                    if (!SystemParametersInfoW(SPI_GETMOUSE, 0, mouse.values, 0)) return false;
                    entry.before = Pack(mouse);
                    return true;
                }
                if (entry.id == c::PowerPlan)
                {
                    PlanSnapshot plan{};
                    if (!ActivePlan(plan.original)) return false;
                    plan.target = plan.original;
                    if (!SelectPlan(plan.target)) return false;
                    entry.before = Pack(plan);
                    return true;
                }
                const GUID* group = nullptr;
                const GUID* setting = nullptr;
                DWORD desired = 0;
                if (!PowerSetting(entry.id, group, setting, desired)) return false;
                const auto planEntry = std::find_if(m_state.entries.begin(), m_state.entries.end(),
                    [](const auto& candidate) { return candidate.id == c::PowerPlan; });
                PlanSnapshot plan{};
                if (planEntry == m_state.entries.end() || !planEntry->applied || !Unpack(planEntry->before, plan)) return false;
                PowerSnapshot snapshot{plan.target, 0, 1};
                const DWORD error = PowerReadACValueIndex(nullptr, &plan.target, group, setting, &snapshot.value);
                if (error == ERROR_FILE_NOT_FOUND || error == ERROR_NOT_SUPPORTED)
                {
                    snapshot.supported = 0;
                    log.Info("Power option %s is unavailable on this device; skipped", entry.id.c_str());
                }
                else if (error != ERROR_SUCCESS) return false;
                entry.before = Pack(snapshot);
                return true;
            }

            bool Apply(const Optimization::Entry& entry) { return Change(entry, false); }
            bool Restore(const Optimization::Entry& entry) { return Change(entry, true); }

            bool Change(const Optimization::Entry& entry, bool restore)
            {
                const bool result = ChangeValue(entry, restore);
                if (result) log.Info("%s %s", restore ? "Restored" : "Applied", entry.id.c_str());
                else log.Warn("Failed to %s %s; snapshot retained", restore ? "restore" : "apply", entry.id.c_str());
                return result;
            }

            bool ChangeValue(const Optimization::Entry& entry, bool restore)
            {
                if (const auto setting = FindSetting(entry.id))
                {
                    if (!restore)
                        return setting->text ? Registry::WriteString(setting->key, setting->name, setting->text)
                            : Registry::WriteDWORD(setting->key, setting->name, setting->number);
                    Registry::Value value;
                    value.exists = entry.before[0] != 0;
                    std::memcpy(&value.type, entry.before.data() + 1, sizeof(DWORD));
                    value.data.assign(entry.before.begin() + 1 + sizeof(DWORD), entry.before.end());
                    return Registry::RestoreValue(setting->key, setting->name, value);
                }
                if (entry.id == c::Mouse)
                {
                    MouseSnapshot mouse{};
                    if (restore && !Unpack(entry.before, mouse)) return false;
                    // Runtime-only: do not rewrite the user's persistent mouse preference.
                    return SystemParametersInfoW(SPI_SETMOUSE, 0, mouse.values, SPIF_SENDCHANGE) != FALSE;
                }
                if (entry.id == c::PowerPlan)
                {
                    PlanSnapshot plan{};
                    if (!Unpack(entry.before, plan)) return false;
                    return PowerSetActiveScheme(nullptr, restore ? &plan.original : &plan.target) == ERROR_SUCCESS;
                }
                const GUID* group = nullptr;
                const GUID* setting = nullptr;
                DWORD desired = 0;
                PowerSnapshot snapshot{};
                if (!PowerSetting(entry.id, group, setting, desired) || !Unpack(entry.before, snapshot)) return false;
                if (!snapshot.supported) return true;
                if (PowerWriteACValueIndex(nullptr, &snapshot.plan, group, setting, restore ? snapshot.value : desired) != ERROR_SUCCESS)
                    return false;
                GUID active{};
                if (!ActivePlan(active)) return false;
                // Refresh only if this plan is active, including after a partially failed restoration.
                return active != snapshot.plan || PowerSetActiveScheme(nullptr, &active) == ERROR_SUCCESS;
            }
        };
    }

    bool HasPendingRestore()
    {
        Registry::Value stored;
        // An unreadable journal also requires attention, not a false "nothing to restore" result.
        return !Registry::ReadValue(c::StateKey, c::Journal, stored) || stored.exists;
    }

    bool Synchronize(bool gaming)
    {
        HANDLE mutex = CreateMutexW(nullptr, FALSE, c::Lock);
        if (!mutex) return false;
        const DWORD acquired = WaitForSingleObject(mutex, 0);
        if (acquired != WAIT_OBJECT_0 && acquired != WAIT_ABANDONED) { CloseHandle(mutex); return false; }
        bool success = false;
        try
        {
            const auto owner = CurrentUser();
            if (!owner.empty())
            {
                Optimization::State state;
                Backend backend(state, owner);
                success = backend.Load() && Optimization::Reconcile(gaming, Actions(), state, backend);
            }
        }
        catch (const std::exception& error)
        {
            log.Error("Cannot process GameBoost journal: %s; no backup will be discarded", error.what());
        }
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return success;
    }
}
