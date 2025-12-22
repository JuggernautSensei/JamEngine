#include "pch.h"

#include "AssetManager.h"

#include "Application.h"
#include "Events.h"

namespace jam
{

struct AssetManagerContext
{
    using Container = HashMap<fs::path, Ref<IAsset>>;

    [[nodiscard]] Optional<fs::path> RelativeFromWorkingDirectory(const fs::path& _path) const;
    [[nodiscard]] Ref<IAsset>             MakeAssetRef(eAssetType _type) const;

    void OnFileModifiedEvent(const FileModifiedEvent& _event) const;
    void OnFileRenamedEvent(const FileRenamedEvent& _event);
    void OnFileDeletedEvent(const FileDeletedEvent& _event);

    Optional<Ref<IAsset>> Load(eAssetType _type, const fs::path& _path, bool _bForceReload);
    void                       Unload(eAssetType _type, const fs::path& _path);
    Optional<Ref<IAsset>> GetOrLoad(eAssetType _type, const fs::path& _path);
    Ref<IAsset>                Get(eAssetType _type, const fs::path& _path) const;
    [[nodiscard]] bool         Has(eAssetType _type, const fs::path& _path) const;

    void Clear(eAssetType _type);
    void CleanGarbage(eAssetType _type);

    Container assetMaps[NumEnums<eAssetType>()] = {};

}* g_assetMgrCtx = nullptr;

Optional<fs::path> AssetManagerContext::RelativeFromWorkingDirectory(
    const fs::path& _path) const
{
    try
    {
        const Application& app = Application::GetInstance();
        return fs::relative(_path, app.GetWorkingDirectory());
    }
    catch (const fs::filesystem_error& ec)
    {
        GlobalBuffer::SetLastError("AssetManagerContext - failed to calculate relative path: {}", ec.what());
        return k_nullopt;
    }
}

Ref<IAsset> AssetManagerContext::MakeAssetRef(
    const eAssetType _type) const
{
    switch (_type)
    {
        case eAssetType::ImageTexture:
            return MakeRef<ImageTexture>();
        case eAssetType::Material:
            return MakeRef<Material>();
        case eAssetType::Mesh:
            return MakeRef<Mesh>();
        case eAssetType::Model:
            return MakeRef<Model>();
        default:
            JAM_ASSERT(0, "unsupported asset type: {}", IntFrom(_type));
            return nullptr;
    }
}

void AssetManagerContext::OnFileRenamedEvent(
    const FileRenamedEvent& _event)
{
    // reload existing asset without creating new object
    const Optional<fs::path> optOld = RelativeFromWorkingDirectory(_event.GetOldPath());
    const Optional<fs::path> optNew = RelativeFromWorkingDirectory(_event.GetNewPath());

    if (!optOld || !optNew)
    {
        return;
    }

    const fs::path& oldPath = optOld.value();
    const fs::path& newPath = optNew.value();

    for (const eAssetType type: EnumRanges<eAssetType>())
    {
        if (!Has(type, oldPath))
        {
            continue;
        }

        // if asset already exists at new path, unload it first
        if (Has(type, newPath))
        {
            Unload(type, newPath);
        }

        // reload into the same object instance
        const Ref<IAsset> pAsset = Get(type, oldPath);
        if (pAsset->Load(newPath))
        {
            // update container key: remove old, insert new
            Container& container = assetMaps[IntFrom(type)];
            container.erase(oldPath);
            container[newPath] = pAsset;
        }
    }
}

void AssetManagerContext::OnFileDeletedEvent(
    const FileDeletedEvent& _event)
{
    // unload existing asset
    if (const Optional<fs::path> opt = RelativeFromWorkingDirectory(_event.GetPath()))
    {
        const fs::path& path = opt.value();
        for (const eAssetType type: EnumRanges<eAssetType>())
        {
            if (Has(type, path))
            {
                Unload(type, path);
            }
        }
    }
}

void AssetManagerContext::OnFileModifiedEvent(
    const FileModifiedEvent& _event) const
{
    // reload existing asset without creating new object
    if (const Optional<fs::path> opt = RelativeFromWorkingDirectory(_event.GetPath()))
    {
        const fs::path& path = opt.value();
        for (const eAssetType type: EnumRanges<eAssetType>())
        {
            if (Has(type, path))
            {
                Get(type, path)->Load(path);
            }
        }
    }
}

Optional<Ref<IAsset>> AssetManagerContext::Load(
    const eAssetType _type,
    const fs::path&  _path,
    const bool       _bForceReload)
{
    const Optional<fs::path> opt = RelativeFromWorkingDirectory(_path);
    if (!opt)
    {
        return k_nullopt;
    }

    const fs::path& path      = opt.value();
    Container&      container = assetMaps[IntFrom(_type)];

    // check if already exists
    if (container.contains(path))
    {
        Ref<IAsset>& asset = container[path];
        if (_bForceReload)
        {
            if (asset->Load(path))
                return asset;

            // load failed, remove invalid asset
            container.erase(path);
            return k_nullopt;
        }
        return asset;
    }

    // create new asset
    Ref<IAsset> pAsset = MakeAssetRef(_type);
    if (pAsset->Load(path))
    {
        container[path] = pAsset;
        return pAsset;
    }
    else
    {
        return k_nullopt;
    }
}

Ref<IAsset> AssetManagerContext::Get(
    const eAssetType _type,
    const fs::path&  _path) const
{
    if (const Optional<fs::path> opt = RelativeFromWorkingDirectory(_path))
    {
        const fs::path&  path      = opt.value();
        const Container& container = assetMaps[IntFrom(_type)];
        JAM_ASSERT(container.contains(path), "asset not found: {}", path.native());
        return container.at(path);
    }
    else
    {
        JAM_ASSERT(0, "failed to calculate relative path for asset: {}", _path.native());
        return nullptr;
    }
}

void AssetManagerContext::Unload(
    const eAssetType _type,
    const fs::path&  _path)
{
    if (const Optional<fs::path> opt = RelativeFromWorkingDirectory(_path))
    {
        const fs::path& path      = opt.value();
        Container&      container = assetMaps[IntFrom(_type)];

        if (container.contains(path))
        {
            container.at(path)->Unload();
            container.erase(path);
        }
    }
}

bool AssetManagerContext::Has(
    const eAssetType _type,
    const fs::path&  _path) const
{
    if (const Optional<fs::path> opt = RelativeFromWorkingDirectory(_path))
    {
        const Container& container = assetMaps[IntFrom(_type)];
        return container.contains(opt.value());
    }
    else
    {
        return false;
    }
}

Optional<Ref<IAsset>> AssetManagerContext::GetOrLoad(
    const eAssetType _type,
    const fs::path&  _path)
{
    if (Has(_type, _path))
    {
        return Get(_type, _path);
    }
    else
    {
        return Load(_type, _path, false);
    }
}

void AssetManagerContext::Clear(
    const eAssetType _type)
{
    assetMaps[IntFrom(_type)].clear();
}

void AssetManagerContext::CleanGarbage(
    const eAssetType _type)
{
    Container& container = assetMaps[IntFrom(_type)];
    for (auto it = container.begin(); it != container.end();)
    {
        // if ref count is 1, it means only the manager holds it
        if (it->second->GetRefCount() == 1)
        {
            it = container.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

}   // namespace jam

namespace jam
{

void AssetManager::Init()
{
    JAM_ASSERT(g_assetMgrCtx == nullptr, "AssetManager::Init() called more than once.");
    g_assetMgrCtx = JAM_NEW(AssetManagerContext);
}

void AssetManager::Shutdown()
{
    JAM_ASSERT(g_assetMgrCtx, "AssetManager::Shutdown() called without Init().");
    JAM_DELETE(g_assetMgrCtx);
}

void AssetManager::OnEvent(
    Event& _event)
{
    HandleEvent<FileModifiedEvent>(_event, g_assetMgrCtx, &AssetManagerContext::OnFileModifiedEvent);
    HandleEvent<FileRenamedEvent>(_event, g_assetMgrCtx, &AssetManagerContext::OnFileRenamedEvent);
    HandleEvent<FileDeletedEvent>(_event, g_assetMgrCtx, &AssetManagerContext::OnFileDeletedEvent);
}

Optional<Ref<IAsset>> AssetManager::Load(
    const eAssetType _type,
    const fs::path&  _path,
    const bool       _bForceReload)
{
    return g_assetMgrCtx->Load(_type, _path, _bForceReload);
}

Optional<Ref<Model>> AssetManager::LoadModel(
    const fs::path& _path,
    const bool      _bForceReload)
{
    if (const Optional<Ref<IAsset>> opt = g_assetMgrCtx->Load(eAssetType::Model, _path, _bForceReload))
    {
        return opt->As<Model>();
    }
    else
    {
        return k_nullopt;
    }
}

Optional<Ref<Mesh>> AssetManager::LoadMesh(
    const fs::path& _path,
    const bool      _bForceReload)
{
    if (const Optional<Ref<IAsset>> opt = Load(eAssetType::Mesh, _path, _bForceReload))
    {
        return opt->As<Mesh>();
    }
    else
    {
        return k_nullopt;
    }
}

Optional<Ref<Material>> AssetManager::LoadMaterial(
    const fs::path& _path,
    const bool      _bForceReload)
{
    if (const Optional<Ref<IAsset>> opt = Load(eAssetType::Material, _path, _bForceReload))
    {
        return opt->As<Material>();
    }
    else
    {
        return k_nullopt;
    }
}

Optional<Ref<ImageTexture>> AssetManager::LoadImageTexture(
    const fs::path& _path,
    const bool      _bForceReload)
{
    if (const Optional<Ref<IAsset>> opt = Load(eAssetType::ImageTexture, _path, _bForceReload))
    {
        return opt->As<ImageTexture>();
    }
    else
    {
        return k_nullopt;
    }
}

Ref<IAsset> AssetManager::Get(
    const eAssetType _type,
    const fs::path&  _path)
{
    return g_assetMgrCtx->Get(_type, _path);
}

Ref<Model> AssetManager::GetModel(
    const fs::path& _path)
{
    return g_assetMgrCtx->Get(eAssetType::Model, _path).As<Model>();
}

Ref<Mesh> AssetManager::GetMesh(
    const fs::path& _path)
{
    return g_assetMgrCtx->Get(eAssetType::Mesh, _path).As<Mesh>();
}

Ref<Material> AssetManager::GetMaterial(
    const fs::path& _path)
{
    return g_assetMgrCtx->Get(eAssetType::Material, _path).As<Material>();
}

Ref<ImageTexture> AssetManager::GetImageTexture(
    const fs::path& _path)
{
    return g_assetMgrCtx->Get(eAssetType::ImageTexture, _path).As<ImageTexture>();
}

Optional<Ref<IAsset>> AssetManager::GetOrLoad(
    const eAssetType _type,
    const fs::path&  _path)
{
    return g_assetMgrCtx->GetOrLoad(_type, _path);
}

Optional<Ref<Model>> AssetManager::GetOrLoadModel(
    const fs::path& _path)
{
    if (const Optional<Ref<IAsset>> opt = g_assetMgrCtx->GetOrLoad(eAssetType::Model, _path))
    {
        return opt->As<Model>();
    }
    else
    {
        return k_nullopt;
    }
}

Optional<Ref<Mesh>> AssetManager::GetOrLoadMesh(
    const fs::path& _path)
{
    if (const Optional<Ref<IAsset>> opt = g_assetMgrCtx->GetOrLoad(eAssetType::Mesh, _path))
    {
        return opt->As<Mesh>();
    }
    else
    {
        return k_nullopt;
    }
}

Optional<Ref<Material>> AssetManager::GetOrLoadMaterial(
    const fs::path& _path)
{
    if (const Optional<Ref<IAsset>> opt = g_assetMgrCtx->GetOrLoad(eAssetType::Material, _path))
    {
        return opt->As<Material>();
    }
    else
    {
        return k_nullopt;
    }
}

Optional<Ref<ImageTexture>> AssetManager::GetOrLoadImageTexture(
    const fs::path& _path)
{
    if (const Optional<Ref<IAsset>> opt = g_assetMgrCtx->GetOrLoad(eAssetType::ImageTexture, _path))
    {
        return opt->As<ImageTexture>();
    }
    else
    {
        return k_nullopt;
    }
}

void AssetManager::Unload(
    const eAssetType _type,
    const fs::path&  _path)
{
    g_assetMgrCtx->Unload(_type, _path);
}

void AssetManager::UnloadModel(
    const fs::path& _path)
{
    Unload(eAssetType::Model, _path);
}

void AssetManager::UnloadMesh(
    const fs::path& _path)
{
    Unload(eAssetType::Mesh, _path);
}

void AssetManager::UnloadMaterial(
    const fs::path& _path)
{
    Unload(eAssetType::Material, _path);
}

void AssetManager::UnloadImageTexture(
    const fs::path& _path)
{
    Unload(eAssetType::ImageTexture, _path);
}

void AssetManager::CleanGarbage(
    const eAssetType _type)
{
    g_assetMgrCtx->CleanGarbage(_type);
}

void AssetManager::CleanGarbageAll()
{
    for (const eAssetType type: EnumRanges<eAssetType>())
    {
        g_assetMgrCtx->CleanGarbage(type);
    }
}

void AssetManager::Clear(
    const eAssetType _type)
{
    g_assetMgrCtx->Clear(_type);
}

void AssetManager::ClearAll()
{
    for (const eAssetType type: EnumRanges<eAssetType>())
    {
        g_assetMgrCtx->Clear(type);
    }
}

}   // namespace jam