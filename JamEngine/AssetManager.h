#pragma once
#include "ImageTexture.h"
#include "Material.h"
#include "Mesh.h"
#include "Model.h"

namespace jam
{

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

    [[nodiscard]] static Ref<IAsset>       Get(eAssetType _type, const fs::path& _path);
    [[nodiscard]] static Ref<Model>        GetModel(const fs::path& _path);
    [[nodiscard]] static Ref<Mesh>         GetMesh(const fs::path& _path);
    [[nodiscard]] static Ref<Material>     GetMaterial(const fs::path& _path);
    [[nodiscard]] static Ref<ImageTexture> GetImageTexture(const fs::path& _path);

    [[nodiscard]] static Optional<Ref<IAsset>>       GetOrLoad(eAssetType _type, const fs::path& _path);
    [[nodiscard]] static Optional<Ref<Model>>        GetOrLoadModel(const fs::path& _path);
    [[nodiscard]] static Optional<Ref<Mesh>>         GetOrLoadMesh(const fs::path& _path);
    [[nodiscard]] static Optional<Ref<Material>>     GetOrLoadMaterial(const fs::path& _path);
    [[nodiscard]] static Optional<Ref<ImageTexture>> GetOrLoadImageTexture(const fs::path& _path);

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