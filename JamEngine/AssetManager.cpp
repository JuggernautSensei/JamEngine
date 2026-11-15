#include "pch.h"

#include "AssetManager.h"

#include "Application.h"
#include "Events.h"
#include "FileSystem.h"

#include "Material.h"
#include "Mesh.h"
#include "Texture.h"
#include "Model.h"

namespace Jam
{

namespace
{

    [[nodiscard]] Expect<String> CreateAssetPath_(
        const StringView _rawPath)
    {
        Application&     app  = GetApp();
        const StringView base = app.GetWorkingDirectory();
        const String     path = FS::Relative(base, _rawPath);

        // check path is under working directory
        if (!base.IsBasePathOf(path))
        {
            JAM_LOG_ERROR("IAsset path '{}' is not under working directory '{}'.", path, base);
            return Error::NotIdealAssetPath;
        }

        return path;
    }

    [[nodiscard]] Expect<Ref<IAsset>> CreateAsset_(
        const StringView _assetPath,
        const eAsset     _type)
    {
        switch (_type)
        {
            case eAsset::Texture:
                return Texture::LoadFromFile(_assetPath);
            case eAsset::Material:
                return Material::LoadFromFile(_assetPath);
            case eAsset::Mesh:
                return Mesh::LoadFromFile(_assetPath);
            case eAsset::Model:
                return Model::LoadFromFile(_assetPath);
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
    const eAsset     _type,
    const StringView _path,
    const bool       _bReloadIfExist)
{
    const Expect<String> path = CreateAssetPath_(_path);
    if (!path)
    {
        return path.GetError();
    }

    const StringView key = path.GetValue();
    AssetMap&        map = m_assetMaps[_type];
    AssetMapIter     it  = map.find(key);

    // 만약 존재할 경우
    if (it != map.end())
    {
        // 필요시 리로드
        if (_bReloadIfExist)
        {
            Ref<IAsset>& pAsset = it->second;
            if (const Error err = pAsset->LoadFromFile_(key))
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
    Expect<Ref<IAsset>> pAsset = CreateAsset_(key, _type);
    if (pAsset)
    {
        map[key] = pAsset.GetValue();
    }

    return pAsset;
}

Expect<Ref<Model>> AssetManager::LoadModel(
    const StringView _path,
    const bool       _bForceReload)
{
    if (const Expect<Ref<IAsset>> pAsset = Load(eAsset::Model, _path, _bForceReload))
    {
        return pAsset->As<Model>();
    }
    else
    {
        return pAsset.GetError();
    }
}

Expect<Ref<Mesh>> AssetManager::LoadMesh(
    const StringView _path,
    const bool       _bForceReload)
{
    if (const Expect<Ref<IAsset>> pAsset = Load(eAsset::Mesh, _path, _bForceReload))
    {
        return pAsset->As<Mesh>();
    }
    else
    {
        return pAsset.GetError();
    }
}

Expect<Ref<Material>> AssetManager::LoadMaterial(
    const StringView _path,
    const bool       _bForceReload)
{
    if (const Expect<Ref<IAsset>> pAsset = Load(eAsset::Material, _path, _bForceReload))
    {
        return pAsset->As<Material>();
    }
    else
    {
        return pAsset.GetError();
    }
}

Expect<Ref<Texture>> AssetManager::LoadTexture(
    const StringView _path,
    const bool       _bForceReload)
{
    if (const Expect<Ref<IAsset>> pAsset = Load(eAsset::Texture, _path, _bForceReload))
    {
        return pAsset->As<Texture>();
    }
    else
    {
        return pAsset.GetError();
    }
}

void AssetManager::Unload(
    const eAsset     _type,
    const StringView _path)
{
    const Expect<String> path = CreateAssetPath_(_path);
    if (!path)
    {
        return;
    }

    const StringView key = path.GetValue();
    AssetMap&        map = m_assetMaps[_type];
    AssetMapIter     it  = map.find(key);
    if (it != map.end())
    {
        Unload_(map, it);
    }
}

void AssetManager::UnloadModel(
    const StringView _path)
{
    Unload(eAsset::Model, _path);
}

void AssetManager::UnloadMesh(
    const StringView _path)
{
    Unload(eAsset::Mesh, _path);
}

void AssetManager::UnloadMaterial(
    const StringView _path)
{
    Unload(eAsset::Material, _path);
}

void AssetManager::UnloadTexture(
    const StringView _path)
{
    Unload(eAsset::Texture, _path);
}

void AssetManager::CleanGarbage(
    const eAsset _type)
{
    AssetMap& map = m_assetMaps[_type];
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
    for (const eAsset type: RangesOf<eAsset>())
    {
        CleanGarbage(type);
    }
}

void AssetManager::Clear(
    const eAsset _type)
{
    AssetMap& map = m_assetMaps[_type];
    for (AssetMapIter it = map.begin(); it != map.end(); ++it)
    {
        Ref<IAsset>& pAsset = it->second;
        pAsset->Invalidate();
    }
    map.clear();
}

void AssetManager::ClearAll()
{
    for (const eAsset type: RangesOf<eAsset>())
    {
        Clear(type);
    }
}

void AssetManager::CreateInstance()
{
    JAM_ASSERT(s_pInstance == nullptr, "AssetManager instance already exists on CreateInstance_.");
    s_pInstance = new AssetManager;
}

void AssetManager::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
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
    const Expect<String> path = CreateAssetPath_(_event.GetPath());
    if (!path)
    {
        return;
    }

    const StringView key = path.GetValue();
    for (eAsset type: RangesOf<eAsset>())
    {
        AssetMap&    map = m_assetMaps[type];
        AssetMapIter it  = map.find(key);
        if (it != map.end())
        {
            Ref<IAsset>& pAsset = it->second;
            pAsset->LoadFromFile_(key);
        }
    }
}

void AssetManager::OnFileRenamedEvent_(
    const FileRenamedEvent& _event)
{
    Expect<String> oldPath = CreateAssetPath_(_event.GetOldPath());
    Expect<String> newPath = CreateAssetPath_(_event.GetNewPath());
    if (!oldPath || !newPath)   // unlikely...
    {
        return;
    }

    const StringView oldKey = oldPath.GetValue();
    const StringView newKey = newPath.GetValue();
    if (oldKey == newKey)   // unlikely...
    {
        return;
    }

    // for each asset type
    for (eAsset type: RangesOf<eAsset>())
    {
        // 없으면 스킵
        AssetMap&    map   = m_assetMaps[type];
        AssetMapIter oldIt = map.find(oldKey);
        if (oldIt == map.end())
        {
            continue;
        }

        // 1. 새로운 경로에 존재하는 에셋을 unload (덮어쓰기 방식)
        AssetMapIter newIt = map.find(newKey);
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
        const Error err = pAsset->LoadFromFile_(newKey);
        if (err)
        {
            continue;
        }

        // 5. 맵에 삽입
        map[newKey] = pAsset;
    }
}

void AssetManager::OnFileDeletedEvent_(
    const FileDeletedEvent& _event)
{
    // 존재하면 언로드
    const Expect<String> path = CreateAssetPath_(_event.GetPath());
    if (!path)
    {
        return;
    }

    const StringView key = path.GetValue();
    for (const eAsset type: RangesOf<eAsset>())
    {
        AssetMap&    map = m_assetMaps[type];
        AssetMapIter it  = map.find(key);
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

}   // namespace Jam