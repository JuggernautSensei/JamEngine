#pragma once
#include "Asset.h"
#include "EnumUtilities.h"

namespace jam
{
class TextureAsset;
class ModelAsset;

class AssetManager
{
public:
    using Container = std::unordered_map<fs::path, std::shared_ptr<Asset>>;

    AssetManager()  = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&)                = delete;
    AssetManager& operator=(const AssetManager&)     = delete;
    AssetManager(AssetManager&&) noexcept            = default;
    AssetManager& operator=(AssetManager&&) noexcept = default;

    std::optional<std::shared_ptr<ModelAsset>> LoadModel(const fs::path& _path);
    bool                                       UnloadModel(const fs::path& _path);
    NODISCARD std::optional<std::shared_ptr<ModelAsset>> GetModel(const fs::path& _path) const;

    std::optional<std::shared_ptr<TextureAsset>> LoadTexture(const fs::path& _path);
    bool                                         UnloadTexture(const fs::path& _path);
    NODISCARD std::optional<std::shared_ptr<TextureAsset>> GetTexture(const fs::path& _path) const;

    NODISCARD const auto& GetContainer(const eAssetType _type) const { return GetContainer_(_type); }
    void                  Clear(eAssetType _type);
    void                  ClearAll();

private:
    std::optional<std::shared_ptr<Asset>> LoadAsset_(eAssetType _type, const fs::path& _path);
    bool                                  UnloadAsset_(eAssetType _type, const fs::path& _path);
    NODISCARD std::optional<std::shared_ptr<Asset>> GetAsset_(eAssetType _type, const fs::path& _path) const;
    NODISCARD std::shared_ptr<Asset> CreateAsset_(eAssetType _type) const;
    NODISCARD Container&             GetContainer_(eAssetType _type);
    NODISCARD const Container&       GetContainer_(eAssetType _type) const;

    Container m_assetMap[EnumCount<eAssetType>()];
};

}   // namespace jam