#include "pch.h"

#include "AssetManager.h"

#include "Application.h"
using namespace jam;

struct AssetManagerContext
{
    fs::path CreateRelativeOfWorkDir(const fs::path& _path) const
    {
        const Application&        app     = Application::GetInstance();
        const ProjectDirectories& dirs    = app.GetProjectDirectories();
        const fs::path&           workDir = dirs.workingDir;

        std::error_code ec;
        fs::relative(_path, workDir, ec);
        if (ec)
        {
            return _path;
        }
        return {};
    }

    Result<std::shared_ptr<IAsset>> Load(const eAssetType _type, const fs::path& _path, const bool _bForceReload = false)
    {
        fs::path path = CreateRelativeOfWorkDir(_path);
        auto&    map  = assetMap[EnumToInt(_type)];

        if (map.contains(path))   // 기존에 로드된 에셋이 있을 경우
        {
            if (_bForceReload)   // 핫 리로드를 위해 기존 객체를 재사용
            {
                std::shared_ptr<IAsset>& asset = map[path];
                asset->Unload();
                eErrorCode ec = asset->Load(path);
                if (IsError(ec))
                {
                    map.erase(path);
                    return ec;
                }
                return asset;
            }

            // 기존 에셋 반환
            return map[path];
        }

        // 새로운 에셋 생성
        std::shared_ptr<IAsset> asset = nullptr;
        switch (_type)
        {
            case eAssetType ::ImageTexture:
                asset = std::make_shared<ImageTexture>();
                break;
            case eAssetType::Material:
                asset = std::make_shared<Material>();
                break;
            case eAssetType::Mesh:
                asset = std::make_shared<Mesh>();
                break;
            default:
                JAM_ASSERT(0, "Unsupported asset type: {}", EnumToInt(_type));
                return eErrorCode::InvalidArgument;
        }
        JAM_ASSERT(asset, "Failed to create asset instance");
        eErrorCode ec = asset->Load(path);
        if (IsError(ec))
        {
            return ec;
        }
        map[path] = asset;
        return asset;
    }

    std::shared_ptr<IAsset> Get(const eAssetType _type, const fs::path& _path) const
    {
        fs::path    path = CreateRelativeOfWorkDir(_path);
        const auto& map  = assetMap[EnumToInt(_type)];
        if (map.contains(path))
        {
            return map.at(path);
        }
        return nullptr;
    }

    void Unload(const eAssetType _type, const fs::path& _path)
    {
        fs::path path = CreateRelativeOfWorkDir(_path);
        auto&    map  = assetMap[EnumToInt(_type)];
        JAM_ASSERT(map.contains(path), "Asset not found: {}", path.string());
        map.erase(path);
    }

    Result<std::shared_ptr<IAsset>> GetOrLoad(const eAssetType _type, const fs::path& _path)
    {
        if (auto asset = Get(_type, _path); asset != nullptr)
        {
            return asset;
        }
        return Load(_type, _path);
    }

    void Clear(const eAssetType _type)
    {
        auto& map = assetMap[EnumToInt(_type)];
        map.clear();
    }

    void CleanGarbage(const eAssetType _type)
    {
        auto& map = assetMap[EnumToInt(_type)];
        for (auto it = map.begin(); it != map.end();)
        {
            if (it->second.use_count() == 1)
            {
                it = map.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    HashMap<fs::path, std::shared_ptr<IAsset>> assetMap[EnumCount<eAssetType>()] = {};
};
static AssetManagerContext g_assetMgrCtx;

namespace jam::asset_manager
{

Result<std::shared_ptr<IAsset>> Load(const eAssetType _type, const fs::path& _path, const bool _bForceReload)
{
    return g_assetMgrCtx.Load(_type, _path, _bForceReload);
}

Result<std::shared_ptr<Mesh>> LoadMesh(const fs::path& _path, const bool _bForceReload)
{
    auto [ec, asset] = g_assetMgrCtx.Load(eAssetType::Mesh, _path, _bForceReload);
    if (IsOk(ec))
    {
        return std::static_pointer_cast<Mesh>(asset);
    }
    return ec;
}

Result<std::shared_ptr<Material>> LoadMaterial(const fs::path& _path, const bool _bForceReload)
{
    auto [ec, asset] = g_assetMgrCtx.Load(eAssetType::Material, _path, _bForceReload);
    if (IsOk(ec))
    {
        return std::static_pointer_cast<Material>(asset);
    }
    return ec;
}

Result<std::shared_ptr<ImageTexture>> LoadImageTexture(const fs::path& _path, const bool _bForceReload)
{
    auto [ec, asset] = g_assetMgrCtx.Load(eAssetType::ImageTexture, _path, _bForceReload);
    if (IsOk(ec))
    {
        return std::static_pointer_cast<ImageTexture>(asset);
    }
    return ec;
}

std::shared_ptr<IAsset> Get(const eAssetType _type, const fs::path& _path)
{
    return g_assetMgrCtx.Get(_type, _path);
}

std::shared_ptr<Mesh> GetMesh(const fs::path& _path)
{
    std::shared_ptr<IAsset> asset = g_assetMgrCtx.Get(eAssetType::Mesh, _path);
    return std::static_pointer_cast<Mesh>(asset);
}

std::shared_ptr<Material> GetMaterial(const fs::path& _path)
{
    std::shared_ptr<IAsset> asset = g_assetMgrCtx.Get(eAssetType::Material, _path);
    return std::static_pointer_cast<Material>(asset);
}

std::shared_ptr<ImageTexture> GetImageTexture(const fs::path& _path)
{
    std::shared_ptr<IAsset> asset = g_assetMgrCtx.Get(eAssetType::ImageTexture, _path);
    return std::static_pointer_cast<ImageTexture>(asset);
}

Result<std::shared_ptr<IAsset>> GetOrLoad(const eAssetType _type, const fs::path& _path)
{
    return g_assetMgrCtx.GetOrLoad(_type, _path);
}

Result<std::shared_ptr<Mesh>> GetOrLoadMesh(const fs::path& _path)
{
    auto [ec, asset] = g_assetMgrCtx.GetOrLoad(eAssetType::Mesh, _path);
    if (IsOk(ec))
    {
        return std::static_pointer_cast<Mesh>(asset);
    }
    return ec;
}

Result<std::shared_ptr<Material>> GetOrLoadMaterial(const fs::path& _path)
{
    auto [ec, asset] = g_assetMgrCtx.GetOrLoad(eAssetType::Material, _path);
    if (IsOk(ec))
    {
        return std::static_pointer_cast<Material>(asset);
    }
    return ec;
}

Result<std::shared_ptr<ImageTexture>> GetOrLoadImageTexture(const fs::path& _path)
{
    auto [ec, asset] = g_assetMgrCtx.GetOrLoad(eAssetType::ImageTexture, _path);
    if (IsOk(ec))
    {
        return std::static_pointer_cast<ImageTexture>(asset);
    }
    return ec;
}

void Unload(const eAssetType _type, const fs::path& _path)
{
    g_assetMgrCtx.Unload(_type, _path);
}

void UnloadMesh(const fs::path& _path)
{
    g_assetMgrCtx.Unload(eAssetType::Mesh, _path);
}

void UnloadMaterial(const fs::path& _path)
{
    g_assetMgrCtx.Unload(eAssetType::Material, _path);
}

void UnloadImageTexture(const fs::path& _path)
{
    g_assetMgrCtx.Unload(eAssetType::ImageTexture, _path);
}

void CleanGarbage(const eAssetType _type)
{
    g_assetMgrCtx.CleanGarbage(_type);
}

void CleanGarbageAll()
{
    for (eAssetType type: EnumRange<eAssetType>())
    {
        g_assetMgrCtx.CleanGarbage(type);
    }
}

void Clear(const eAssetType _type)
{
    g_assetMgrCtx.Clear(_type);
}

void ClearAll()
{
    for (eAssetType type: EnumRange<eAssetType>())
    {
        g_assetMgrCtx.Clear(type);
    }
}

const HashMap<fs::path, std::shared_ptr<IAsset>>& GetAssetContainer(const eAssetType _type)
{
    return g_assetMgrCtx.assetMap[EnumToInt(_type)];
}

}   // namespace jam::asset_manager