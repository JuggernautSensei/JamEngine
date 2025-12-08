#pragma once

#define JSON_THROW_USER throw std::exception
#include "Vendor/nlohmann/json_fwd.hpp"
#include "Vendor/nlohmann/json.hpp"
using Json = nlohmann::json;