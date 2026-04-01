#pragma once
#include "GraphicsBase.h"
#include "IAsset.h"

namespace Jam
{

class Texture : public IAsset
{
public:
    [[nodiscard]] static Ref<Texture>         Load(TextureHandle _texh);
    [[nodiscard]] static Expect<Ref<Texture>> LoadFromFile(StringView _path);
    ~Texture() override;

    Texture(const Texture& _other)            = delete;
    Texture& operator=(const Texture& _other) = delete;
    Texture(Texture&& _other) noexcept;
    Texture& operator=(Texture&& _other) noexcept;

    Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const override;
    void  SetName(StringView _name) override;
    void  SetVirtualPath(StringView _path) override;
    void  Invalidate() override;

    [[nodiscard]] bool       IsValid() const override;
    [[nodiscard]] eAsset     GetAssetType() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt32     GetTypeHash() const override;
    [[nodiscard]] StringView GetPath() const override;
    [[nodiscard]] StringView GetName() const override;
    [[nodiscard]] StringView GetExtension() const override;
    [[nodiscard]] StringView GetFilter() const override;

    [[nodiscard]] TextureHandle GetHandle() const;
    [[nodiscard]] Vec2i         GetResolution() const;
    [[nodiscard]] eTexture      GetTextureType() const;

    void Bind(eShader _shader, Int32 _slot) const;
    void Unbind(eShader _shader, Int32 _slot) const;

    static constexpr StringView k_typeName  = NameOf<Texture>();
    static constexpr UInt32     k_typeHash  = HashOf<Texture>();
    static constexpr eAsset     k_asestType = eAsset::Texture;
    static constexpr StringView k_extension = ".jtex";
    static constexpr StringView k_filter    = "*.jtex";

private:
    Texture() = default;
    explicit Texture(TextureHandle _texh);
    Error Load_(StringView _path) override;
    void  CleanUp_();

    String        m_path            = {};
    String        m_name            = {};
    TextureHandle m_texh            = k_nullHandle;
    Vec2i         m_res             = { 0, 0 };
    eTexture      m_type            = eTexture::Texture2D;
    bool          m_bLoadedFromFile = false;
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Texture>& _pTexture);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Texture>& _pTexture);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Texture>);

}   // namespace Jam