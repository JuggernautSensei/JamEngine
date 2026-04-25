#include "../ShaderBase.hlsl"
#define SHADOW_CASCADE_LEVELS 3
#define NUM_LIGHT_MATRICES (6 > SHADOW_CASCADE_LEVELS ? 6 : SHADOW_CASCADE_LEVELS)

//#define LIGHT_TYPE_DIRECTIONAL 0
//#define LIGHT_TYPE_SPOT 1
//#define LIGHT_TYPE_POINT 2

Texture2D t_sceneNormal : register(t0);
Texture2D t_sceneColor : register(t1);
Texture2D t_sceneDepth : register(t2);

Texture2D t_shadowMap : register(t0);
Texture2DArray t_shadowMapArray : register(t1);
TextureCube t_shadowMapCube : register(t2);

SamplerState s_linearWrap : register(s0);

cbuffer CBShadowMappingSceneEntity : register(b0)
{
    float4x4 cb_world;
    float4x4 cb_invTransWorld;
};

cbuffer CBShadowMappingSceneCamera : register(b1)
{
    float4x4 cb_view;
    float4x4 cb_proj;
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
    uint cb_makeShadow;
    float cb_shadowBias;
};

const static float4 kLightColor = SPLAT4(1.f);
const static float4 kAmbientColor = float4(0.2f, 0.2f, 0.2f, 1.f);

cbuffer CBShadowCaster : register(b3)
{
    float4x4 cb_light[NUM_LIGHT_MATRICES];
}