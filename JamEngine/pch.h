#pragma once

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
#include <memory>
#include <source_location>
#include <type_traits>

// directx
#include <d3d11_4.h>
#include <DirectXTex.h>
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

// imgui
#define IMGUI_DEFINE_MATH_OPERATORS
#include "Vendor/imgui/imgui.h"
#include "Vendor/imgui/imgui_internal.h"
#include "Vendor/imgui/imgui_stdlib.h"
#include "Vendor/ImGuizmo/ImGuizmo.h"

// containers
#include <ankerl/unordered_dense.h>

// core base
#include "Configs.h"
#include "Macros.h"
#include "DataType.h"
#include "Assertion.h"

// core math base
#include "Math.h"

// core utilities
#include "Utilities.h"
#include "Scoped.h"
#include "RefCounted.h"
#include "Containers.h"
#include "String.h"
#include "LogSystem.h"
#include "Delegate.h"
#include "Optional.h"
#include "Error.h"
#include "MemoryView.h"
#include "NameOf.h"
#include "EnumTraits.h"
#include "Flags.h"

// core math
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"

// core graphics
#include "Graphics.h"