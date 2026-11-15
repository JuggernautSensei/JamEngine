#include "ParticleCommon.hlsl"
#include "Camera.hlsl"

struct VSInput
{
    float2 pos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 ndc : TEXCOORD0;
};

struct PSOutput
{
    float4 color : SV_TARGET;
    float depth : SV_DEPTH;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.pos, 0.f, 1.f);
    output.ndc = input.pos;
    return output;
}

PSOutput PSMain(PSInput input)
{
    float4 farPoint = mul(float4(input.ndc, 1.f, 1.f), cb_invViewProj);
    const float3 ray = normalize(farPoint.xyz / farPoint.w - cb_eye);
    const float3 horizon = cb_effect == 0 ? float3(0.14f, 0.21f, 0.3f) : float3(0.055f, 0.075f, 0.12f);
    const float3 sky = lerp(horizon, horizon * 0.2f, saturate(ray.y * 1.8f));
    PSOutput output;
    output.color = float4(sky, 1.f);
    output.depth = 1.f;
    const float t = abs(ray.y) > 0.0001f ? -cb_eye.y / ray.y : -1.f;
    if (t > 0.f)
    {
        const float3 worldPos = cb_eye + ray * t;
        const float4 projected = mul(float4(worldPos, 1.f), cb_viewProj);
        const float depth = projected.z / projected.w;
        if (depth >= 0.f && depth < 1.f)
        {
            const float2 gridUv = worldPos.xz * 0.5f;
            const float2 grid = abs(frac(gridUv - 0.5f) - 0.5f) / max(fwidth(gridUv), 0.0001f);
            const float gridLine = 1.f - saturate(min(grid.x, grid.y));
            float3 ground = cb_effect == 0 ? float3(0.21f, 0.28f, 0.36f) : float3(0.075f, 0.09f, 0.13f);
            ground *= 1.f - gridLine * 0.14f;
            output.color.rgb = lerp(sky, ground, exp(-t * 0.035f));
            output.depth = depth;
        }
    }
    return output;
}
