#pragma once
#include "EmbeddedShaderGlue.h"
#include "ImageTexture.h"
#include "RefCounted.h"

namespace jam
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
    Filmic
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

struct DirectionalLightData
{
    // environment
    bool  bEnabled  = false;
    Vec3  radiance  = Vec3(1.f, 1.f, 1.f);
    float intensity = 1.f;
    Vec3  direction = Vec3(0.f, -1.f, 0.f);

    // shadow
    bool                            bShadowEnabled   = false;
    Array<float, JAM_CASCADE_LEVEL> cascadeDistances = { 100, 200, 400 };
};

struct ImageBasedLightingData
{
    Ref<ImageTexture> pIrradianceTexture         = nullptr;
    Ref<ImageTexture> pSpecularTexture           = nullptr;
    Ref<ImageTexture> pBrdfLutTexture            = nullptr;
    float             irradianceTextureIntensity = 1.f;
    float             specularTextureIntensity   = 1.f;
};

struct SkyboxData
{
    eSkyboxType       type           = eSkyboxType::SolidColor;
    Ref<ImageTexture> pSkyboxTexture = nullptr;                  // cubemap
    Vec3              solidColor     = Vec3(0.2f, 0.2f, 0.2f);   // solid radiance
};

struct SSAOData
{
    bool  bEnabled = false;
    float radius   = 0.5f;
    float bias     = 0.025f;
};

struct FogData
{
    bool  bEnabled          = false;
    Vec3  radiance          = Vec3(0.5f, 0.5f, 0.5f);
    float density           = 0.01f;
    float fallOffStart      = 10.f;
    float fallOffEnd        = 100.f;
    float heightAttenuation = 0.1f;
};

struct ToneMappingData
{
    bool             bEnabled = false;
    eToneMappingType type     = eToneMappingType::Linear;
    float            exposure = 1.f;
    float            gamma    = 2.2f;
};

struct BloomData
{
    static constexpr Int32 k_maxBlurPasses = 10;

    bool  bEnabled      = false;
    float blurRadius    = 0.005f;   // sigma
    float bloomStrength = 0.05f;
    Int32 numBlurPasses = 5;   // downsample level
};

struct FXAAData
{
    bool         bEnabled = false;
    eFxaaQuality quality  = eFxaaQuality::Medium;
};

struct EnvironmentMisc
{
    float shadowBias = 0.005f;
};

class Environment
{
public:
    [[nodiscard]] const DirectionalLightData& GetDirectionalLight() const;
    void                                      SetDirectionalLight(const DirectionalLightData& _dirLight);

    [[nodiscard]] const ImageBasedLightingData& GetIBL() const;
    void                                        SetIBL(const ImageBasedLightingData& _ibl);

    [[nodiscard]] const SkyboxData& GetSkybox() const;
    void                            SetSkybox(const SkyboxData& _skybox);

    [[nodiscard]] const SSAOData& GetSSAO() const;
    void                          SetSSAO(const SSAOData& _ssao);

    [[nodiscard]] const FogData& GetFog() const;
    void                         SetFog(const FogData& _fog);

    [[nodiscard]] const ToneMappingData& GetToneMapping() const;
    void                                 SetToneMapping(const ToneMappingData& _toneMapping);

    [[nodiscard]] const BloomData& GetBloom() const;
    void                           SetBloom(const BloomData& _bloom);

    [[nodiscard]] const FXAAData& GetFXAA() const;
    void                          SetFXAA(const FXAAData& _fxaa);

    [[nodiscard]] const EnvironmentMisc& GetMisc() const;
    void                                 SetMisc(const EnvironmentMisc& _misc);

    [[nodiscard]] bool IsDirty() const;
    void               ClearDirty();

private:
    DirectionalLightData   m_dirLight    = {};
    ImageBasedLightingData m_ibl         = {};
    SkyboxData             m_skybox      = {};
    SSAOData               m_ssao        = {};
    FogData                m_fog         = {};
    ToneMappingData        m_toneMapping = {};
    BloomData              m_bloom       = {};
    FXAAData               m_fxaa        = {};
    EnvironmentMisc        m_misc        = {};
    bool                   m_bDirty      = true;
};

}   // namespace jam