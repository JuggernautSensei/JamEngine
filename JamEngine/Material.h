#pragma once
#include "EnumArray.h"
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
    Vec3  ambient   = Numbers<Vec3>::k_one;
    Vec3  diffuse   = Numbers<Vec3>::k_one;
    Vec3  specular  = Numbers<Vec3>::k_one;
    float shininess = 32.0f;

    Vec3  albedo    = Numbers<Vec3>::k_one;
    float roughness = 1.f;
    float metallic  = 1.f;

    Vec3  emissive          = Numbers<Vec3>::k_zero;
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
using MaterialTextures = EnumArray<eMaterialTexture, Ref<Texture>>;

struct MaterialTextureSettings
{
    float           emissiveMapIntensity       = 1.f;
    float           vertexDisplacementMapScale = 1.f;
    bool            bNormalMapFlipedY          = false;
    float           lightMapIntensity          = 1.f;
    eTextureChannel metallicMapChannel         = eTextureChannel::Red;
    eTextureChannel roughnessMapChannel        = eTextureChannel::Red;
};

struct MaterialDesc
{
    MaterialParams          params   = {};
    MaterialTextures        textures = {};
    MaterialTextureSettings settings = {};
};

class Material : public IAsset
{
public:
    [[nodiscard]] static Ref<Material>         Load(const MaterialDesc& _desc);
    [[nodiscard]] static Expect<Ref<Material>> LoadFromFile(StringView _path);

    Error Save(StringView _path, bool _bReplaceExtension = true, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite) const override;
    void  SetName(StringView _name) override;
    void  SetVirtualPath(StringView _path) override;
    void  Invalidate() override;

    void                              SetDesc(const MaterialDesc& _desc);
    [[nodiscard]] const MaterialDesc& GetDesc() const;

    [[nodiscard]] bool       IsValid() const override;
    [[nodiscard]] StringView GetPath() const override;
    [[nodiscard]] eAsset GetAssetType() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt32     GetTypeHash() const override;
    [[nodiscard]] UInt16     GetSortKey() const;
    [[nodiscard]] StringView GetName() const override;
    [[nodiscard]] StringView GetExtension() const override;
    [[nodiscard]] StringView GetFilter() const override;

    static constexpr StringView k_typeName  = NameOf<Material>();
    static constexpr UInt32     k_typeHash  = HashOf<Material>();
    static constexpr eAsset k_assetType = eAsset::Material;
    static constexpr StringView k_extension = ".jmat";
    static constexpr StringView k_filter    = "*.jmat";

private:
    Material() = default;
    explicit Material(const MaterialDesc& _desc);
    Error Load_(StringView _path) override;
    void  CleanUp_();

    String m_name            = {};
    String m_path            = {};
    UInt16 m_sortKey         = 0;
    bool   m_bValid          = false;
    bool   m_bLoadedFromFile = false;

    MaterialDesc m_desc = {};
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<Material>& _pMaterial);
Error ReadFromArchive(ArchiveReader& _archive, Ref<Material>& _pMaterial);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<Material>);

}   // namespace Jam