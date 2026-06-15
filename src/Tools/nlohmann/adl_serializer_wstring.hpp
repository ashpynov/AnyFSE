#pragma once

#include "json.hpp"
#include "Tools/Unicode.hpp"


namespace nlohmann
{
    template<>
    struct adl_serializer<std::wstring> {
        static void to_json(json& j, const std::wstring& value) {
            j = Unicode::to_string(value);
        }

        static void from_json(const json& j, std::wstring& value) {
            std::string narrow_str = j.get<std::string>();
            value = Unicode::to_wstring(narrow_str);
        }
    };
}