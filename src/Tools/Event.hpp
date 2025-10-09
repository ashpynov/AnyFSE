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