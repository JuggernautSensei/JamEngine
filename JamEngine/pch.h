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

// 3rd party
#include "HashMap.h"
#include "Optional.h"
#include "STL.h"
#include <entt/entt.hpp>

// forward declarations

// non-dependent core
#include "Configs.h"
#include "Macros.h"
#include "DataType.h"
#include "Fwd.h"

// dependent core
#include "Error.h"
#include "TypeTrait.h"
#include "Math.h"
#include "BaseUtilities.h"
#include "StringView.h"
#include "RefCounted.h"
#include "MemView.h"
#include "Log.h"

// utilities
#include "GlobalBuffer.h"
#include "StringUtilities.h"
#include "FileSystemUtilities.h"
#include "EnumUtilities.h"
#include "FunctionRef.h"

// 3rd party glue
#include "Json.h"
#include "ImGuiGlue.h"
