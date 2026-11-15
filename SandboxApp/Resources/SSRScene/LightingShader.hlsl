#include "Fullscreen.hlsl"

Texture2D t_color : register(t0);
Texture2D t_normal : register(t1);
Texture2D<float> t_depth : register(t2);

cbuffer CBLighting : register(b0)
{
    float3 cb_lightDirV;
    float cb_ambient;
    float3 cb_skyColor;
    float cb_padding;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    const int3 pixel = int3(int2(input.posH.xy), 0);
    if (t_depth.Load(pixel) >= 1.0f)
    {
        return float4(cb_skyColor, 1.0f);
    }

    const float3 albedo = t_color.Load(pixel).rgb;
    const float3 N = normalize(t_normal.Load(pixel).xyz);
    const float diffuse = saturate(dot(N, cb_lightDirV));
    return float4(albedo * (cb_ambient + (1.0f - cb_ambient) * diffuse), 1.0f);
}
