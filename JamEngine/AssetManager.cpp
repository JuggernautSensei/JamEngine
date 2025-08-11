#include "pch.h"

#include "AssetManager.h"

#include "Application.h"
#include "AssetUtilities.h"
#include "ModelAsset.h"
#include "TextureAsset.h"

namespace
{

using namespace jam;
bool IsCorrectPath(const fs::path& _path)
{
    // correct path = path of assets directory
    const fs::path    assetDir = GetApplication().GetAssetsDirectory();
    const fs::path    rel      = fs::relative(_path, assetDir);
    std::wstring_view relStr   = rel.native();
    return !relStr.empty() && !relStr.starts_with(L"..");
}

}   // namespace

namespace jam
{

void AssetManager::Clear(const eAssetType _type)
{
    JAM_ASSERT(IsValidEnum(_type), "AssetManager::Clear() - Invalid asset type");
    m_containers[EnumToInt(_type)].clear();
}

void AssetManager::ClearAll()
{
    for (Container& container: m_containers)
    {
        container.clear();   // Clear each asset type container
    }
}

std::optional<Ref<Asset>> AssetManager::GetOrLoadAsset_(const eAssetType _type, const fs::path& _path)
{
    Container& container = GetContainer_(_type);
    auto       it        = container.find(_path);

    if (it == container.end())   // no exist
    {
        // load asset
        return LoadAsset_(_type, _path);
    }
    else   // already exist
    {
        return it->second;   // Return the existing asset
    }
}

std::optional<Ref<Asset>> AssetManager::LoadAsset_(const eAssetType _type, const fs::path& _path)
{
    if (!IsCorrectPath(_path))
    {
        JAM_ERROR("AssetManager::LoadAsset_() - Invalid asset path: {}", _path.string());
        return std::nullopt;
    }

    Container& container = GetContainer_(_type);
    Ref<Asset> asset     = nullptr;
    auto       it        = container.find(_path);
    bool       bExists   = (it != container.end());

    if (bExists)   // 이미 존재함 - 덮어쓰기
    {
        asset = it->second;
    }
    else   // 존재하지 않음 - 새로 생성
    {
        asset = CreateAsset_(_type);
    }

    if (!asset->Load(_path))
    {
        JAM_ERROR("AssetManager::LoadAsset_() - Failed to load asset from path: {}", _path.string());
        return std::nullopt;   // Loading failed
    }

    if (bExists)   // 이미 존재함 이벤트 전송
    {
        AssetModifiedEvent event(_type, _path);
        GetApplication().DispatchEvent(event);
    }
    else   // 존재하지 않음 - 새로 생성 이벤트 전송 + 컨테이너에 추가
    {
        container[_path] = asset;

        AssetLoadEvent event(_type, _path);
        GetApplication().DispatchEvent(event);
    }

    return asset;   // Return the loaded or existing asset
}

void AssetManager::UnloadAsset_(const eAssetType _type, const fs::path& _path)
{
    Container& container = GetContainer_(_type);
    auto       it        = container.find(_path);
    bool       bExists   = (it != container.end());

    if (bExists)   // not found
    {
        // 언로드
        const Ref<Asset>& asset = it->second;
        asset->Unload();

        // 제거
        container.erase(it);

        // 이벤트 전송
        AssetUnloadEvent event(_type, _path);
        GetApplication().DispatchEvent(event);
    }
    else   // find asset
    {
        JAM_ERROR("AssetManager::UnloadAsset_() - Asset not found at path: {}", _path.string());
    }
}

std::optional<Ref<Asset>> AssetManager::GetAsset_(const eAssetType _type, const fs::path& _path) const
{
    const Container& container = GetContainer_(_type);
    auto             it        = container.find(_path);
    if (it != container.end())
    {
        return it->second;   // Return the found asset
    }
    return std::nullopt;   // Asset not found
}

Ref<Asset> AssetManager::CreateAsset_(const eAssetType _type) const
{
    switch (_type)
    {
        case eAssetType::Model: return MakeRef<ModelAsset>();
        case eAssetType::Texture: return MakeRef<TextureAsset>();
    }

    JAM_CRASH("Unsupported asset type: {}", EnumToInt(_type));
}

AssetManager::Container& AssetManager::GetContainer_(const eAssetType _type)
{
    return m_containers[EnumToInt(_type)];
}

const AssetManager::Container& AssetManager::GetContainer_(const eAssetType _type) const
{
    return m_containers[EnumToInt(_type)];
}

bool AssetManager::IsExistAsset_(const eAssetType _type, const fs::path& _path) const
{
    const Container& container = GetContainer_(_type);
    return container.contains(_path);
}

std::optional<Ref<Asset>> AssetManager::ReloadAsset_(const eAssetType _type, const fs::path& _path) const
{
    std::optional<Ref<Asset>> assetOrNull = GetAsset_(_type, _path);   // Ensure the asset exists before reloading
    if (assetOrNull)                                                   // If the asset exists, attempt to reload it
    {
        Ref<Asset> asset = *assetOrNull;
        if (!asset->Load(_path))
        {
            JAM_ERROR("AssetManager::ReloadAsset_() - Failed to reload asset from path: {}", _path.string());
            return std::nullopt;   // Reloading failed
        }
    }
    return assetOrNull;   // reload successful or null
}

}   // namespace jam