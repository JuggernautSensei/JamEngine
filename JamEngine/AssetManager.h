#pragma once
#include "Asset.h"
#include "EnumUtilities.h"

namespace jam
{

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

    template<typename T>
    std::optional<Ref<T>> Load(const fs::path& _path)
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        std::optional<Ref<Asset>> assetOrNull = LoadAsset_(T::s_type, _path);
        if (assetOrNull)
        {
            return std::static_pointer_cast<T>(*assetOrNull);
        }
        else
        {
            JAM_ERROR("AssetManager::Load() - Failed to load asset from item: {}", _path.string());
            return std::nullopt;   // Loading failed
        }
    }

    template<typename T>
    std::optional<Ref<T>> Reload(const fs::path& _path) const
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        std::optional<Ref<Asset>> assetOrNull = ReloadAsset_(T::s_type, _path);
        if (assetOrNull)
        {
            return std::static_pointer_cast<T>(*assetOrNull);
        }
        else
        {
            JAM_ERROR("AssetManager::Reload() - Failed to reload asset from item: {}", _path.string());
            return std::nullopt;   // Reloading failed
        }
    }

    template<typename T>
    NODISCARD std::optional<Ref<T>> GetOrLoad(const fs::path& _path)
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        std::optional<Ref<Asset>> assetOrNull = GetOrLoadAsset_(T::s_type, _path);
        if (assetOrNull)
        {
            return std::static_pointer_cast<T>(*assetOrNull);
        }
        else
        {
            JAM_ERROR("AssetManager::GetOrLoad() - Failed to get or load asset from item: {}", _path.string());
            return std::nullopt;   // Getting or loading failed
        }
    }

    template<typename T>
    NODISCARD std::optional<Ref<T>> Get(const fs::path& _path) const
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        std::optional<Ref<Asset>> assetOrNull = ReloadAsset_(T::s_type, _path);
        if (assetOrNull)
        {
            return std::static_pointer_cast<T>(*assetOrNull);
        }
        else
        {
            JAM_ERROR("AssetManager::Get() - Asset not found at item: {}", _path.string());
            return std::nullopt;   // Asset not found
        }
    }

    template<typename T>
    void Unload(const fs::path& _path)
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        UnloadAsset_(T::s_type, _path);
    }

    NODISCARD const Container& GetContainer(const eAssetType _type) const { return GetContainer_(_type); }
    NODISCARD Container&       GetContainerRef(const eAssetType _type) { return GetContainer_(_type); }

    NODISCARD size_t GetAssetCount(const eAssetType _type) const
    {
        JAM_ASSERT(IsValidEnum(_type), "AssetManager::GetAssetCount() - Invalid asset type");
        return GetContainer_(_type).size();
    }

    NODISCARD size_t GetTotalAssetCount() const
    {
        size_t totalCount = 0;
        for (const Container& container: m_containers)
        {
            totalCount += container.size();
        }
        return totalCount;
    }

    void ClearAll();
    void Clear(eAssetType _type);

private:
    std::optional<Ref<Asset>> LoadAsset_(eAssetType _type, const fs::path& _path);
    std::optional<Ref<Asset>> ReloadAsset_(eAssetType _type, const fs::path& _path) const;
    NODISCARD std::optional<Ref<Asset>> GetOrLoadAsset_(eAssetType _type, const fs::path& _path);
    NODISCARD std::optional<Ref<Asset>> GetAsset_(eAssetType _type, const fs::path& _path) const;
    void                                UnloadAsset_(eAssetType _type, const fs::path& _path);

    NODISCARD bool IsExistAsset_(eAssetType _type, const fs::path& _path) const;
    NODISCARD Ref<Asset>       CreateAsset_(eAssetType _type) const;
    NODISCARD Container&       GetContainer_(eAssetType _type);
    NODISCARD const Container& GetContainer_(eAssetType _type) const;

    Container m_containers[EnumCount<eAssetType>()];
};

}   // namespace jam