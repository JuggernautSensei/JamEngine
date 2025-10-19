#pragma once

namespace jam
{

class ImageTexture;

enum class eSkyboxType
{
    SolidColor = 0,
    Cubemap,

    // https://pvpmc.sandia.gov/modeling-guide/1-weather-design-inputs/plane-of-array-poa-irradiance/calculating-poa-irradiance/poa-sky-diffuse/perez-sky-diffuse-model/
    // Dynamic_PerezDiffuseModel (todo)
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

enum class eCascadeLevel
{
    level2,
    level3,
    level4
};

constexpr Int32 k_maxShadowCascadeLevel = 4;

enum class eSSAOKernel
{
    Kernel4x4,
    Kernel6x6,
    Kernel8x8,
};

struct Environment
{
    // sun
    Vec3  sunDirection = Vec3(0.f, 1.f, 0.f);
    float sunIntensity = 1.f;
    Vec3  sunColor     = Vec3(1.f, 1.f, 1.f);

    // shadow
    bool          bShadowEnabled                               = false;
    eCascadeLevel shadowCascadeLevel                           = eCascadeLevel::level3;
    float         shadowCascadeRanges[k_maxShadowCascadeLevel] = { 100, 300, 600, 1000 };
    float         shadowBias                                   = 0.005f;
    float         shadowCascadeExtendZScale                    = 5.f;

    // ibl
    std::shared_ptr<ImageTexture> irradianceMap          = nullptr;
    std::shared_ptr<ImageTexture> specularMap            = nullptr;
    std::shared_ptr<ImageTexture> brdfLUT                = nullptr;
    float                         irradianceMapIntensity = 1.f;
    float                         specularMapIntensity   = 1.f;

    // skybox
    eSkyboxType                   skyboxType    = eSkyboxType::SolidColor;
    std::shared_ptr<ImageTexture> skyboxTexture = nullptr;                  // cubemap
    Vec3                          skyboxColor   = Vec3(0.1f, 0.1f, 0.1f);   // solid color

    // ssao
    bool        bSSAOEnabled   = false;
    eSSAOKernel ssaoKernelSize = eSSAOKernel::Kernel8x8;
    float       ssaoRadius     = 0.5f;
    float       ssaoBias       = 0.025f;

    // fog
    Vec3  fogColor             = Vec3(0.5f, 0.5f, 0.5f);
    float fogDensity           = 0.01f;   // exp(-density * distance)
    float fogFallOffStart      = 10.f;
    float fogFallOffEnd        = 100.f;
    float fogHeightAttenuation = 0.1f;   // exp(-heightAttenuation * height)

    // tone mapping
    eToneMappingType toneMappingType = eToneMappingType::Linear;
    float            exposure        = 1.f;
    float            gamma           = 2.2f;

    // bloom
    float bloomBlurRadius    = 4.f;   // sigma
    float bloomCombineFactor = 1.f;

    // fxaa
    eFxaaQuality fxaaQuality = eFxaaQuality::Medium;
};

}   // namespace jam