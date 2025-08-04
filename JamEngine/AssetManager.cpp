#include "pch.h"

#include "AssetManager.h"

#include "Application.h"
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

std::optional<std::shared_ptr<ModelAsset>> AssetManager::LoadModel(const fs::path& _path)
{
    std::optional<std::shared_ptr<Asset>> asset = LoadAsset_(eAssetType::Model, _path);
    if (asset)
    {
        std::shared_ptr<ModelAsset> modelAsset = std::static_pointer_cast<ModelAsset>(asset.value());
        return modelAsset;
    }
    else
    {
        JAM_ERROR("AssetManager::LoadModel() - Failed to load model from path: {}", _path.string());
        return std::nullopt;   // Loading failed
    }
}

bool AssetManager::UnloadModel(const fs::path& _path)
{
    return UnloadAsset_(eAssetType::Model, _path);
}

std::optional<std::shared_ptr<ModelAsset>> AssetManager::GetModel(const fs::path& _path) const
{
    std::optional<std::shared_ptr<Asset>> asset = GetAsset_(eAssetType::Model, _path);
    if (asset)
    {
        std::shared_ptr<ModelAsset> modelAsset = std::static_pointer_cast<ModelAsset>(asset.value());
        return modelAsset;
    }
    else
    {
        JAM_ERROR("AssetManager::GetModel() - Model not found at path: {}", _path.string());
        return std::nullopt;   // Asset not found
    }
}

std::optional<std::shared_ptr<TextureAsset>> AssetManager::LoadTexture(const fs::path& _path)
{
    std::optional<std::shared_ptr<Asset>> asset = LoadAsset_(eAssetType::Texture, _path);
    if (asset)
    {
        std::shared_ptr<TextureAsset> textureAsset = std::static_pointer_cast<TextureAsset>(asset.value());
        return textureAsset;
    }
    else
    {
        return std::nullopt;   // Loading failed
    }
}

std::optional<std::shared_ptr<TextureAsset>> AssetManager::GetTexture(const fs::path& _path) const
{
    std::optional<std::shared_ptr<Asset>> asset = GetAsset_(eAssetType::Texture, _path);
    if (asset)
    {
        std::shared_ptr<TextureAsset> textureAsset = std::static_pointer_cast<TextureAsset>(asset.value());
        return textureAsset;
    }
    else
    {
        JAM_ERROR("AssetManager::GetTexture() - Texture not found at path: {}", _path.string());
        return std::nullopt;   // Asset not found
    }
}

bool AssetManager::UnloadTexture(const fs::path& _path)
{
    return UnloadAsset_(eAssetType::Texture, _path);
}

void AssetManager::Clear(const eAssetType _type)
{
    JAM_ASSERT(IsValidEnum(_type), "AssetManager::Clear() - Invalid asset type");
    m_assetMap[EnumToInt(_type)].clear();
}

void AssetManager::ClearAll()
{
    for (Container& container: m_assetMap)
    {
        container.clear();   // Clear each asset type container
    }
}

std::optional<std::shared_ptr<Asset>> AssetManager::LoadAsset_(const eAssetType _type, const fs::path& _path)
{
    if (!IsCorrectPath(_path))
    {
        JAM_ERROR("AssetManager::LoadAsset_() - Invalid asset path: {}", _path.string());
        return std::nullopt;
    }

    Container& assetMap = GetContainer_(_type);
    auto       it       = assetMap.find(_path);

    // 이미 존재함 - 덮어쓰기
    if (it != assetMap.end())
    {
        return it->second;
    }

    // 생성
    std::shared_ptr<Asset> asset = CreateAsset_(_type);
    if (!asset->Load(_path))
    {
        JAM_ERROR("AssetManager::LoadAsset_() - Failed to load asset from path: {}", _path.string());
        return std::nullopt;   // Loading failed
    }

    assetMap[_path] = asset;
    return asset;   // Successfully loaded and added to the map
}

bool AssetManager::UnloadAsset_(const eAssetType _type, const fs::path& _path)
{
    Container& assetMap = GetContainer_(_type);
    auto       it       = assetMap.find(_path);
    if (it != assetMap.end())
    {
        const std::shared_ptr<Asset>& asset = it->second;
        asset->Unload();
        assetMap.erase(it);
        return true;
    }
    return false;
}

std::optional<std::shared_ptr<Asset>> AssetManager::GetAsset_(const eAssetType _type, const fs::path& _path) const
{
    const Container& assetMap = GetContainer_(_type);
    auto             it       = assetMap.find(_path);
    if (it != assetMap.end())
    {
        return it->second;   // Return the found asset
    }
    return std::nullopt;   // Asset not found
}

std::shared_ptr<Asset> AssetManager::CreateAsset_(const eAssetType _type) const
{
    switch (_type)
    {
        case eAssetType::Model: return std::make_shared<ModelAsset>();
        case eAssetType::Texture: return std::make_shared<TextureAsset>();
    }

    JAM_CRASH("Unsupported asset type: {}", EnumToInt(_type));
}

AssetManager::Container& AssetManager::GetContainer_(const eAssetType _type)
{
    return m_assetMap[EnumToInt(_type)];
}

const AssetManager::Container& AssetManager::GetContainer_(const eAssetType _type) const
{
    return m_assetMap[EnumToInt(_type)];
}

}   // namespace jam