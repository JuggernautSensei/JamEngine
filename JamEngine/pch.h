#pragma once

// predefine macros
#ifndef NOMINMAX
#    define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif

// windows
#include <windows.h>

// std
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <format>

// directX
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/SimpleMath.inl>
#include <wrl.h>
#pragma comment(lib, "d3d11.lib")

// 3rd party - nlohmann/m_json
#include <nlohmann/json.hpp>

// 3rd party - entt
#include <entt/entt.hpp>

// 3rd party - filesystem (std::string base)
#define GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
#include "vendor/ghc/filesystem.hpp"

// 3rd party - unordered_dense
#include "unordered_dense.h"

//
#include "Configs.h"
//
#include "DataType.h"
#include "Macros.h"
//
#include "EnumUtilities.h"
#include "Error.h"
//
#include "Log.h"
//
#include "Result.h"
//
#include "TypeTrait.h"
//
#include "MathUtilities.h"
#include "StringUtilities.h"