#include "pch.h"

#include "AssetManager.h"

#include "Application.h"
#include "Events.h"

#include "Material.h"
#include "Mesh.h"
#include "ImageTexture.h"
#include "Model.h"

namespace jam
{

struct AssetManagerContext
{
    using AssetMap     = HashMap<fs::path, Ref<IAsset>>;
    using AssetMapIter = AssetMap::iterator;

    [[nodiscard]] fs::path    ProcessPath(const fs::path& _path) const;
    [[nodiscard]] Ref<IAsset> MakeAssetRef(eAssetType _type) const;

    void OnFileModifiedEvent(const FileModifiedEvent& _event);
    void OnFileRenamedEvent(const FileRenamedEvent& _event);
    void OnFileDeletedEvent(const FileDeletedEvent& _event);

    [[nodiscard]] AssetMapIter Find(eAssetType _type, const fs::path& _processedPath);
    [[nodiscard]] bool         Has(eAssetType _type, AssetMapIter _it);
    Optional<Ref<IAsset>>      Load(eAssetType _type, const fs::path& _processedPath, bool _bReloadIfExist);
    Optional<Ref<IAsset>>      Reload(eAssetType _type, AssetMapIter _it);
    void                       Unload(eAssetType _type, AssetMapIter _it);
    void                       Rename(eAssetType _type, AssetMapIter _oldIt, const fs::path& _newProcessedPath);

    [[nodiscard]] AssetMap& GetAssetMap(eAssetType _type);

    void Clear(eAssetType _type);
    void CleanGarbage(eAssetType _type);

    Array<AssetMap, NumEnums<eAssetType>()> assetMaps;

}* g_assetMgrCtx = nullptr;

fs::path AssetManagerContext::ProcessPath(
    const fs::path& _path) const
{
    const Application& app        = Application::GetInstance();
    const fs::path&    workingDir = app.GetWorkingDirectory();
    return fs::relative(_path, workingDir);
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
    // process path
    const fs::path& oldPath = ProcessPath(_event.GetOldPath());
    const fs::path& newPath = ProcessPath(_event.GetNewPath());

    // for each asset type
    for (eAssetType type: eAssetType {})
    {
        // 없으면 스킵
        AssetMapIter oldIt = Find(type, oldPath);
        if (Has(type, oldIt) == false)
        {
            continue;
        }

        // rename
        Rename(type, oldIt, newPath);
    }
}

void AssetManagerContext::OnFileDeletedEvent(
    const FileDeletedEvent& _event)
{
    // unload existing asset
    const fs::path path = ProcessPath(_event.GetPath());
    for (const eAssetType type: eAssetType {})
    {
        const AssetMapIter it = Find(type, path);
        if (Has(type, it))
        {
            Unload(type, it);
        }
    }
}

AssetManagerContext::AssetMapIter AssetManagerContext::Find(
    const eAssetType _type,
    const fs::path&  _processedPath)
{
    AssetMap& map = GetAssetMap(_type);
    return map.find(_processedPath);
}

bool AssetManagerContext::Has(const eAssetType _type, const AssetMapIter _it)
{
    AssetMap& map = GetAssetMap(_type);
    return _it != map.end();
}

Optional<Ref<IAsset>> AssetManagerContext::Load(
    const eAssetType _type,
    const fs::path&  _processedPath,
    const bool       _bReloadIfExist)
{
    AssetMapIter it  = Find(_type, _processedPath);
    AssetMap&    map = GetAssetMap(_type);

    // 만약 존재할 경우 그대로 리턴
    if (it != map.end())
    {
        if (_bReloadIfExist)
        {
            // 필요시 리로드
            return Reload(_type, it);
        }
        else
        {
            // 그대로 리턴
            return it->second;
        }
    }

    // 없으면 새롭게 생성 후 로드
    Ref<IAsset> pAsset = MakeAssetRef(_type);
    if (!pAsset->Load(_processedPath))
    {
        return k_nullopt;
    }

    map[_processedPath] = pAsset;
    return pAsset;
}

Optional<Ref<IAsset>> AssetManagerContext::Reload(const eAssetType _type, AssetMapIter _it)
{
    Ref<IAsset>& pAsset = _it->second;
    if (!pAsset->Load(pAsset->GetPath()))
    {
        AssetMap& map = GetAssetMap(_type);
        map.erase(_it);   // remove invalid asset
        return k_nullopt;
    }
    return pAsset;
}

void AssetManagerContext::Unload(const eAssetType _type, AssetMapIter _it)
{
    AssetMap& map = GetAssetMap(_type);
    JAM_ASSERT(_it != map.end(), "AssetManagerContext::Unload() called with invalid iterator.");
    Ref<IAsset>& pAsset = _it->second;
    pAsset->Unload();
    map.erase(_it);
}

void AssetManagerContext::Rename(
    const eAssetType _type,
    AssetMapIter     _oldIt,
    const fs::path&  _newProcessedPath)
{
    // new path에 이미 에셋이 있으면 unload.
    const AssetMapIter it = Find(_type, _newProcessedPath);
    if (Has(_type, it))
    {
        Unload(_type, it);
    }

    // new path load
    Load(_type, _newProcessedPath, false);

    // old path unload
    Unload(_type, _oldIt);
}

AssetManagerContext::AssetMap& AssetManagerContext::GetAssetMap(
    const eAssetType _type)
{
    return assetMaps[IntFrom(_type)];
}

void AssetManagerContext::OnFileModifiedEvent(
    const FileModifiedEvent& _event)
{
    // reload existing asset without creating new object
    const fs::path path = ProcessPath(_event.GetPath());
    for (const eAssetType type: eAssetType {})
    {
        // 존재하면 리로드
        AssetMapIter it = Find(type, path);
        if (Has(type, it))
        {
            Reload(type, it);
        }
    }
}

void AssetManagerContext::Clear(
    const eAssetType _type)
{
    AssetMap& map = GetAssetMap(_type);
    for (auto it = map.begin(); it != map.end();)
    {
        Unload(_type, it++);
    }
}

void AssetManagerContext::CleanGarbage(
    const eAssetType _type)
{
    AssetMap& map = GetAssetMap(_type);
    for (auto it = map.begin(); it != map.end();)
    {
        // if ref count is 1, it means only the manager holds it
        if (it->second->GetRefCount() == 1)
        {
            it = map.erase(it);
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
    const fs::path path = g_assetMgrCtx->ProcessPath(_path);
    return g_assetMgrCtx->Load(_type, path, _bForceReload);
}

Optional<Ref<Model>> AssetManager::LoadModel(
    const fs::path& _path,
    const bool      _bForceReload)
{
    Ref<IAsset> pAsset;
    if (!MoveIfHasValue(Load(eAssetType::Model, _path, _bForceReload), pAsset))
    {
        return k_nullopt;
    }

    return pAsset.As<Model>();
}

Optional<Ref<Mesh>> AssetManager::LoadMesh(
    const fs::path& _path,
    const bool      _bForceReload)
{
    Ref<IAsset> pAsset;
    if (!MoveIfHasValue(Load(eAssetType::Mesh, _path, _bForceReload), pAsset))
    {
        return k_nullopt;
    }

    return pAsset.As<Mesh>();
}

Optional<Ref<Material>> AssetManager::LoadMaterial(
    const fs::path& _path,
    const bool      _bForceReload)
{
    Ref<IAsset> pAsset;
    if (!MoveIfHasValue(Load(eAssetType::Material, _path, _bForceReload), pAsset))
    {
        return k_nullopt;
    }

    return pAsset.As<Material>();
}

Optional<Ref<ImageTexture>> AssetManager::LoadImageTexture(
    const fs::path& _path,
    const bool      _bForceReload)
{
    Ref<IAsset> pAsset;
    if (!MoveIfHasValue(Load(eAssetType::ImageTexture, _path, _bForceReload), pAsset))
    {
        return k_nullopt;
    }

    return pAsset.As<ImageTexture>();
}

void AssetManager::Unload(
    const eAssetType _type,
    const fs::path&  _path)
{
    const fs::path                    path = g_assetMgrCtx->ProcessPath(_path);
    AssetManagerContext::AssetMapIter it   = g_assetMgrCtx->Find(_type, path);
    if (g_assetMgrCtx->Has(_type, it))
    {
        g_assetMgrCtx->Unload(_type, it);
    }
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
    for (const eAssetType type: eAssetType {})
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
    for (const eAssetType type: eAssetType {})
    {
        g_assetMgrCtx->Clear(type);
    }
}

}   // namespace jam