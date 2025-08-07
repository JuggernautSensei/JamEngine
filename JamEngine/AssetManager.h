#pragma once
#include "Asset.h"
#include "EnumUtilities.h"
#include "Model.h"

#include <memory>
#include <optional>

namespace jam
{
class TextureAsset;
class ModelAsset;

class AssetManager
{
public:
    using Container = std::unordered_map<fs::path, Ref<Asset>>;

    AssetManager()  = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&)                = delete;
    AssetManager& operator=(const AssetManager&)     = delete;
    AssetManager(AssetManager&&) noexcept            = default;
    AssetManager& operator=(AssetManager&&) noexcept = default;

    // Loadxxx : load asset and get. if duplicated, overwrite and return the new one. return nullopt if failed.
    // GetOrLoadxxx : get asset from the container. if not found, load it and return the new one. return nullopt if failed.
    // Unloadxxx : unload asset and remove from the container.
    // Getxxx : get asset from the container. if not found, return std::nullopt. return nullopt if not exists.

    std::optional<Ref<ModelAsset>> LoadModel(const fs::path& _path);
    void UnloadModel(const fs::path& _path);
    NODISCARD std::optional<Ref<ModelAsset>> GetModel(const fs::path& _path) const;

    std::optional<Ref<TextureAsset>> LoadTexture(const fs::path& _path);
    void UnloadTexture(const fs::path& _path);
    NODISCARD std::optional<Ref<TextureAsset>> GetTexture(const fs::path& _path) const;

    NODISCARD const auto& GetContainer(const eAssetType _type) const { return GetContainer_(_type); }
    void                  Clear(eAssetType _type);
    void                  ClearAll();

private:
    std::optional<Ref<Asset>> GetOrLoadAsset_(eAssetType _type, const fs::path& _path);
    std::optional<Ref<Asset>> LoadAsset_(eAssetType _type, const fs::path& _path);
    void UnloadAsset_(eAssetType _type, const fs::path& _path);
    NODISCARD std::optional<Ref<Asset>> GetAsset_(eAssetType _type, const fs::path& _path) const;
    NODISCARD Ref<Asset> CreateAsset_(eAssetType _type) const;
    NODISCARD Container&             GetContainer_(eAssetType _type);
    NODISCARD const Container&       GetContainer_(eAssetType _type) const;

    Container m_containers[EnumCount<eAssetType>()];
};

}   // namespace jam