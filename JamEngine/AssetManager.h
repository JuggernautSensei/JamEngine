#pragma once
#include "ImageTexture.h"
#include "Material.h"
#include "Mesh.h"

namespace jam
{

class AssetManager
{
public:

    static Result<std::shared_ptr<IAsset>>       Load(eAssetType _type, const fs::path& _path, bool _bForceReload = false);
    static Result<std::shared_ptr<Mesh>>         LoadMesh(const fs::path& _path, bool _bForceReload = false);
    static Result<std::shared_ptr<Material>>     LoadMaterial(const fs::path& _path, bool _bForceReload = false);
    static Result<std::shared_ptr<ImageTexture>> LoadImageTexture(const fs::path& _path, bool _bForceReload = false);

    JAM_NODISCARD static std::shared_ptr<IAsset> Get(eAssetType _type, const fs::path& _path);
    JAM_NODISCARD static std::shared_ptr<Mesh> GetMesh(const fs::path& _path);
    JAM_NODISCARD static std::shared_ptr<Material> GetMaterial(const fs::path& _path);
    JAM_NODISCARD static std::shared_ptr<ImageTexture> GetImageTexture(const fs::path& _path);

    JAM_NODISCARD static Result<std::shared_ptr<IAsset>>       GetOrLoad(eAssetType _type, const fs::path& _path);
    JAM_NODISCARD static Result<std::shared_ptr<Mesh>>         GetOrLoadMesh(const fs::path& _path);
    JAM_NODISCARD static Result<std::shared_ptr<Material>>     GetOrLoadMaterial(const fs::path& _path);
    JAM_NODISCARD static Result<std::shared_ptr<ImageTexture>> GetOrLoadImageTexture(const fs::path& _path);

    static void Unload(eAssetType _type, const fs::path& _path);
    static void UnloadMesh(const fs::path& _path);
    static void UnloadMaterial(const fs::path& _path);
    static void UnloadImageTexture(const fs::path& _path);

    static void CleanGarbage(eAssetType _type);
    static void CleanGarbageAll();

    static void Clear(eAssetType _type);
    static void ClearAll();

    JAM_NODISCARD static const HashMap<fs::path, std::shared_ptr<IAsset>>& GetAssetContainer(eAssetType _type);
};

}   // namespace jam::asset_manager