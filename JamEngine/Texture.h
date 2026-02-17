#pragma once
#include "GraphicsBase.h"
#include "IAsset.h"

namespace Jam
{

class Texture : public IAsset
{
public:
    ~Texture() override;

    Texture(const Texture& _other)            = delete;
    Texture& operator=(const Texture& _other) = delete;
    Texture(Texture&& _other) noexcept;
    Texture& operator=(Texture&& _other) noexcept;

    [[nodiscard]] static Expect<Ref<Texture>> Create(const Path& _path);
    [[nodiscard]] static Ref<Texture>         Create(TextureHandle _texh);

    Error Load(const Path& _path) override;
    void  Load(TextureHandle _texh);
    Error Save(const Path& _path, bool _bReplaceExtension = true, Flags<eSaveFileFlags> _flags = eSaveFileFlags::None) const override;
    void  SetName(StringView _name) override;
    void  SetVirtualPath(const Path& _path) override;
    void  Invalidate() override;

    [[nodiscard]] bool        IsValid() const override;
    [[nodiscard]] eAssetType  GetAssetType() const override;
    [[nodiscard]] StringView  GetTypeName() const override;
    [[nodiscard]] UInt32      GetTypeHash() const override;
    [[nodiscard]] const Path& GetPath() const override;
    [[nodiscard]] StringView  GetName() const override;
    [[nodiscard]] StringView  GetExtension() const override;
    [[nodiscard]] StringView  GetFilter() const override;

    [[nodiscard]] TextureHandle GetHandle() const;
    [[nodiscard]] Vec2i         GetResolution() const;
    [[nodiscard]] eTextureType  GetTextureType() const;

    void Bind(eShader _shader, Int32 _slot) const;
    void Unbind(eShader _shader, Int32 _slot) const;

    static constexpr StringView k_typeName  = NameOf<Texture>();
    static constexpr UInt32     k_typeHash  = HashOf<Texture>();
    static constexpr eAssetType k_asestType = eAssetType::Texture;
    static constexpr StringView k_extension = ".jtex";
    static constexpr StringView k_filter    = "*.jtex";

private:
    Texture() = default;
    void CleanUp_();

    Path          m_path            = {};
    String        m_name            = {};
    TextureHandle m_texh            = k_nullHandle;
    Vec2i         m_res             = { 0, 0 };
    eTextureType  m_type            = eTextureType::Texture2D;
    bool          m_bLoadedFromFile = false;
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Texture>& _pTexture);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Texture>& _pTexture);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Texture>);

}   // namespace Jam