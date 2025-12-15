#pragma once

// predefine macros
#ifndef NOMINMAX
#    define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif

// crt
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

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
#include "Vendor/DirectXTK/SimpleMath.h"
#include "Vendor/DirectXTK/SimpleMath.inl"
#include <d3d11.h>
#include <wrl.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

#include "HashMap.h"
#include "STL.h"
#include <entt/entt.hpp>

#include "Configs.h"
#include "DataType.h"
#include "Error.h"
#include "ErrorBuffer.h"
#include "Log.h"

#include "Macros.h"

#include "MemView.h"

#include "BaseUtilities.h"
#include "MathUtilities.h"
#include "RefCounted.h"

#include "FileSystemUtilities.h"

#include "EnumUtilities.h"
#include "StringUtilities.h"

#include "Json.h"

#include "ImGuiGlue.h"
