#pragma once

#include "EmbeddedShaderGlue.h"
#include "Texture.h"

namespace Jam
{

enum class eSkyboxType
{
    SolidColor = 0,
    CubeMap,
};

enum class eToneMappingType
{
    Linear = 0,
    Reinhard,
    LumaBasedReinhard,
    WhitePreservingReinhard,
    RomBinDaHouse,
    Uncharted2,
    Filmic,
};

enum class eFxaaQuality
{
    VeryLow = 0,
    Low,
    Medium,
    High,
    VeryHigh,
    Ultra,
};

using CascadeDistancesArray = Array<float, JAM_CASCADE_LEVEL>;

class Environment
{
public:
    // directional light
    void                SetDirectionalLightEnabled(bool _bEnabled);
    [[nodiscard]] bool  IsDirectionalLightEnabled() const;
    void                SetDirectionalLightRadiance(Vector3 _radiance);
    [[nodiscard]] Vector3  GetDirectionalLightRadiance() const;
    void                SetDirectionalLightIntensity(float _intensity);
    [[nodiscard]] float GetDirectionalLightIntensity() const;
    void                SetDirectionalLightDirection(Vector3 _direction);
    [[nodiscard]] Vector3  GetDirectionalLightDirection() const;

    // directional light shadow (CSM)
    void                                       SetDirectionalLightShadowEnabled(bool _bEnabled);
    [[nodiscard]] bool                         IsDirectionalLightShadowEnabled() const;
    void                                       SetDirectionalLightShadowBias(float _bias);
    [[nodiscard]] float                        GetDirectionalLightShadowBias() const;
    void                                       SetDirectionalLightCascadeDistances(const CascadeDistancesArray& _distances);
    [[nodiscard]] const CascadeDistancesArray& GetDirectionalLightCascadeDistances() const;

    // image based lighting
    void                              SetIblIrradianceTex(const Ref<Texture>& _pTexture);
    [[nodiscard]] const Ref<Texture>& GetIblIrradianceTex() const;
    void                              SetIblSpecularTex(const Ref<Texture>& _pTexture);
    [[nodiscard]] const Ref<Texture>& GetIblSpecularTex() const;
    void                              SetIblBrdfLutTex(const Ref<Texture>& _pTexture);
    [[nodiscard]] const Ref<Texture>& GetIblBrdfLutTex() const;
    void                              SetIblIrradianceIntensity(float _intensity);
    [[nodiscard]] float               GetIblIrradianceIntensity() const;
    void                              SetIblSpecularIntensity(float _intensity);
    [[nodiscard]] float               GetIblSpecularIntensity() const;

    // skybox
    void                              SetSkyboxType(eSkyboxType _type);
    [[nodiscard]] eSkyboxType         GetSkyboxType() const;
    void                              SetSkyboxTex(const Ref<Texture>& _pTexture);
    [[nodiscard]] const Ref<Texture>& GetSkyboxTex() const;
    void                              SetSkyboxSolidColor(Vector3 _color);
    [[nodiscard]] Vector3                GetSkyboxSolidColor() const;

    // ssao
    void                SetSsaoEnabled(bool _bEnabled);
    [[nodiscard]] bool  IsSsaoEnabled() const;
    void                SetSsaoRadius(float _radius);
    [[nodiscard]] float GetSsaoRadius() const;
    void                SetSsaoBias(float _bias);
    [[nodiscard]] float GetSsaoBias() const;

    // fog
    void                SetFogEnabled(bool _bEnabled);
    [[nodiscard]] bool  IsFogEnabled() const;
    void                SetFogRadiance(Vector3 _radiance);
    [[nodiscard]] Vector3  GetFogRadiance() const;
    void                SetFogDensity(float _density);
    [[nodiscard]] float GetFogDensity() const;
    void                SetFogFallOffStart(float _start);
    [[nodiscard]] float GetFogFallOffStart() const;
    void                SetFogFallOffEnd(float _end);
    [[nodiscard]] float GetFogFallOffEnd() const;
    void                SetFogHeightAttenuation(float _attenuation);
    [[nodiscard]] float GetFogHeightAttenuation() const;

    // tone mapping
    void                           SetToneMappingEnabled(bool _bEnabled);
    [[nodiscard]] bool             IsToneMappingEnabled() const;
    void                           SetToneMappingType(eToneMappingType _type);
    [[nodiscard]] eToneMappingType GetToneMappingType() const;
    void                           SetToneMappingExposure(float _exposure);
    [[nodiscard]] float            GetToneMappingExposure() const;
    void                           SetToneMappingGamma(float _gamma);
    [[nodiscard]] float            GetToneMappingGamma() const;

    // bloom
    void                SetBloomEnabled(bool _bEnabled);
    [[nodiscard]] bool  IsBloomEnabled() const;
    void                SetBloomBlurRadius(float _radius);
    [[nodiscard]] float GetBloomBlurRadius() const;
    void                SetBloomStrength(float _strength);
    [[nodiscard]] float GetBloomStrength() const;
    void                SetBloomNumBlurPasses(Int32 _numPasses);
    [[nodiscard]] Int32 GetBloomNumBlurPasses() const;

    // fxaa
    void                       SetFxaaEnabled(bool _bEnabled);
    [[nodiscard]] bool         IsFxaaEnabled() const;
    void                       SetFxaaQuality(eFxaaQuality _quality);
    [[nodiscard]] eFxaaQuality GetFxaaQuality() const;

    // dirty flag
    [[nodiscard]] bool IsDirty() const;
    void               ClearDirty();

    void  WriteToArchive(ArchiveWriter& _writer) const;
    Error ReadFromArchive(ArchiveReader& _reader);

    static constexpr Int32 k_maxNumbloomPasses = 10;

private:
    // directional light
    bool  m_bDirLightEnabled  = false;
    Vector3  m_dirLightRadiance  = { 1.f, 1.f, 1.f };
    float m_dirLightIntensity = 1.f;
    Vector3  m_dirLightDirection = { 0.f, -1.f, 0.f };

    // directional light shadow (CSM)
    bool                  m_bDirLightShadowEnabled   = false;
    float                 m_dirLightShadowBias       = 0.005f;
    CascadeDistancesArray m_dirLightCascadeDistances = { 100, 200, 400 };

    // image based lighting
    Ref<Texture> m_pIblIrradianceTex         = nullptr;
    Ref<Texture> m_pIblSpecularTex           = nullptr;
    Ref<Texture> m_pIblBrdfLutTex            = nullptr;
    float        m_iblIrradianceTexIntensity = 1.f;
    float        m_iblSpecularTexIntensity   = 1.f;

    // skybox
    eSkyboxType  m_skyboxType       = eSkyboxType::SolidColor;
    Ref<Texture> m_pSkyboxTex       = nullptr;                // cubemap
    Vector3         m_skyboxSolidColor = { 0.2f, 0.2f, 0.2f };   // solid radiance

    // ssao
    bool  m_bSsaoEnabled = false;
    float m_ssaoRadius   = 0.5f;
    float m_ssaoBias     = 0.025f;

    // fog
    bool  m_bFogEnabled          = false;
    Vector3  m_fogRadiance          = { 0.5f, 0.5f, 0.5f };
    float m_fogDensity           = 0.01f;
    float m_fogFallOffStart      = 10.f;
    float m_fogFallOffEnd        = 100.f;
    float m_fogHeightAttenuation = 0.1f;

    // tone mapping
    bool             m_bToneMappingEnabled = false;
    eToneMappingType m_toneMappingType     = eToneMappingType::Linear;
    float            m_toneMappingExposure = 1.f;
    float            m_toneMappingGamma    = 2.2f;

    // bloom
    bool  m_bBloomEnabled      = false;
    float m_bloomBlurRadius    = 0.005f;   // sigma
    float m_bloomStrength      = 0.05f;
    Int32 m_bloomNumBlurPasses = 5;   // downsample level

    // fxaa
    bool         m_bFxaaEnabled = false;
    eFxaaQuality m_fxaaQuality  = eFxaaQuality::Medium;

    // dirty flag
    bool m_bDirty = false;
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Environment);

}   // namespace Jam