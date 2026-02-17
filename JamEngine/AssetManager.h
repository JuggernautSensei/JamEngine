#pragma once
#include "EnumArray.h"
#include "IAsset.h"

namespace Jam
{
class FileDeletedEvent;
class FileRenamedEvent;
class FileModifiedEvent;

class Texture;
class Material;
class Mesh;
class Model;
class IAsset;
class Event;

class AssetManager
{
    using AssetMap     = HashMap<Path, Ref<IAsset>>;
    using AssetMapIter = AssetMap::iterator;

public:
    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&)                 = delete;
    AssetManager& operator=(AssetManager&&)      = delete;

    [[nodiscard]] static AssetManager& GetInstance();

    Expect<Ref<IAsset>>   Load(eAssetType _type, const Path& _path, bool _bReloadIfExist = false);
    Expect<Ref<Model>>    LoadModel(const Path& _path, bool _bForceReload = false);
    Expect<Ref<Mesh>>     LoadMesh(const Path& _path, bool _bForceReload = false);
    Expect<Ref<Material>> LoadMaterial(const Path& _path, bool _bForceReload = false);
    Expect<Ref<Texture>>  LoadTexture(const Path& _path, bool _bForceReload = false);

    void Unload(eAssetType _type, const Path& _path);
    void UnloadModel(const Path& _path);
    void UnloadMesh(const Path& _path);
    void UnloadMaterial(const Path& _path);
    void UnloadTexture(const Path& _path);

    void CleanGarbage(eAssetType _type);
    void CleanGarbageAll();

    void Clear(eAssetType _type);
    void ClearAll();

private:
    AssetManager()  = default;
    ~AssetManager() = default;

    static void CreateInstance();
    static void DestroyInstance();

    void OnEvent(Event& _event);
    void OnFileModifiedEvent_(const FileModifiedEvent& _event);
    void OnFileRenamedEvent_(const FileRenamedEvent& _event);
    void OnFileDeletedEvent_(const FileDeletedEvent& _event);

    void Unload_(AssetMap& _pAssetMap, AssetMapIter _iter) const;

    EnumArray<eAssetType, AssetMap> m_assetMaps = {};

    inline static AssetManager* s_pInstance = nullptr;
    friend class Application;
};

}   // namespace Jam

#define JAM_ASSET_MGR Jam::AssetManager::GetInstance()
