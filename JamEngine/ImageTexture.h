#pragma once
#include "GraphicsBase.h"
#include "IAsset.h"

namespace jam
{

class ImageTexture : public IAsset
{
public:
    ImageTexture() = default;
    ~ImageTexture() override;

    ImageTexture(const ImageTexture& _other)            = delete;
    ImageTexture& operator=(const ImageTexture& _other) = delete;

    ImageTexture(ImageTexture&& _other) noexcept;
    ImageTexture& operator=(ImageTexture&& _other) noexcept;

    bool Load(const fs::path& _path) override;
    void LoadHandle(TextureHandle _texh);

    void Unload() override;
    bool Save(const fs::path& _path, bool _bReplaceExtension, eSaveFileFlags _flags = eSaveFileFlags_None) const override;
    void SetName(StringView _name) override;
    void SetVirtualPath(const fs::path& _path) override;

    [[nodiscard]] bool             IsValid() const override;
    [[nodiscard]] eAssetType       GetAssetType() const override;
    [[nodiscard]] StringView       GetTypeName() const override;
    [[nodiscard]] UInt32           GetTypeHash() const override;
    [[nodiscard]] const fs::path&  GetPath() const override;
    [[nodiscard]] const StringView GetName() const override;

    [[nodiscard]] TextureHandle GetHandle() const;
    [[nodiscard]] Vec2i         GetResolution() const;
    [[nodiscard]] eTextureType  GetTextureType() const;

    void Bind(eShader _shader, Int32 _slot) const;
    void Unbind(eShader _shader, Int32 _slot) const;

    static constexpr StringView k_name = NameOf<ImageTexture>();
    static constexpr UInt32     k_hash = HashOf<ImageTexture>();
    static constexpr eAssetType k_type = eAssetType::ImageTexture;

private:
    void CleanUp_();

    fs::path      m_path            = {};
    std::string   m_name            = {};
    TextureHandle m_texh            = k_invalidHandle;
    bool          m_bLoadedFromFile = false;

    // cache
    Vec2i        m_res  = { 0, 0 };
    eTextureType m_type = eTextureType::Texture2D;
};

}   // namespace jam