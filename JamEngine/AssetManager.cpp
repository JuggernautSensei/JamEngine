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

std::optional<Ref<ModelAsset>> AssetManager::LoadModel(const fs::path& _path)
{
    std::optional<Ref<Asset>> asset = LoadAsset_(eAssetType::Model, _path);
    if (asset)
    {
        Ref<ModelAsset> modelAsset = std::static_pointer_cast<ModelAsset>(asset.value());
        return modelAsset;
    }
    else
    {
        JAM_ERROR("AssetManager::LoadModel() - Failed to load model from path: {}", _path.string());
        return std::nullopt;   // Loading failed
    }
}

void AssetManager::UnloadModel(const fs::path& _path)
{
    UnloadAsset_(eAssetType::Model, _path);
}

std::optional<Ref<ModelAsset>> AssetManager::GetModel(const fs::path& _path) const
{
    std::optional<Ref<Asset>> asset = GetAsset_(eAssetType::Model, _path);
    if (asset)
    {
        Ref<ModelAsset> modelAsset = std::static_pointer_cast<ModelAsset>(asset.value());
        return modelAsset;
    }
    else
    {
        JAM_ERROR("AssetManager::GetModel() - Model not found at path: {}", _path.string());
        return std::nullopt;   // Asset not found
    }
}

std::optional<Ref<TextureAsset>> AssetManager::LoadTexture(const fs::path& _path)
{
    std::optional<Ref<Asset>> asset = LoadAsset_(eAssetType::Texture, _path);
    if (asset)
    {
        Ref<TextureAsset> textureAsset = std::static_pointer_cast<TextureAsset>(asset.value());
        return textureAsset;
    }
    else
    {
        return std::nullopt;   // Loading failed
    }
}

std::optional<Ref<TextureAsset>> AssetManager::GetTexture(const fs::path& _path) const
{
    std::optional<Ref<Asset>> asset = GetAsset_(eAssetType::Texture, _path);
    if (asset)
    {
        Ref<TextureAsset> textureAsset = std::static_pointer_cast<TextureAsset>(asset.value());
        return textureAsset;
    }
    else
    {
        JAM_ERROR("AssetManager::GetTexture() - Texture not found at path: {}", _path.string());
        return std::nullopt;   // Asset not found
    }
}

void AssetManager::UnloadTexture(const fs::path& _path)
{
    UnloadAsset_(eAssetType::Texture, _path);
}

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
    auto       it        = container.find(_path);

    // 이미 존재함 - 덮어쓰기
    if (it != container.end())
    {
        return it->second;
    }

    // 생성
    Ref<Asset> asset = CreateAsset_(_type);
    if (asset->Load(_path)) // load succeeded
    {
        container[_path] = asset;  
        return asset;              
    }
    else // load failed
    {
        JAM_ERROR("AssetManager::LoadAsset_() - Failed to load asset from path: {}", _path.string());
        return std::nullopt;   // Loading failed
    }
}

void AssetManager::UnloadAsset_(const eAssetType _type, const fs::path& _path)
{
    Container& container = GetContainer_(_type);
    auto       it        = container.find(_path);

    if (it == container.end()) // not found
    {
        JAM_ERROR("AssetManager::UnloadAsset_() - Asset not found at path: {}", _path.string());
    }
    else   // find asset
    {
        const Ref<Asset>& asset = it->second;
        asset->Unload();
        container.erase(it);
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
        case eAssetType::Model: return CreateRef<ModelAsset>();
        case eAssetType::Texture: return CreateRef<TextureAsset>();
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

}   // namespace jam