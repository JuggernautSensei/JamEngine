#pragma once
#include "File.h"
#include "IAsset.h"
#include "Texture.h"

namespace Jam
{

enum class eTextureChannel
{
    Red   = 0,
    Green = 1,
    Blue  = 2,
    Alpha = 3,
};

struct MaterialParams
{
    Vec3  ambient   = vec3::k_one;
    Vec3  diffuse   = vec3::k_one;
    Vec3  specular  = vec3::k_one;
    float shininess = 32.0f;

    Vec3  albedo    = vec3::k_one;
    float roughness = 1.f;
    float metallic  = 1.f;

    Vec3  emissive          = vec3::k_zero;
    float emissiveIntensity = 1.f;

    bool bAlphaCutout = false;
    bool bDoubleSided = false;
};

enum class eMaterialTextureType
{
    Albedo,
    Ao,
    Light,
    VertexDisplacement,
    Emissive,
    Normal,
    Metallic,
    Roughness,

};

struct MaterialTextures
{
    [[nodiscard]] const Ref<Texture>& Get(
        const eMaterialTextureType _type) const
    {
        return texes[UnderlyingFrom(_type)];
    }

    void Set(
        const eMaterialTextureType _type,
        const Ref<Texture>&        _pTexture)
    {
        texes[UnderlyingFrom(_type)] = _pTexture;
    }

    [[nodiscard]] bool Has(
        const eMaterialTextureType _type) const
    {
        return texes[UnderlyingFrom(_type)] != nullptr;
    }

    Array<Ref<Texture>, CountOf<eMaterialTextureType>()> texes = {};
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
    [[nodiscard]] static Expect<Ref<Material>> Create(const Path& _path);
    [[nodiscard]] static Ref<Material>         Create(const MaterialParams& _params, const MaterialTextures& _texes, const MaterialTextureSettings& _settings);

    Error Load(const Path& _path) override;
    void  Load(const MaterialParams& _params, const MaterialTextures& _texes, const MaterialTextureSettings& _settings);
    Error Save(const Path& _path, bool _bReplaceExtension = true, Flags<eSaveFileFlags> _flags = eSaveFileFlags::None) const override;
    void  SetName(StringView _name) override;
    void  SetVirtualPath(const Path& _path) override;
    void  Invalidate() override;

    void                                         SetParams(const MaterialParams& _params);
    [[nodiscard]] const MaterialParams&          GetParams() const;
    void                                         SetTextures(const MaterialTextures& _texes);
    [[nodiscard]] const MaterialTextures&        GetTextures() const;
    void                                         SetTexture(eMaterialTextureType _type, const Ref<Texture>& _tex);
    [[nodiscard]] Ref<Texture>                   GetTexture(eMaterialTextureType _type) const;
    [[nodiscard]] bool                           HasTexture(eMaterialTextureType _type) const;
    void                                         SetTextureSettings(const MaterialTextureSettings& _settings);
    [[nodiscard]] const MaterialTextureSettings& GetTextureSettings() const;

    [[nodiscard]] bool        IsValid() const override;
    [[nodiscard]] const Path& GetPath() const override;
    [[nodiscard]] eAssetType  GetAssetType() const override;
    [[nodiscard]] StringView  GetTypeName() const override;
    [[nodiscard]] UInt32      GetTypeHash() const override;
    [[nodiscard]] UInt16      GetSortKey() const;
    [[nodiscard]] StringView  GetName() const override;
    [[nodiscard]] StringView  GetExtension() const override;
    [[nodiscard]] StringView  GetFilter() const override;

    static constexpr StringView k_typeName  = NameOf<Material>();
    static constexpr UInt32     k_typeHash  = HashOf<Material>();
    static constexpr eAssetType k_assetType = eAssetType::Material;
    static constexpr StringView k_extension = ".jmat";
    static constexpr StringView k_filter    = "*.jmat";

private:
    Material() = default;
    void CleanUp_();

    String m_name            = {};
    Path   m_path            = {};
    UInt16 m_sortKey         = 0;
    bool   m_bValid          = false;
    bool   m_bLoadedFromFile = false;

    MaterialParams          m_params   = {};
    MaterialTextureSettings m_settings = {};
    MaterialTextures        m_textures = {};
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Material>& _pMat);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Material>& _pMat);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Material>);

}   // namespace Jam