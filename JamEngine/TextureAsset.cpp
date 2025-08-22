#include "pch.h"

#include "TextureAsset.h"

namespace jam
{

bool TextureAsset::Save(const fs::path& _path) const
{
    if (!m_texture.SaveToFile(_path))
    {
        JAM_ERROR("Failed to save texture to file: {}", _path.string());
        return false;
    }
    return true;
}

bool TextureAsset::Load(MAYBE_UNUSED AssetManager& _assetMgrRef, const fs::path& _path)
{
    if (!m_texture.LoadFromFile(_path))
    {
        JAM_ERROR("Failed to load texture from file: {}", _path.string());
        return false;
    }

    m_texture.AttachSRV();
    m_path = _path;
    return true;
}

void TextureAsset::Unload()
{
    m_texture.Reset();
}

eAssetType TextureAsset::GetType() const
{
    return s_type;
}

}   // namespace jam