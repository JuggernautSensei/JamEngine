#pragma once
#include "EmbeddedShaderGlue.h"

namespace jam
{

class ImageTexture;

enum class eSkyboxType
{
    SolidColor = 0,
    Cubemap,
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
    None
};

enum class eCascadeLevel
{
    level2,
    level3,
    level4
};

struct Environment
{
    // screen resolution
    float screenWidth = 800.f;
    float screenHeight = 600.f;

    // sun
    Vec3  dirLightDirection = Vec3(0.f, 1.f, 0.f);
    float dirLightIntensity = 1.f;
    Vec3  dirLightColor     = Vec3(1.f, 1.f, 1.f);

    // shadow
    bool          bDirLightShadowEnabled                             = false;
    eCascadeLevel dirLightShadowCascadeLevel                         = eCascadeLevel::level3;
    float         dirLightCascadeShadowRanges[JAM_MAX_CASCADE_LEVEL] = { 100, 300, 600, 1000 };
    float         dirLightShadowBias                                 = 0.005f;
    float         dirShadowCascadeExtendZScale                       = 5.f;

    // ibl
    std::shared_ptr<ImageTexture> irradianceMap          = nullptr;
    std::shared_ptr<ImageTexture> specularMap            = nullptr;
    std::shared_ptr<ImageTexture> brdfLUT                = nullptr;
    float                         irradianceMapIntensity = 1.f;
    float                         specularMapIntensity   = 1.f;

    // skybox
    eSkyboxType skyboxType = eSkyboxType::SolidColor;

    // skybox by cubemap
    std::shared_ptr<ImageTexture> skyboxTexture = nullptr;   // cubemap

    // skybox by solid color
    Vec3 skyboxSolidColor = Vec3(0.1f, 0.1f, 0.1f);   // solid color

    // ssao
    bool  bSSAOEnabled = false;
    float ssaoRadius   = 0.5f;
    float ssaoBias     = 0.025f;

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
};

}   // namespace jam