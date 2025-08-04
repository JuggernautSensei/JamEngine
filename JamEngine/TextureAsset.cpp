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

bool TextureAsset::Load(const fs::path& _path)
{
    if (!m_texture.LoadFromFile(_path))
    {
        JAM_ERROR("Failed to load texture from file: {}", _path.string());
        return false;
    }

    m_path = _path;
    return true;
}

void TextureAsset::Unload()
{
    m_texture.Reset();
}

bool TextureAsset::IsLoaded() const
{
    return m_texture.Get();
}

eAssetType TextureAsset::GetType() const
{
    return eAssetType::Texture;
}

}   // namespace jam