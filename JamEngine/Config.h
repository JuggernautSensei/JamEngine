#pragma once
// ReSharper disable CppClangTidyClangDiagnosticUnusedMacros

namespace jam
{

#define JAM_ENGINE_VERTION_MAJOR 0
#define JAM_ENGINE_VERTION_MINOR 0
#define JAM_ENGINE_VERTION_PATCH 1
#define JAM_ENGINE_VERSION_STRING     \
    JAM_STR(JAM_ENGINE_VERTION_MAJOR) \
    "." JAM_STR(JAM_ENGINE_VERTION_MINOR) "." JAM_STR(JAM_ENGINE_VERTION_PATCH)

constexpr std::string_view k_jamEngineVersion = JAM_ENGINE_VERSION_STRING;
constexpr std::string_view k_jamEngineName    = "JamEngine";

// about asset
constexpr std::string_view  k_jamModelExtension  = ".jmodel";
constexpr std::wstring_view k_jamModelExtensionW = L".jmodel";

}   // namespace jam