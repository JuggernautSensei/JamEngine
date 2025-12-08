#pragma once

namespace jam
{

constexpr std::string_view k_engineName    = JAM_ENGINE_NAME;
constexpr std::string_view k_engineVersion = JAM_VERSION_NAME;

constexpr std::string_view k_contentsDirName = "Contents";

constexpr std::string_view k_jamSceneExt    = ".jsc";
constexpr std::string_view k_jamMeshExt     = ".jmsh";
constexpr std::string_view k_jamMaterialExt = ".jmat";
constexpr std::string_view k_jamModelExt    = ".jmdl";

constexpr std::string_view k_jamSceneExtFilter    = "*.jsc";
constexpr std::string_view k_jamMeshExtFilter     = "*.jmsh";
constexpr std::string_view k_jamMaterialExtFilter = "*.jmat";
constexpr std::string_view k_jamModelExtFilter    = "*.jmdl";

}   // namespace jam