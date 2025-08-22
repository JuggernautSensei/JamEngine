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
    Result<Ref<T>> Load(const fs::path& _path)
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        auto [pAsset, bResult] = Load(T::s_type, _path);   // 에셋을 로드
        if (bResult)                                       // 로드 성공
        {
            return std::static_pointer_cast<T>(pAsset);   // 성공적으로 로드된 에셋을 T 타입으로 변환
        }
        else
        {
            JAM_ERROR("AssetManager::Load() - Failed to load asset from item: {}", _path.string());
            return Fail;
        }
    }

    template<typename T>
    NODISCARD Result<Ref<T>> GetOrLoad(const fs::path& _path)
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        auto [pAsset, bResult] = GetOrLoad(T::s_type, _path);   // 에셋을 가져오거나 로드
        if (bResult)                                            // 로드 성공
        {
            return std::static_pointer_cast<T>(pAsset);   // 성공적으로 가져온 에셋을 T 타입으로 변환
        }
        else   // 로드 실패
        {
            JAM_ERROR("AssetManager::GetOrLoad() - Failed to get or load asset from item: {}", _path.string());
            return Fail;
        }
    }

    template<typename T>
    NODISCARD Result<Ref<T>> Get(const fs::path& _path) const
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        auto [pAsset, bResult] = Get(T::s_type, _path);   // 에셋을 가져오기
        if (bResult)                                      // 가져오기 성공
        {
            return std::static_pointer_cast<T>(pAsset);   // 성공적으로 가져온 에셋을 T 타입으로 변환
        }
        else   // 가져오기 실패
        {
            JAM_ERROR("AssetManager::Get() - Failed to get asset from item: {}", _path.string());
            return Fail;   // 실패 반환
        }
    }

    template<typename T>
    bool Unload(const fs::path& _path)
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        return Unload(T::s_type, _path);
    }

    template<typename T>
    NODISCARD bool Contain(const fs::path& _path) const
    {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset.");
        return Contain(T::s_type, _path);
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

    NODISCARD Result<Ref<Asset>> GetOrLoad(eAssetType _type, const fs::path& _path);
    Result<Ref<Asset>>           Load(eAssetType _type, const fs::path& _path);
    NODISCARD Result<Ref<Asset>> Get(eAssetType _type, const fs::path& _path) const;
    bool                         Unload(eAssetType _type, const fs::path& _path);
    NODISCARD bool               Contain(eAssetType _type, const fs::path& _path) const;

    void ClearAll();
    void Clear(eAssetType _type);

private:
    NODISCARD Ref<Asset>       CreateAsset_(eAssetType _type) const;
    NODISCARD Container&       GetContainer_(eAssetType _type);
    NODISCARD const Container& GetContainer_(eAssetType _type) const;

    Container m_containers[EnumCount<eAssetType>()];
};

}   // namespace jam