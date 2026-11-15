#include "Fullscreen.hlsl"

Texture2D t_normal : register(t0);
Texture2D<float> t_depth : register(t1);
Texture2D t_scene : register(t2);

cbuffer CBSSR : register(b0)
{
    float4x4 cb_proj;
    float4x4 cb_invProj;
    float2 cb_screenSize;
    float cb_maxDistance;
    float cb_thickness;
    float cb_steps;
    float cb_edgeFade;
    float cb_bSkyFallback;
    float cb_padding1;
    float3 cb_skyColor;
    float cb_padding2;
    float3 cb_upV;
    float cb_padding3;
};

float ViewDepth(float _depth)
{
    return cb_proj._43 / (_depth - cb_proj._33);
}

float3 ViewPosition(int2 _pixel)
{
    const float depth = t_depth.Load(int3(_pixel, 0));
    const float2 uv = (float2(_pixel) + 0.5f) / cb_screenSize;
    const float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
    const float4 posV = mul(float4(ndc, depth, 1.0f), cb_invProj);
    return posV.xyz / posV.w;
}

float3 Environment(float3 _direction)
{
    const float elevation = dot(_direction, cb_upV);
    const float3 ground = cb_skyColor * lerp(0.12f, 0.45f, saturate(elevation + 1.0f));
    const float3 sky = cb_skyColor * (0.7f + 0.6f * sqrt(saturate(elevation)));
    return lerp(ground, sky, smoothstep(-0.08f, 0.15f, elevation));
}
