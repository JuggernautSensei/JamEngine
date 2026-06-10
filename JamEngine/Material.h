#pragma once
#include "Archive.h"
#include "EnumArray.h"
#include "IAsset.h"

namespace Jam
{

class Texture;

enum class eTextureChannel
{
    Red   = 0,
    Green = 1,
    Blue  = 2,
    Alpha = 3,
};

struct MaterialParams
{
    Vec3  ambient   = Vec3::kOne;
    Vec3  diffuse   = Vec3::kOne;
    Vec3  specular  = Vec3::kOne;
    float shininess = 32.0f;

    Vec3  albedo    = Vec3::kOne;
    float roughness = 1.f;
    float metallic  = 1.f;

    Vec3  emissive          = Vec3::kZero;
    float emissiveIntensity = 1.f;

    bool bAlphaCutout = false;
    bool bDoubleSided = false;
};

enum class eMaterialTexture
{
    Albedo = 0,
    Normal,
    Metallic,
    Roughness,
    Emissive,
    Light,
    Ao,
    VertexDisplacement,
};

struct MaterialTextureSettings
{
    float           emissiveMapIntensity       = 1.f;
    float           vertexDisplacementMapScale = 1.f;
    bool            bNormalMapFlipedY          = false;
    float           lightMapIntensity          = 1.f;
    eTextureChannel metallicMapChannel         = eTextureChannel::Red;
    eTextureChannel roughnessMapChannel        = eTextureChannel::Red;
};

class Material : public IAsset
{
public:
    [[nodiscard]] static Ref<Material>         Load();
    [[nodiscard]] static Expect<Ref<Material>> LoadFromFile(StringView _path);

    Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const override;
    void  SetName(StringView _name) override;
    void  Invalidate() override;

    void SetParams(const MaterialParams& _params);
    void SetTextures(const EnumArray<eMaterialTexture, Ref<Texture>>& _textures);
    void SetTexture(eMaterialTexture _type, const Ref<Texture>& _texture);
    void SetSettings(const MaterialTextureSettings& _settings);

    [[nodiscard]] const MaterialParams&                            GetParams() const;
    [[nodiscard]] const EnumArray<eMaterialTexture, Ref<Texture>>& GetTextures() const;
    [[nodiscard]] const Ref<Texture>&                              GetTexture(eMaterialTexture _type) const;
    [[nodiscard]] const MaterialTextureSettings&                   GetSettings() const;

    [[nodiscard]] bool       IsValid() const override;
    [[nodiscard]] StringView GetPath() const override;
    [[nodiscard]] eAsset     GetAssetType() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt64     GetTypeHash() const override;
    [[nodiscard]] UInt16     GetSortKey() const;
    [[nodiscard]] StringView GetName() const override;
    [[nodiscard]] StringView GetExtension() const override;
    [[nodiscard]] StringView GetFilter() const override;

    static constexpr StringView kTypeName  = NameOf<Material>();
    static constexpr UInt64     kTypeHash  = HashOf<Material>();
    static constexpr eAsset     kAssetType = eAsset::Material;
    static constexpr StringView kExtension = ".jmat";
    static constexpr StringView kFilter    = "*.jmat";

private:
    Material();
    Error LoadFromFile_(StringView _path) override;
    void  CleanUp_();

    String m_name    = {};
    String m_path    = {};
    UInt16 m_sortKey = 0;
    bool   m_bValid  = false;

    MaterialParams                            m_params   = {};
    EnumArray<eMaterialTexture, Ref<Texture>> m_textures = {};
    MaterialTextureSettings                   m_settings = {};
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Material>& _pMaterial);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Material>& _pMaterial);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Material>);

}   // namespace Jam