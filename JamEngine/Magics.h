#pragma once

namespace jam
{

constexpr StringView k_engineName    = JAM_ENGINE_NAME;
constexpr StringView k_engineVersion = JAM_VERSION_NAME;

constexpr StringView k_contentsDirName   = "Contents";
constexpr StringView k_cachesDirName     = "Caches";
constexpr StringView k_logsDirName       = "Logs";
constexpr StringView k_crashDumpsDirName = "CrashDumps";

constexpr StringView k_jamSceneExt        = ".jsc";
constexpr StringView k_jamMeshExt         = ".jmsh";
constexpr StringView k_jamMaterialExt     = ".jmat";
constexpr StringView k_jamModelExt        = ".jmdl";
constexpr StringView k_jamImageTextureExt = ".jtex";

constexpr StringView k_jamSceneExtFilter        = "*.jsc";
constexpr StringView k_jamMeshExtFilter         = "*.jmsh";
constexpr StringView k_jamMaterialExtFilter     = "*.jmat";
constexpr StringView k_jamModelExtFilter        = "*.jmdl";
constexpr StringView k_jamImageTextureExtFilter = "*.jtex";

}   // namespace jam