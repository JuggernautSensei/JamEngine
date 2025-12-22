#pragma once

#define JSON_THROW_USER throw std::exception
#include "Vendor/nlohmann/Json.hpp"
#include "Vendor/nlohmann/json_fwd.hpp"

namespace jam
{

namespace json = nlohmann;
using Json     = json::json;

[[nodiscard]] Optional<Json> LoadJsonFile(const fs::path& _path);
bool                         SaveJsonFile(const fs::path& _path, const Json& _json, Int32 _dump = 4);

[[nodiscard]] Optional<Json> LoadBsonFile(const fs::path& _path);
bool                         SaveBsonFile(const fs::path& _path, const Json& _j);

}   // namespace jam
