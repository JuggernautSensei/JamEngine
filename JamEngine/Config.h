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

constexpr std::string_view  k_jamSceneExtension  = ".jscene";
constexpr std::wstring_view k_jamSceneExtensionW = L".jscene";

// file system
constexpr std::wstring_view k_jamContentsDirectory = L"contents";
constexpr std::wstring_view k_jamScenesDirectory   = L"scenes";
constexpr std::wstring_view k_jamAssetsDirectory   = L"assets";
constexpr std::wstring_view k_jamModelDirectory    = L"models";
constexpr std::wstring_view k_jamTextureDirectory  = L"textures";

}   // namespace jam