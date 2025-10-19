// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#pragma once

#include <vector>
#include <functional>

namespace AnyFSE::Tools
{
    class Event
    {
        protected:
            std::vector<std::function<void()>> callbacks;

        public:
            void operator +=(std::function<void()> callback);
            void operator -=(const std::function<void()>& callback);
            void Notify();
    };
}
using namespace AnyFSE::Tools;