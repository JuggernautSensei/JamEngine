#include "SSRCommon.hlsl"
Texture2D<float4> t_reflection : register(t3);
float4 PSMain(PSInput input) : SV_TARGET
{
    const int2 pixel = int2(input.posH.xy);
    if (t_depth.Load(int3(pixel, 0)) >= 1.0f || t_normal.Load(int3(pixel, 0)).a <= 0.001f) return 0.0f;
    const float4 reflection = t_reflection.Load(int3(pixel, 0));
    const float3 normal = normalize(t_normal.Load(int3(pixel, 0)).xyz);
    const float3 position = ViewPosition(pixel);
    float3 colorSum = 0.0f;
    float confidenceSum = 0.0f;
    float weightSum = 0.0f;
    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            const int2 neighbor = pixel + int2(x, y);
            if (any(neighbor < 0) || any(neighbor >= int2(cb_screenSize))) continue;
            const float4 neighborSurface = t_normal.Load(int3(neighbor, 0));
            if (t_depth.Load(int3(neighbor, 0)) >= 1.0f || neighborSurface.a <= 0.001f) continue;
            if (dot(normal, normalize(neighborSurface.xyz)) < 0.95f) continue;
            const float planeError = abs(dot(ViewPosition(neighbor) - position, normal));
            if (planeError > max(0.015f, position.z * 0.001f)) continue;
            const float weight = exp(-0.5f * float(x * x + y * y));
            const float4 sample = t_reflection.Load(int3(neighbor, 0));
            colorSum += sample.rgb * sample.a * weight;
            confidenceSum += sample.a * weight;
            weightSum += weight;
        }
    }
    // Resolve only across the same receiving surface. Unsupported isolated hits
    // fade out; small coverage holes borrow confidence from nearby valid rays.
    const float support = confidenceSum / max(weightSum, 1e-4f);
    const float confidence = max(support, reflection.a * smoothstep(0.18f, 0.45f, support));
    const float3 color = confidenceSum > 1e-4f ? colorSum / confidenceSum : reflection.rgb;
    const float3 direction = reflect(normalize(ViewPosition(pixel)), normal);
    return cb_bSkyFallback > 0.5f ? float4(lerp(Environment(direction), color, confidence), 1.0f) : float4(color, confidence);
}
