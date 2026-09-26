#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace AnyFSE::App::Optimization
{
    struct Entry
    {
        std::string id;
        std::vector<std::uint8_t> before;
        bool applied = false;
    };
    struct State
    {
        bool restoring = false;
        std::vector<Entry> entries;
    };

    // Backend saves must be durable. Every mutation has a write-ahead snapshot.
    // Reapplying or restoring the same action must be idempotent.
    template<class Backend>
    bool Reconcile(bool gaming, const std::vector<std::string>& actions, State& state, Backend& backend)
    {
        bool complete = true;
        if (!gaming || state.restoring)
        {
            if (state.entries.empty() && !state.restoring) return true;
            state.restoring = true;
            if (!backend.Save(state)) return false;
            for (size_t index = state.entries.size(); index > 0; --index)
            {
                const Entry entry = state.entries[index - 1];
                if (!backend.Restore(entry)) { complete = false; continue; }
                state.entries.erase(state.entries.begin() + index - 1);
                if (!backend.Save(state)) return false;
            }
            if (state.entries.empty())
            {
                state.restoring = false;
                if (!backend.Save(state)) return false;
            }
            // A new session may only start after all old snapshots have been restored.
            return complete && !gaming;
        }
        for (const auto& id : actions)
        {
            if (!backend.ShouldApply()) return false;
            auto it = std::find_if(state.entries.begin(), state.entries.end(), [&](const Entry& entry) { return entry.id == id; });
            if (it == state.entries.end())
            {
                Entry entry{id};
                if (!backend.Capture(entry)) { complete = false; continue; }
                state.entries.push_back(entry);
                if (!backend.Save(state)) return false;
                it = state.entries.end() - 1;
            }
            if (it->applied) continue;
            if (!backend.Apply(*it)) { complete = false; continue; }
            it->applied = true;
            if (!backend.Save(state)) return false;
        }
        return complete;
    }
}
