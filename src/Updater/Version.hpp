#pragma once

#include <algorithm>
#include <sstream>
#include <string>

namespace AnyFSE::Updater
{
    struct SemVer
    {
        int major = 0, minor = 0, patch = 0;
        std::string pre;
    };

    inline SemVer ParseSemVer(const std::string &tag)
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

    inline bool SemVerGreater(const SemVer &a, const SemVer &b)
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

    inline bool SemVerGreater(const std::string &a, const std::string &b)
    {
        return SemVerGreater(ParseSemVer(a), ParseSemVer(b));
    }
}
