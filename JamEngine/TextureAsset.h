#pragma once
#include "Asset.h"
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

    bool Save(const fs::path& _path) const override;
    bool Load(const fs::path& _path) override;
    void Unload() override;

    NODISCARD bool       IsLoaded() const override;
    NODISCARD eAssetType GetType() const override;
    void                 BindAsShaderResource(const eShader _shader, const UInt32 _slot) const { m_texture.BindAsShaderResource(_shader, _slot); }

private:
    Texture2D m_texture;
};

}   // namespace jam