#pragma once
#include "IAsset.h"

namespace jam
{

class Material;
class Mesh;
class Model;
class IAsset;
class Event;

class AssetManager
{
public:
    static void Init();
    static void Shutdown();
    static void OnEvent(Event& _event);

    static Optional<Ref<IAsset>>       Load(eAssetType _type, const fs::path& _path, bool _bForceReload = false);
    static Optional<Ref<Model>>        LoadModel(const fs::path& _path, bool _bForceReload = false);
    static Optional<Ref<Mesh>>         LoadMesh(const fs::path& _path, bool _bForceReload = false);
    static Optional<Ref<Material>>     LoadMaterial(const fs::path& _path, bool _bForceReload = false);
    static Optional<Ref<ImageTexture>> LoadImageTexture(const fs::path& _path, bool _bForceReload = false);

    static void Unload(eAssetType _type, const fs::path& _path);
    static void UnloadModel(const fs::path& _path);
    static void UnloadMesh(const fs::path& _path);
    static void UnloadMaterial(const fs::path& _path);
    static void UnloadImageTexture(const fs::path& _path);

    static void CleanGarbage(eAssetType _type);
    static void CleanGarbageAll();

    static void Clear(eAssetType _type);
    static void ClearAll();
};

}   // namespace jam