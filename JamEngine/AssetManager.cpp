#include "pch.h"

#include "AssetManager.h"

#include "Application.h"
#include "Events.h"

#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "Model.h"

namespace Jam
{

namespace
{

    [[nodiscard]] Path CreateProcessedPath(
        const Path& _path)
    {
        const Path& workingDir = JAM_APP.GetWorkingDirectory();
        return fs::relative(_path, workingDir);
    }

    [[nodiscard]] Expect<Ref<IAsset>> CreateAsset(
        const Path&      _processedPath,
        const eAssetType _type)
    {
        switch (_type)
        {
            case eAssetType::Texture:
                return Texture::Create(_processedPath);
            case eAssetType::Material:
                return Material::Create(_processedPath);
            case eAssetType::Mesh:
                return Mesh::Create(_processedPath);
            case eAssetType::Model:
                return Model::Create(_processedPath);
            default:
                JAM_UNREACHABLE;
        }
    }

}   // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////

AssetManager& AssetManager::GetInstance()
{
    return *s_pInstance;
}

Expect<Ref<IAsset>> AssetManager::Load(
    const eAssetType _type,
    const Path&      _path,
    const bool       _bReloadIfExist)
{
    const Path   path = CreateProcessedPath(_path);
    AssetMap&    map  = GetAssetMap_(_type);
    AssetMapIter it   = map.find(path);

    // 만약 존재할 경우
    if (it != map.end())
    {
        // 필요시 리로드
        if (_bReloadIfExist)
        {
            Ref<IAsset>& pAsset = it->second;
            if (const Error err = pAsset->Load(path))
            {
                // 실패해도 pAsset 자체가 invalid 상태가 아니므로
                // 에러만 리턴하고, 맵에서는 제거하지 않음
                return err;
            }

            return pAsset;
        }

        // 아니면 그대로 리턴
        return it->second;
    }

    // 없으면 새롭게 생성 후 로드
    Expect<Ref<IAsset>> pAsset = CreateAsset(path, _type);
    if (pAsset)
    {
        map[path] = pAsset.GetValue();
    }

    return pAsset;
}

Expect<Ref<Model>> AssetManager::LoadModel(
    const Path& _path,
    const bool  _bForceReload)
{
    if (const Expect<Ref<IAsset>> pAsset = Load(eAssetType::Model, _path, _bForceReload))
    {
        return pAsset->As<Model>();
    }
    else
    {
        return pAsset.GetError();
    }
}

Expect<Ref<Mesh>> AssetManager::LoadMesh(
    const Path& _path,
    const bool  _bForceReload)
{
    if (const Expect<Ref<IAsset>> pAsset = Load(eAssetType::Mesh, _path, _bForceReload))
    {
        return pAsset->As<Mesh>();
    }
    else
    {
        return pAsset.GetError();
    }
}

Expect<Ref<Material>> AssetManager::LoadMaterial(
    const Path& _path,
    const bool  _bForceReload)
{
    if (const Expect<Ref<IAsset>> pAsset = Load(eAssetType::Material, _path, _bForceReload))
    {
        return pAsset->As<Material>();
    }
    else
    {
        return pAsset.GetError();
    }
}

Expect<Ref<Texture>> AssetManager::LoadTexture(
    const Path& _path,
    const bool  _bForceReload)
{
    if (const Expect<Ref<IAsset>> pAsset = Load(eAssetType::Texture, _path, _bForceReload))
    {
        return pAsset->As<Texture>();
    }
    else
    {
        return pAsset.GetError();
    }
}

void AssetManager::Unload(
    const eAssetType _type,
    const Path&      _path)
{
    const Path   path = CreateProcessedPath(_path);
    AssetMap&    map  = GetAssetMap_(_type);
    AssetMapIter it   = map.find(path);
    if (it != map.end())
    {
        Unload_(map, it);
    }
}

void AssetManager::UnloadModel(
    const Path& _path)
{
    Unload(eAssetType::Model, _path);
}

void AssetManager::UnloadMesh(
    const Path& _path)
{
    Unload(eAssetType::Mesh, _path);
}

void AssetManager::UnloadMaterial(
    const Path& _path)
{
    Unload(eAssetType::Material, _path);
}

void AssetManager::UnloadTexture(
    const Path& _path)
{
    Unload(eAssetType::Texture, _path);
}

void AssetManager::CleanGarbage(
    const eAssetType _type)
{
    AssetMap& map = GetAssetMap_(_type);
    for (AssetMapIter it = map.begin(); it != map.end();)
    {
        Ref<IAsset>& pAsset = it->second;
        if (pAsset->GetRefCount() <= 1)
        {
            // map에서 asset을 제거하는 순간 invalidate 되므로 invalidate 호출 불필요
            it = map.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void AssetManager::CleanGarbageAll()
{
    for (const eAssetType type: EnumRanges<eAssetType>())
    {
        CleanGarbage(type);
    }
}

void AssetManager::Clear(
    const eAssetType _type)
{
    AssetMap& map = GetAssetMap_(_type);
    for (AssetMapIter it = map.begin(); it != map.end(); ++it)
    {
        Ref<IAsset>& pAsset = it->second;
        pAsset->Invalidate();
    }
    map.clear();
}

void AssetManager::ClearAll()
{
    for (const eAssetType type: EnumRanges<eAssetType>())
    {
        Clear(type);
    }
}

void AssetManager::CreateInstance()
{
    JAM_ASSERT(s_pInstance == nullptr, "AssetManager instance already exists on CreateInstance.");
    s_pInstance = new AssetManager;
}

void AssetManager::DestroyInstance()
{
    JAM_ASSERT(s_pInstance, "AssetManager instance is null on DestroyInstance.");
    delete s_pInstance;
}

void AssetManager::OnEvent(
    Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<FileModifiedEvent>(this, &AssetManager::OnFileModifiedEvent_);
    dispatcher.Dispatch<FileRenamedEvent>(this, &AssetManager::OnFileRenamedEvent_);
    dispatcher.Dispatch<FileDeletedEvent>(this, &AssetManager::OnFileDeletedEvent_);
}

void AssetManager::OnFileModifiedEvent_(
    const FileModifiedEvent& _event)
{
    // 존재하면 forced reload
    const Path path = CreateProcessedPath(_event.GetPath());
    for (eAssetType type: EnumRanges<eAssetType>())
    {
        AssetMap&    map = GetAssetMap_(type);
        AssetMapIter it  = map.find(path);
        if (it != map.end())
        {
            Ref<IAsset>& pAsset = it->second;
            pAsset->Load(path);
        }
    }
}

void AssetManager::OnFileRenamedEvent_(
    const FileRenamedEvent& _event)
{
    // process path
    const Path& oldPath = CreateProcessedPath(_event.GetOldPath());
    const Path& newPath = CreateProcessedPath(_event.GetNewPath());
    JAM_ASSERT(oldPath != newPath, "Old path and new path are the same in OnFileRenamedEvent_.");

    // for each asset type
    for (eAssetType type: EnumRanges<eAssetType>())
    {
        // 없으면 스킵
        AssetMap&    map   = GetAssetMap_(type);
        AssetMapIter oldIt = map.find(oldPath);
        if (oldIt == map.end())
        {
            continue;
        }

        // 1. 새로운 경로에 존재하는 에셋을 unload (덮어쓰기 방식)
        AssetMapIter newIt = map.find(newPath);
        if (newIt != map.end())
        {
            Unload_(map, newIt);
        }

        // 2. 기존 에셋의 참조 카운트 획득
        Ref<IAsset> pAsset = oldIt->second;

        // 3. 기존 경로에서 에셋 제거
        Unload_(map, oldIt);

        // 4. 에셋을 새로운 경로로 로드 및 맵에 삽입
        //    만약 실패했다면, 에셋은 무효화된 상태
        const Error err = pAsset->Load(newPath);
        if (err)
        {
            continue;
        }

        // 5. 맵에 삽입
        map[newPath] = pAsset;
    }
}

void AssetManager::OnFileDeletedEvent_(
    const FileDeletedEvent& _event)
{
    // 존재하면 언로드
    const Path path = CreateProcessedPath(_event.GetPath());
    for (const eAssetType type: EnumRanges<eAssetType>())
    {
        AssetMap&    map = GetAssetMap_(type);
        AssetMapIter it  = map.find(path);
        if (it != map.end())
        {
            Unload_(map, it);
        }
    }
}

void AssetManager::Unload_(
    AssetMap&    _pAssetMap,
    AssetMapIter _iter) const
{
    // 무효화 및 맵에서 제거
    Ref<IAsset>& pAsset = _iter->second;
    pAsset->Invalidate();
    _pAssetMap.erase(_iter);
}

AssetManager::AssetMap& AssetManager::GetAssetMap_(
    const eAssetType _type)
{
    return m_assetMaps[UnderlyingFrom(_type)];
}

}   // namespace Jam