#pragma once
#include "IAsset.h"
#include "ImageTexture.h"
#include "Magics.h"

namespace jam
{

enum class eTextureChannel
{
    Red   = 0,
    Green = 1,
    Blue  = 2,
    Alpha = 3,
};

class Material : public IAsset
{
public:
    bool Load(const fs::path& _path) override;
    void LoadEmpty(const fs::path& _virtualPath = {});   // load empty material
    void Unload() override;
    bool Save(const fs::path& _path) const override;

    void               SetAmbientColor(Vec3 _color);
    [[nodiscard]] Vec3 GetAmbientColor() const;

    void               SetDiffuseColor(Vec3 _color);
    [[nodiscard]] Vec3 GetDiffuseColor() const;

    void               SetSpecularColor(Vec3 _color);
    [[nodiscard]] Vec3 GetSpecularColor() const;

    void                SetShininess(float _shininess);
    [[nodiscard]] float GetShininess() const;

    void               SetAlbedoColor(Vec3 _color);
    [[nodiscard]] Vec3 GetAlbedoColor() const;

    void                SetRoughness(float _roughness);
    [[nodiscard]] float GetRoughness() const;

    void                SetMetallic(float _metallic);
    [[nodiscard]] float GetMetallic() const;

    void               SetEmissiveColor(Vec3 _color);
    [[nodiscard]] Vec3 GetEmissiveColor() const;

    void                SetEmissiveIntensity(float _intensity);
    [[nodiscard]] float GetEmissiveIntensity() const;

    void                SetEmissiveTextureIntensity(float _intensity);
    [[nodiscard]] float GetEmissiveTextureIntensity() const;

    void               SetAlphaCutout(bool _alphaCutout);
    [[nodiscard]] bool IsAlphaCutout() const;

    void               SetDoubleSide(bool _doubleSide);
    [[nodiscard]] bool IsDoubleSide() const;

    void               SetFlipYNormalTexture(bool _flipY);
    [[nodiscard]] bool IsFlipYNormalTexture() const;

    void                SetLightTextureIntensity(float _intensity);
    [[nodiscard]] float GetLightTextureIntensity() const;

    void                SetVertexDisplacementTextureScale(float _scale);
    [[nodiscard]] float GetVertexDisplacementTextureScale() const;

    void                                   SetAlbedoTexture(const Ref<ImageTexture>& _texture);
    [[nodiscard]] const Ref<ImageTexture>& GetAlbedoTexture() const;

    void                                   SetNormalTexture(const Ref<ImageTexture>& _texture);
    [[nodiscard]] const Ref<ImageTexture>& GetNormalTexture() const;

    void                                   SetMetallicTexture(const Ref<ImageTexture>& _texture);
    void                                   SetMetallicTextureChannel(eTextureChannel _channel);
    [[nodiscard]] const Ref<ImageTexture>& GetMetallicTexture() const;
    [[nodiscard]] eTextureChannel          GetMetallicTextureChannel() const;

    void                                   SetRoughnessTexture(const Ref<ImageTexture>& _texture);
    void                                   SetRoughnessTextureChannel(eTextureChannel _channel);
    [[nodiscard]] const Ref<ImageTexture>& GetRoughnessTexture() const;
    [[nodiscard]] eTextureChannel          GetRoughnessTextureChannel() const;

    void                                   SetAoTexture(const Ref<ImageTexture>& _texture);
    [[nodiscard]] const Ref<ImageTexture>& GetAoTexture() const;

    void                                   SetEmissiveTexture(const Ref<ImageTexture>& _texture);
    [[nodiscard]] const Ref<ImageTexture>& GetEmissiveTexture() const;

    void                                   SetLightTexture(const Ref<ImageTexture>& _texture);
    [[nodiscard]] const Ref<ImageTexture>& GetLightTexture() const;

    void                                   SetVertexDisplacementTexture(const Ref<ImageTexture>& _texture);
    [[nodiscard]] const Ref<ImageTexture>& GetVertexDisplacementTexture() const;

    [[nodiscard]] bool             IsValid() const override;
    [[nodiscard]] const fs::path&  GetPath() const override;
    [[nodiscard]] eAssetType       GetAssetType() const override;
    [[nodiscard]] StringView GetTypeName() const override;
    [[nodiscard]] UInt32           GetTypeHash() const override;
    [[nodiscard]] UInt16           GetSortKey() const;

    static constexpr StringView k_name      = NameOf<Material>();
    static constexpr UInt32           k_hash      = HashOf<Material>();
    static constexpr StringView k_extension = k_jamMaterialExt;
    static constexpr eAssetType       k_type      = eAssetType::Material;

private:
    void CleanUp_();

    fs::path m_path    = {};
    UInt16   m_sortKey = 0;
    bool     m_bValid  = false;

    // phong properties
    Vec3  m_ambientColor  = Vec3::One;
    Vec3  m_diffuseColor  = Vec3::One;
    Vec3  m_specularColor = Vec3::One;
    float m_shininess     = 32.0f;

    // PBR properties
    Vec3  m_albedoColor = Vec3::One;
    float m_roughness   = 1.f;
    float m_metallic    = 1.f;

    // emissive properties
    Vec3  m_emissiveColor     = Vec3::Zero;
    float m_emissiveIntensity = 1.f;

    // misc
    bool  m_bAlphaCutout                   = false;
    bool  m_bDoubleSided                   = false;
    bool  m_bNormalTextureFliped           = false;
    float m_lightTextureIntensity          = 1.f;
    float m_emissiveTextureIntensity       = 1.f;
    float m_vertexDisplacementTextureScale = 1.f;

    // channels
    eTextureChannel m_metallicTextureChannel  = eTextureChannel::Red;
    eTextureChannel m_roughnessTextureChannel = eTextureChannel::Red;

    // textures
    Ref<ImageTexture> m_pAlbedoTexture             = nullptr;
    Ref<ImageTexture> m_pNormalTexture             = nullptr;
    Ref<ImageTexture> m_pMetallicTexture           = nullptr;
    Ref<ImageTexture> m_pRoughnessTexture          = nullptr;
    Ref<ImageTexture> m_pAoTexture                 = nullptr;
    Ref<ImageTexture> m_pEmissiveTexture           = nullptr;
    Ref<ImageTexture> m_pLightTexture              = nullptr;
    Ref<ImageTexture> m_pVertexDisplacementTexture = nullptr;
};

}   // namespace jam