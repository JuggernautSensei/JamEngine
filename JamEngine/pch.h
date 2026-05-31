#pragma once

// intercept memory allocators
#include "Allocator.h"

// predefine m_focusingShaderItemMacrosEdit
#ifndef NOMINMAX
#    define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif

// imgui
#define IMGUI_DEFINE_MATH_OPERATORS

// windows
#include <windows.h>

// std
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

// directX
#include <d3d11_4.h>
#include <wrl.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// entt
#include <entt/entt.hpp>

// fmt
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/xchar.h>

// non-dependent core
#include "Configs.h"
#include "STL.h"
#include "Macros.h"
#include "DataType.h"
#include "Assertion.h"
#include "HashMap.h"

// dependent core
#include "BaseUtilities.h"
#include "Optional.h"
#include "Math.h"
#include "Vector.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "String.h"
#include "RefCounted.h"
#include "LogSystem.h"
#include "MemoryView.h"
#include "Error.h"
#include "Scoped.h"
#include "Delegate.h"

// utilities
#include "EnumUtilities.h"
#include "Flags.h"
