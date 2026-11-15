#include "Fullscreen.hlsl"

Texture2D t_scene : register(t0);
Texture2D t_reflection : register(t1);
Texture2D t_normal : register(t2);

cbuffer CBComposite : register(b0)
{
    float cb_bUseSsr;
    float cb_bReflectionOnly;
    float cb_strength;
    float cb_padding;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    const int3 pixel = int3(int2(input.posH.xy), 0);
    const float3 scene = t_scene.Load(pixel).rgb;
    const float4 reflection = t_reflection.Load(pixel);
    const float reflectivity = t_normal.Load(pixel).a;

    const float amount = cb_bUseSsr > 0.5f ? saturate(reflection.a * reflectivity * cb_strength) : 0.0f;

    if (cb_bReflectionOnly > 0.5f)
    {
        return float4(reflection.rgb * amount, 1.0f);
    }
    return float4(lerp(scene, reflection.rgb, amount), 1.0f);
}
