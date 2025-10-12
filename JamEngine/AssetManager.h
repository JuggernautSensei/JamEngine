#pragma once
#include "ImageTexture.h"
#include "Material.h"
#include "Mesh.h"

namespace jam::asset_manager
{

Result<std::shared_ptr<IAsset>>       Load(eAssetType _type, const fs::path& _path, bool _bForceReload = false);
Result<std::shared_ptr<Mesh>>         LoadMesh(const fs::path& _path, bool _bForceReload = false);
Result<std::shared_ptr<Material>>     LoadMaterial(const fs::path& _path, bool _bForceReload = false);
Result<std::shared_ptr<ImageTexture>> LoadImageTexture(const fs::path& _path, bool _bForceReload = false);

JAM_NODISCARD std::shared_ptr<IAsset> Get(eAssetType _type, const fs::path& _path);
JAM_NODISCARD std::shared_ptr<Mesh> GetMesh(const fs::path& _path);
JAM_NODISCARD std::shared_ptr<Material> GetMaterial(const fs::path& _path);
JAM_NODISCARD std::shared_ptr<ImageTexture> GetImageTexture(const fs::path& _path);

JAM_NODISCARD Result<std::shared_ptr<IAsset>> GetOrLoad(eAssetType _type, const fs::path& _path);
JAM_NODISCARD Result<std::shared_ptr<Mesh>> GetOrLoadMesh(const fs::path& _path);
JAM_NODISCARD Result<std::shared_ptr<Material>> GetOrLoadMaterial(const fs::path& _path);
JAM_NODISCARD Result<std::shared_ptr<ImageTexture>> GetOrLoadImageTexture(const fs::path& _path);

void Unload(eAssetType _type, const fs::path& _path);
void UnloadMesh(const fs::path& _path);
void UnloadMaterial(const fs::path& _path);
void UnloadImageTexture(const fs::path& _path);

void CleanGarbage(eAssetType _type);
void CleanGarbageAll();

void Clear(eAssetType _type);
void ClearAll();

JAM_NODISCARD const HashMap<fs::path, std::shared_ptr<IAsset>>& GetAssetContainer(eAssetType _type);

}   // namespace jam::asset_manager