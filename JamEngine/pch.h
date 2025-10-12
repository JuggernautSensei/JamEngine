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
#include <format>
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
#include <DirectXTK/SimpleMath.h>
#include <DirectXTK/SimpleMath.inl>
#include <wrl.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// 3rd party
#include "FileSystem.h"
#include "HashMap.h"
#include "json.h"
#include <entt/entt.hpp>
#include "ImGuiGlue.h"

// base
#include "Configs.h"
#include "DataType.h"
#include "Error.h"
#include "Logger.h"

// global utilities
#include "BaseUtilities.h"
#include "EnumUtilities.h"
#include "MathUtilities.h"
#include "StringUtilities.h"