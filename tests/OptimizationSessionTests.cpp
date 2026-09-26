#include "App/OptimizationSession.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
#include <thread>
#include "Tools/ElevatedReservation.hpp"

namespace o = AnyFSE::App::Optimization;
namespace elevated = AnyFSE::Tools::Elevated;
namespace Constants { constexpr wchar_t TestReservationPrefix[] = L"Local\\AnyFSE.Tests.Reservation."; }

void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

struct FakeBackend
{
    o::State disk;
    std::map<std::string, std::vector<std::uint8_t>> values;
    std::vector<std::string> restoreOrder;
    bool gaming = true;
    bool failCapture = false;
    bool failApply = false;
    bool failRestore = false;
    int failSaveNumber = 0;
    int saves = 0;
    int applications = 0;

    bool ShouldApply() { return gaming; }
    bool Save(const o::State& state)
    {
        if (++saves == failSaveNumber) return false;
        disk = state;
        return true;
    }
    bool Capture(o::Entry& entry)
    {
        if (failCapture) return false;
        const auto it = values.find(entry.id);
        entry.before = {static_cast<std::uint8_t>(it != values.end())};
        if (it != values.end()) entry.before.insert(entry.before.end(), it->second.begin(), it->second.end());
        return true;
    }
    bool Apply(const o::Entry& entry)
    {
        Require(!disk.entries.empty(), "Mutation without persisted snapshot");
        if (failApply) return false;
        values[entry.id] = {99};
        ++applications;
        return true;
    }
    bool Restore(const o::Entry& entry)
    {
        Require(disk.restoring, "Restore must persist its phase first");
        if (failRestore) return false;
        restoreOrder.push_back(entry.id);
        if (entry.before.at(0)) values[entry.id] = {entry.before.begin() + 1, entry.before.end()};
        else values.erase(entry.id);
        return true;
    }
    bool Run(bool active, const std::vector<std::string>& actions = {"a", "b"})
    {
        // Each invocation models a fresh elevated process, recovering the durable journal.
        o::State state = disk;
        return o::Reconcile(active, actions, state, *this);
    }
};

int main()
{
    try
    {
        {
            FakeBackend backend;
            backend.values["a"] = {0, 7, 255, 0};
            const auto original = backend.values;
            Require(backend.Run(true), "Activation failed");
            Require(backend.Run(true) && backend.applications == 2, "Repeated activation must not overwrite backup or reapply");
            Require(backend.Run(false), "Restoration failed");
            Require(backend.values == original, "Restore exact bytes and delete originally absent values");
            Require(backend.restoreOrder == std::vector<std::string>({"b", "a"}), "Restore reverse dependency order");
            Require(backend.disk.entries.empty(), "Journal should be empty after success");
            Require(backend.Run(false), "Repeated desktop synchronization must be harmless");
        }
        {
            FakeBackend backend;
            backend.failSaveNumber = 1;
            Require(!backend.Run(true) && backend.values.empty(), "Backup failure must prevent mutation");
        }
        {
            FakeBackend backend;
            backend.failCapture = true;
            Require(!backend.Run(true) && backend.values.empty(), "Read failure must not be treated as missing value");
        }
        {
            FakeBackend backend;
            backend.failApply = true;
            Require(!backend.Run(true) && !backend.disk.entries.empty(), "Apply failure must retain snapshots");
            backend.failApply = false;
            Require(backend.Run(true), "Pending applications must retry");
            Require(backend.Run(false) && backend.values.empty(), "Failed application recovery");
        }
        {
            FakeBackend backend;
            backend.values["a"] = {5};
            backend.failSaveNumber = 2; // Crash after mutation, before applied marker.
            Require(!backend.Run(true), "Simulate interrupted commit");
            Require(backend.Run(true), "Idempotent reapplication after crash");
            Require(backend.Run(false) && backend.values.at("a") == std::vector<std::uint8_t>({5}), "Original snapshot survives crash");
        }
        {
            FakeBackend backend;
            Require(backend.Run(true), "Activation failed");
            backend.failRestore = true;
            Require(!backend.Run(false) && backend.disk.restoring, "Restore failure must retain recovery phase");
            const int count = backend.applications;
            Require(!backend.Run(true) && backend.applications == count, "New session must not overwrite unrestored state");
            backend.failRestore = false;
            Require(!backend.Run(true) && backend.values.empty(), "Finish old recovery before new session");
            Require(backend.Run(true) && backend.applications > count, "Next retry may start a new session");
        }
        {
            FakeBackend backend;
            Require(backend.Run(true), "Activation failed");
            backend.failSaveNumber = backend.saves + 2; // Crash after first successful restore.
            Require(!backend.Run(false), "Simulate crash while committing restore");
            Require(backend.Run(false) && backend.values.empty(), "Restoration must safely replay after a crash");
        }
        {
            FakeBackend backend;
            backend.gaming = false;
            Require(!backend.Run(true) && backend.values.empty(), "Abort activation if actual Xbox mode has ended");
        }
        {
            const auto name = std::wstring(Constants::TestReservationPrefix) + std::to_wstring(GetCurrentProcessId());
            HANDLE owner = elevated::ReserveCall(name.c_str(), 0);
            Require(owner != nullptr, "First administrative caller must acquire the slot");
            HANDLE blocked = elevated::ReserveCall(name.c_str(), 50);
            const DWORD error = GetLastError();
            Require(blocked == nullptr && error == ERROR_TIMEOUT, "Contender must not steal an occupied slot");
            std::thread release([owner]() { Sleep(150); CloseHandle(owner); });
            HANDLE queued = elevated::ReserveCall(name.c_str(), 5000);
            release.join();
            Require(queued != nullptr, "Launcher must wait for optimization instead of being discarded");
            CloseHandle(queued);
            HANDLE next = elevated::ReserveCall(name.c_str(), 0);
            Require(next != nullptr, "A completed command must release the slot");
            CloseHandle(next);
        }
        std::cout << "8 recovery scenarios and elevated-call contention tests passed; no Windows settings were modified.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
