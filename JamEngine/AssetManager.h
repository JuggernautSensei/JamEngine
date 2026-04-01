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
    using AssetMap     = StringHashMap<Ref<IAsset>>;
    using AssetMapIter = AssetMap::iterator;

public:
    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&)                 = delete;
    AssetManager& operator=(AssetManager&&)      = delete;

    [[nodiscard]] static AssetManager& GetInstance();

    Expect<Ref<IAsset>>   Load(eAsset _type, StringView _path, bool _bReloadIfExist = false);
    Expect<Ref<Model>>    LoadModel(StringView _path, bool _bForceReload = false);
    Expect<Ref<Mesh>>     LoadMesh(StringView _path, bool _bForceReload = false);
    Expect<Ref<Material>> LoadMaterial(StringView _path, bool _bForceReload = false);
    Expect<Ref<Texture>>  LoadTexture(StringView _path, bool _bForceReload = false);

    void Unload(eAsset _type, StringView _path);
    void UnloadModel(StringView _path);
    void UnloadMesh(StringView _path);
    void UnloadMaterial(StringView _path);
    void UnloadTexture(StringView _path);

    void CleanGarbage(eAsset _type);
    void CleanGarbageAll();

    void Clear(eAsset _type);
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

    EnumArray<eAsset, AssetMap> m_assetMaps = {};

    inline static AssetManager* s_pInstance = nullptr;
    friend class Application;
};

[[nodiscard]] inline AssetManager& GetAssetManager()
{
    return AssetManager::GetInstance();
}

}   // namespace Jam
