#pragma once
#include "Asset.h"
#include "AssetManager.h"
#include "Textures.h"

namespace jam
{

class TextureAsset : public Asset
{
public:
    TextureAsset()           = default;
    ~TextureAsset() override = default;

    TextureAsset(TextureAsset&&) noexcept            = default;
    TextureAsset& operator=(TextureAsset&&) noexcept = default;
    TextureAsset(const TextureAsset&)                = delete;
    TextureAsset& operator=(const TextureAsset&)     = delete;

    bool Load(AssetManager& _assetMgrRef, const fs::path& _path) override;
    bool Save(const fs::path& _path) const override;
    void Unload() override;

    NODISCARD eAssetType       GetType() const override;
    NODISCARD const Texture2D& GetTexture() const { return m_texture; }

    void BindAsShaderResource(const eShader _shader, const UInt32 _slot) const { m_texture.BindAsShaderResource(_shader, _slot); }

    constexpr static eAssetType s_type = eAssetType::Texture;

private:
    Texture2D m_texture;
};

}   // namespace jam