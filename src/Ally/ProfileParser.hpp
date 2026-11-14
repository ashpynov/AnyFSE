#pragma once

#include "Ally/Hid.hpp"
#include "Ally/Profile.hpp"

namespace Ally::ProfileParser
{
    bool Load(Profile::Mode mode, Hid::Mapping& mapping);
}
