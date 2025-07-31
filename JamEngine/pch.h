#pragma once

// =============================
//  THIS IS PRECOMPILED HEADER
// =============================

// windows
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// std
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

// directX
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/SimpleMath.inl>
#include <wrl.h>
#pragma comment(lib, "d3d11.lib")

// 3rd party - imgui
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

// 3rd party - nlohmann/json
#include <nlohmann/json.hpp>

// 3rd party - entt
#include <entt/entt.hpp>

// my headers
#include "DataType.h"
#include "Error.h"
#include "Macros.h"
#include "MathUtilities.h"
//
#include "EnumUtilities.h"
#include "Log.h"
#include "TypeTrait.h"
