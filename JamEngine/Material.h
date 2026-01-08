#pragma once
#include "IAsset.h"
#include "ImageTexture.h"

namespace jam
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
    // phong
    Vec3  ambient   = Vec3::One;
    Vec3  diffuse   = Vec3::One;
    Vec3  specular  = Vec3::One;
    float shininess = 32.0f;

    // PBR
    Vec3  albedo    = Vec3::One;
    float roughness = 1.f;
    float metallic  = 1.f;

    // emissive
    Vec3  emissive          = Vec3::Zero;
    float emissiveIntensity = 1.f;

    // misc
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

    Min = Albedo,
    Max = Roughness,
};
JAM_ENUM_STATIC_ASSERT(eMaterialTextureType);
JAM_ENABLE_ENUM_ITERATOR(eMaterialTextureType);

struct MaterialTextures
{
    [[nodiscard]] const Ref<ImageTexture>& Get(const eMaterialTextureType _type) const
    {
        return textures[IntFrom(_type)];
    }

    void Set(const eMaterialTextureType _type, const Ref<ImageTexture>& _pTexture)
    {
        textures[IntFrom(_type)] = _pTexture;
    }

    Array<Ref<ImageTexture>, NumEnums<eMaterialTextureType>()> textures = {};
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
    bool Load(const fs::path& _path) override;
    void LoadMemory(const MaterialParams& _params, const MaterialTextures& _textures, const MaterialTextureSettings& _settings);

    void Unload() override;
    bool Save(const fs::path& _path, bool _bReplaceExtension, eSaveFileFlags _flags = eSaveFileFlags_None) const override;
    void SetName(StringView _name) override;
    void SetVirtualPath(const fs::path& _path) override;

    void                                SetParams(const MaterialParams& _params);
    [[nodiscard]] const MaterialParams& GetParams() const;

    void                                  SetTextures(const MaterialTextures& _textures);
    [[nodiscard]] const MaterialTextures& GetTextures() const;

    void                            SetTexture(eMaterialTextureType _type, const Ref<ImageTexture>& _texture);
    [[nodiscard]] Ref<ImageTexture> GetTexture(eMaterialTextureType _type) const;

    void                                         SetTextureSettings(const MaterialTextureSettings& _settings);
    [[nodiscard]] const MaterialTextureSettings& GetTextureSettings() const;

    [[nodiscard]] bool             IsValid() const override;
    [[nodiscard]] const fs::path&  GetPath() const override;
    [[nodiscard]] eAssetType       GetAssetType() const override;
    [[nodiscard]] StringView       GetTypeName() const override;
    [[nodiscard]] UInt32           GetTypeHash() const override;
    [[nodiscard]] UInt16           GetSortKey() const;
    [[nodiscard]] const StringView GetName() const override;

    static constexpr StringView k_name = NameOf<Material>();
    static constexpr UInt32     k_hash = HashOf<Material>();
    static constexpr eAssetType k_type = eAssetType::Material;

private:
    void CleanUp_();

    std::string m_name            = {};
    fs::path    m_path            = {};
    UInt16      m_sortKey         = 0;
    bool        m_bValid          = false;
    bool        m_bLoadedFromFile = false;

    MaterialParams          m_params   = {};
    MaterialTextureSettings m_settings = {};
    MaterialTextures        m_textures = {};
};

}   // namespace jam