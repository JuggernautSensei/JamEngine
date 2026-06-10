#include "../Common/Common.hlsl"
#define SHADOW_CASCADE_LEVELS 4

#define SHADOW_TYPE_HARD 0
#define SHADOW_TYPE_PCF3 1
#define SHADOW_TYPE_PCF5 2
#define SHADOW_TYPE_PCF7 3
#define SHADOW_TYPE_VSM 4

Texture2D t_sceneNormal : register(t0);
Texture2D t_sceneColor : register(t1);
Texture2D t_sceneDepth : register(t2);
Texture2D t_sceneLight : register(t3);
Texture2D t_baseColor : register(t4);

Texture2D t_shadowMap : register(t5);
Texture2D t_shadowMapColor : register(t6);
Texture2DArray t_shadowMapArray : register(t7);
Texture2DArray t_shadowMapColorArray : register(t8);

cbuffer CBEntity : register(b0)
{
    float4x4 cb_world;
    uint cb_rgba;
    uint cb_bUseTexture;
    PADDING2;
};

cbuffer CBCamera : register(b1)
{
    float4x4 cb_viewProj;
    float4x4 cb_invViewProj;
    float3 cb_eyePos;
    PADDING1;
};

cbuffer CBLight : register(b2)
{
    float3 cb_lightPos;
    float cb_lightFallOffStart;
    float3 cb_lightDir; // use dir light
    float cb_lightFallOffEnd;
    float cb_lightSpotInnerCos;
    float cb_lightSpotOuterCos;
    PADDING2;
};

cbuffer CBShadow : register(b3)
{
    uint cb_shadowType;
    uint cb_bUseComparisonSampler;
    uint cb_bRenderLightRegion;
    float cb_shadowBias;
    float cb_shadowNormalOffset;
    int cb_cascadeLevel;
    PADDING2;
};

const static float3 kLightColor = 1.f;
const static float3 kAmbientColor = 0.1f;
const static float kVSMMinVariance = 1e-4f;
const static float kVSMBleedingReduction = 0.1f;

const static float3 kRegionColor0 = float3(1.f, 1.f, 0.f);
const static float3 kRegionColor1 = float3(0.f, 1.f, 1.f);
const static float3 kRegionColor2 = float3(1.f, 0.f, 1.f);
const static float3 kRegionColor3 = float3(1.f, 0.5f, 0.f);
const static float3 kRegionColor4 = float3(0.5f, 0.f, 1.f);
const static float3 kRegionColor5 = float3(0.5f, 1.f, 0.f);

cbuffer CBShadowCaster : register(b4)
{
    float4x4 cb_lightView;
    float4x4 cb_lightProj;
};

cbuffer CBCascadeShadowReceiver : register(b5)
{
    float4x4 cb_cascadeLightView[SHADOW_CASCADE_LEVELS];
    float4x4 cb_cascadeLightProj[SHADOW_CASCADE_LEVELS];
    float4 cb_cascadeDistances;
};
