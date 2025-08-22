#pragma once

// =============================
//  THIS IS PRECOMPILED HEADER
// =============================

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
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <optional>

// directX
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <directxtk/SimpleMath.inl>
#include <wrl.h>
#pragma comment(lib, "d3d11.lib")

// 3rd party - imgui
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_internal.h"
#include "vendor/imgui/imgui_impl_dx11.h"
#include "vendor/imgui/imgui_impl_win32.h"
#include "vendor/imgui/imgui_stdlib.h"

// 3rd party - nlohmann/m_json
#include <nlohmann/json.hpp>

// 3rd party - entt
#include <entt/entt.hpp>

// my headers
#include "DataType.h"
#include "Error.h"
#include "Macros.h"
#include "Result.h"
#include "SmartPointer.h"
//
#include "EnumUtilities.h"
#include "Log.h"
#include "MathUtilities.h"
#include "TypeTrait.h"