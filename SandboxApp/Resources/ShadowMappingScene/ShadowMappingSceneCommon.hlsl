#include "../Common/Common.hlsl"
#define SHADOW_CASCADE_LEVELS 3

#define SHADOW_TYPE_HARD 0
#define SHADOW_TYPE_PCF3 1
#define SHADOW_TYPE_PCF5 2
#define SHADOW_TYPE_PCF7 3
#define SHADOW_TYPE_VSM 4
#define SHADOW_TYPE_ESM 5

Texture2D t_sceneNormal : register(t0);
Texture2D t_sceneColor : register(t1);
Texture2D t_sceneDepth : register(t2);
Texture2D t_sceneLight : register(t3);
Texture2D t_baseColor : register(t4);

Texture2D t_shadowMap : register(t5);
Texture2DArray t_shadowMapArray : register(t6);
TextureCube t_shadowMapCube : register(t7);
Texture2D t_shadowMapAdv : register(t8);

cbuffer CBShadowMappingSceneEntity : register(b0)
{
    float4x4 cb_world;
    uint cb_rgba;
    uint cb_bUseTexture;
    PADDING2;
};

cbuffer CBShadowMappingSceneCamera : register(b1)
{
    float4x4 cb_viewProj;
    float4x4 cb_invViewProj;
    float3 cb_eyePos;
    PADDING1;
};

cbuffer CBShadowMappingSceneLight : register(b2)
{
    float3 cb_lightPos;
    float cb_lightFallOffStart;
    float3 cb_lightDir; // use dir light
    float cb_lightFallOffEnd;
    float cb_lightSpotInnerCos;
    float cb_lightSpotOuterCos;
    uint cb_bMakeShadow;
    PADDING1;
};

cbuffer CBShadowMappingSceneShadow : register(b3)
{
    uint cb_shadowType;
    uint cb_bUseComparisonSampler;
    float cb_shadowBias;
    float cb_shadowNormalOffset;
    float cb_esmExponent;
    PADDING3;
};

const static float3 kLightColor = 1.f;
const static float3 kAmbientColor = 0.1f;
const static float kEpsilon = 1e-6f;
const static float kVSMMinVariance = 1e-4f;
const static float kVSMBleedingReduction = 0.1f;

cbuffer CBShadowMappingSceneShadowCast : register(b4)
{
    float4x4 cb_lightView;
    float4x4 cb_lightProj;
};

cbuffer CBShadowMappingSceneOmniShadowCast : register(b5)
{
    float4x4 cb_lightCube[6];
};

cbuffer CBShadowMappingSceneCascadeShadowCast : register(b6)
{
    float4x4 cb_lightCascade[SHADOW_CASCADE_LEVELS];
    float cb_cascadeSplits[SHADOW_CASCADE_LEVELS];
};

float CalcDistanceAttenuation(float _dist, float _fallOffStart, float _fallOffEnd)
{
    return 1.f - smoothstep(_fallOffStart, _fallOffEnd, _dist);
}

float CalcSpotAttenuation(float3 _L, float3 _lightDir, float _spotInnerCos, float _spotOuterCos)
{
    float nDotLSpot = dot(-_L, _lightDir);
    return smoothstep(_spotOuterCos, _spotInnerCos, nDotLSpot);
}

float CalcBlinePhong(float3 _L, float3 _V, float3 _N, float _shininess)
{
    float3 H = normalize(_L + _V);
    float nDotL = max(dot(_N, _L), 0.f);
    float nDotH = max(dot(_N, H), 0.f);
    return nDotL + pow(nDotH, _shininess);
}

