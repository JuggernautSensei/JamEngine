#pragma once
#include "GraphicsBase.h"
#include "IAsset.h"
#include "ImageBase.h"
#include "MemView.h"

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

    bool Load(const fs::path& _path) override;                                                            // use normally
    bool LoadFromMemory(ConstMemView _mem, eImageFormat _format, const fs::path& _virtualPath = {});   // use when creating from memory (or non-file source). but you can still set a virtual path for reference

    void Unload() override;
    bool Save(const fs::path& _path) const override;

    [[nodiscard]] bool             IsValid() const override;
    [[nodiscard]] eAssetType       GetAssetType() const override;
    [[nodiscard]] std::string_view GetTypeName() const override;
    [[nodiscard]] UInt32           GetTypeHash() const override;
    [[nodiscard]] const fs::path&  GetPath() const override;
    [[nodiscard]] TextureHandle    GetHandle() const;

    void Bind(eShader _shader, Int32 _slot) const;
    void Unbind(eShader _shader, Int32 _slot) const;

    static constexpr std::string_view k_name = NameOf<ImageTexture>();
    static constexpr UInt32           k_hash = HashOf<ImageTexture>();
    static constexpr eAssetType       k_type = eAssetType::ImageTexture;

private:
    void CleanUp_();

    fs::path      m_path = {};
    TextureHandle m_texh = k_invalidHandle;
};

}   // namespace jam