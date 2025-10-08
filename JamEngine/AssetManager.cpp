#include "pch.h"

#include "AssetManager.h"

#include "Application.h"
#include "ResourcePool.h"
using namespace jam;

namespace
{

struct AssetManagerContext
{
    template<IsAssetBased AssetT>
    JAM_NODISCARD auto& GetHandleHashMap()
    {
        if constexpr (std::is_same_v<AssetT, Mesh>)
            return meshHandleMap;
        else if constexpr (std::is_same_v<AssetT, Material>)
            return materialHandleMap;
        else if constexpr (std::is_same_v<AssetT, ImageTexture>)
            return imageTextureMap;
        else
            static_assert(AlwaysFalse, "Unsupported asset type");
    }

    template<IsAssetBased AssetT>
    JAM_NODISCARD const auto& GetHandleHashMap() const
    {
        if constexpr (std::is_same_v<AssetT, Mesh>)
            return meshHandleMap;
        else if constexpr (std::is_same_v<AssetT, Material>)
            return materialHandleMap;
        else if constexpr (std::is_same_v<AssetT, ImageTexture>)
            return imageTextureMap;
        else
            static_assert(AlwaysFalse, "Unsupported asset type");
    }

    template<IsAssetBased AssetT>
    JAM_NODISCARD auto& GetResourcePool()
    {
        if constexpr (std::is_same_v<AssetT, Mesh>)
            return meshes;
        else if constexpr (std::is_same_v<AssetT, Material>)
            return materials;
        else if constexpr (std::is_same_v<AssetT, ImageTexture>)
            return imageTextures;
        else
            static_assert(AlwaysFalse, "Unsupported asset type");
    }

    template<IsAssetBased AssetT, typename HandleT = Handle<AssetT>>
    Result<HandleT> LoadAsset(const fs::path& _path, const bool _bOverwrite)
    {
        fs::path path = CreateAssetPathOrEmpty(_path);
        JAM_ASSERT(!path.empty(), "Asset path must be inside working directory: {}", _path.native());

        AssetT     asset;
        eErrorCode ec = asset.Load(path);
        if (IsError(ec))
        {
            return ec;
        }

        // find existing
        auto& pool        = GetResourcePool<AssetT>();
        auto& handleHashT = GetHandleHashMap<AssetT>();
        auto  it          = handleHashT.find(path);
        if (it != handleHashT.end())
        {
            HandleT handle = it->second;
            if (_bOverwrite)   // 덮어 쓰기
            {
                // 핸들을 따로 할당하지 않고 기존 핸들에 덮어 씀
                AssetT& existing = pool[handle];
                existing.Unload();
                existing = std::move(asset);
            }
            return handle;
        }
        else
        {
            // insert new
            HandleT newHandle = pool.Alloc();
            pool[newHandle]   = std::move(asset);
            handleHashT[path] = newHandle;
            return newHandle;
        }
    }

    template<IsAssetBased AssetT, typename HandleT = Handle<AssetT>>
    void UnloadAsset(HandleT _handle)
    {
        auto&           pool  = GetResourcePool<AssetT>();
        auto&           asset = pool[_handle];
        const fs::path& path  = asset.GetPath();

        // unload
        asset.Unload();

        // remove from map
        auto& handleHashT = GetHandleHashMap<AssetT>();
        handleHashT.erase(path);

        // free from pool
        pool.Free(_handle);
    }

    template<IsAssetBased AssetT, typename HandleT = Handle<AssetT>>
    JAM_NODISCARD HandleT GetAssetHandle(const fs::path& _path)
    {
        fs::path path = CreateAssetPathOrEmpty(_path);
        JAM_ASSERT(!path.empty(), "Asset path must be inside working directory: {}", _path.native());

        auto& handleHashT = GetHandleHashMap<AssetT, HandleT>();
        auto  it          = handleHashT.find(path);
        JAM_ASSERT(it != handleHashT.end(), "Asset not loaded: {}", path.native());
        return it->second;
    }

    template<IsAssetBased AssetT>
    JAM_NODISCARD bool IsAssetLoaded(const fs::path& _path)
    {
        fs::path path = CreateAssetPathOrEmpty(_path);
        if (path.empty())
        {
            return false;
        }

        auto& handleHashT = GetHandleHashMap<AssetT>();
        return handleHashT.contains(path);
    }

    template<IsAssetBased AssetT, typename HandleT = Handle<AssetT>>
    JAM_NODISCARD Result<HandleT> GetOrLoadAsset(const fs::path& _path)
    {
        if (IsAssetLoaded<AssetT>(_path))
        {
            return GetAssetHandle<AssetT>(_path);
        }
        else
        {
            return LoadAsset<AssetT>(_path, false);
        }
    }

    template<IsAssetBased AssetT, typename HandleT = Handle<AssetT>>
    JAM_NODISCARD const AssetT& GetAsset(HandleT _handle)
    {
        auto& pool = GetResourcePool<AssetT>();
        return pool[_handle];
    }

    template<IsAssetBased AssetT, typename HandleT = Handle<AssetT>>
    JAM_NODISCARD eErrorCode SaveAsset(HandleT _handle, const fs::path& _path)
    {
        auto& asset = GetAsset<AssetT>(_handle);
        return asset.Save(_path);
    }

    JAM_NODISCARD fs::path CreateAssetPathOrEmpty(const fs::path& _path) const
    {
        // working directory 로 부터 상대 경로 생성
        const Application&        app     = Application::GetInstance();
        const ProjectDirectories& projDir = app.GetProjectDirectories();

        std::error_code ec;
        fs::path        path = fs::relative(_path, projDir.workingDir, ec);
        if (ec)
        {
            return {};
        }

        // working directory 내부 경로가 아니라면 빈 경로 반환
        std::string_view str = path.native();
        if (str.empty() || str.starts_with(".."))
        {
            return {};
        }

        return path;
    }

    HashMap<fs::path, MeshHandle>         meshHandleMap;
    HashMap<fs::path, MaterialHandle>     materialHandleMap;
    HashMap<fs::path, ImageTextureHandle> imageTextureMap;

    ResourcePool<Mesh>         meshes;
    ResourcePool<Material>     materials;
    ResourcePool<ImageTexture> imageTextures;
};

AssetManagerContext g_ctx;

}   // namespace

namespace jam
{
namespace asset_manager
{

    bool DetectLeak()
    {
        bool bLeakDetected = false;

        for (const auto& [path, handle]: g_ctx.meshHandleMap)
        {
            log::Warn("Leaked Mesh Asset: {} (Handle: {})", path.native(), handle.value);
            bLeakDetected = true;
        }

        for (const auto& [path, handle]: g_ctx.materialHandleMap)
        {
            log::Warn("Leaked Material Asset: {} (Handle: {})", path.native(), handle.value);
            bLeakDetected = true;
        }

        for (const auto& [path, handle]: g_ctx.imageTextureMap)
        {
            log::Warn("Leaked ImageTexture Asset: {} (Handle: {})", path.native(), handle.value);
            bLeakDetected = true;
        }

        return bLeakDetected;
    }

    Result<MeshHandle> LoadMesh(const fs::path& _path, const bool _bOverwrite)
    {
        return g_ctx.LoadAsset<Mesh>(_path, _bOverwrite);
    }

    Result<MaterialHandle> LoadMaterial(const fs::path& _path, const bool _bOverwrite)
    {
        return g_ctx.LoadAsset<Material>(_path, _bOverwrite);
    }

    Result<ImageTextureHandle> LoadImageTexture(const fs::path& _path, const bool _bOverwrite)
    {
        return g_ctx.LoadAsset<ImageTexture>(_path, _bOverwrite);
    }

    void UnloadMesh(const MeshHandle _handle)
    {
        g_ctx.UnloadAsset<Mesh>(_handle);
    }

    void UnloadMaterial(const MaterialHandle _handle)
    {
        g_ctx.UnloadAsset<Material>(_handle);
    }

    void UnloadImageTexture(const ImageTextureHandle _handle)
    {
        g_ctx.UnloadAsset<ImageTexture>(_handle);
    }

    MeshHandle GetMeshHandle(const fs::path& _path)
    {
        return g_ctx.GetAssetHandle<Mesh>(_path);
    }

    MaterialHandle GetMaterialHandle(const fs::path& _path)
    {
        return g_ctx.GetAssetHandle<Material>(_path);
    }

    ImageTextureHandle GetImageTextureHandle(const fs::path& _path)
    {
        return g_ctx.GetAssetHandle<ImageTexture>(_path);
    }

    Result<MeshHandle> GetOrLoadMesh(const fs::path& _path)
    {
        return g_ctx.GetOrLoadAsset<Mesh>(_path);
    }

    Result<MaterialHandle> GetOrLoadMaterial(const fs::path& _path)
    {
        return g_ctx.GetOrLoadAsset<Material>(_path);
    }

    Result<ImageTextureHandle> GetOrLoadImageTexture(const fs::path& _path)
    {
        return g_ctx.GetOrLoadAsset<ImageTexture>(_path);
    }

    bool IsMeshLoaded(const fs::path& _path)
    {
        return g_ctx.IsAssetLoaded<Mesh>(_path);
    }

    bool IsMaterialLoaded(const fs::path& _path)
    {
        return g_ctx.IsAssetLoaded<Material>(_path);
    }

    bool IsImageTextureLoaded(const fs::path& _path)
    {
        return g_ctx.IsAssetLoaded<ImageTexture>(_path);
    }

    const Mesh& GetMesh(const MeshHandle _handle)
    {
        return g_ctx.GetAsset<Mesh>(_handle);
    }

    const Material& GetMaterial(const MaterialHandle _handle)
    {
        return g_ctx.GetAsset<Material>(_handle);
    }

    const ImageTexture& GetImageTexture(const ImageTextureHandle _handle)
    {
        return g_ctx.GetAsset<ImageTexture>(_handle);
    }

    eErrorCode SaveMesh(const MeshHandle _handle, const fs::path& _path)
    {
        return g_ctx.SaveAsset<Mesh>(_handle, _path);
    }

    eErrorCode SaveMaterial(const MaterialHandle _handle, const fs::path& _path)
    {
        return g_ctx.SaveAsset<Material>(_handle, _path);
    }

    eErrorCode SaveImageTexture(const ImageTextureHandle _handle, const fs::path& _path)
    {
        return g_ctx.SaveAsset<ImageTexture>(_handle, _path);
    }

    const HashMap<fs::path, MeshHandle>& GetAllMeshHandles()
    {
        return g_ctx.GetHandleHashMap<Mesh>();
    }

    const HashMap<fs::path, MaterialHandle>& GetAllMaterialHandles()
    {
        return g_ctx.GetHandleHashMap<Material>();
    }

    const HashMap<fs::path, ImageTextureHandle>& GetAllImageTextureHandles()
    {
        return g_ctx.GetHandleHashMap<ImageTexture>();
    }

}   // namespace asset_manager
}   // namespace jam
