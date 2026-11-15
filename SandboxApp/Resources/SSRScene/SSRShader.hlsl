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

struct TraceResult
{
    float2 uv;
    int2 pixel;
    float distance;
    bool bHit;
};

TraceResult Trace(float3 _origin, float3 _direction, int2 _sourcePixel)
{
    TraceResult result = (TraceResult)0;

    float rayLength = cb_maxDistance;
    if (_direction.z < -1e-5f)
    {
        rayLength = min(rayLength, (ViewDepth(0.0f) - _origin.z) / _direction.z);
    }
    else if (_direction.z > 1e-5f)
    {
        rayLength = min(rayLength, (ViewDepth(1.0f) - _origin.z) / _direction.z);
    }
    const float4 startH = mul(float4(_origin, 1.0f), cb_proj);
    const float4 endH = mul(float4(_origin + _direction * rayLength, 1.0f), cb_proj);
    const float startK = 1.0f / startH.w;
    const float endK = 1.0f / endH.w;
    const float2 start = float2(startH.x * startK * 0.5f + 0.5f, 0.5f - startH.y * startK * 0.5f) * cb_screenSize;
    const float2 end = float2(endH.x * endK * 0.5f + 0.5f, 0.5f - endH.y * endK * 0.5f) * cb_screenSize;
    const float2 delta = end - start;
    const bool bValid = rayLength > 0.0f && max(abs(delta.x), abs(delta.y)) >= 1e-4f;

    const float startQ = _origin.z * startK;
    const float endQ = (_origin.z + _direction.z * rayLength) * endK;
    int2 pixel = int2(floor(start));
    const int2 step = int2(sign(delta));
    const float2 interval = 1.0f / max(abs(delta), 1e-10f);
    const float2 boundary = float2(pixel) + float2(step.x > 0 ? 1.0f : 0.0f, step.y > 0 ? 1.0f : 0.0f);
    float2 next = abs(boundary - start) * interval;
    next.x = abs(delta.x) > 1e-5f ? next.x : 1e20f;
    next.y = abs(delta.y) > 1e-5f ? next.y : 1e20f;
    float entry = 0.0f;

    for (int i = 0; bValid && i < (int)cb_steps && entry < 1.0f; ++i)
    {
        if (any(pixel < 0) || any(pixel >= int2(cb_screenSize)))
        {
            break;
        }

        const float exit = min(min(next.x, next.y), 1.0f);
        const float z0 = lerp(startQ, endQ, entry) / lerp(startK, endK, entry);
        const float z1 = lerp(startQ, endQ, exit) / lerp(startK, endK, exit);
        const float depth = t_depth.Load(int3(pixel, 0));
        if (any(pixel != _sourcePixel) && depth < 1.0f)
        {
            const float sceneZ = ViewDepth(depth);
            // 같은 픽셀 안의 광선 구간과 표면 두께를 비교
            if (max(z0, z1) >= sceneZ && min(z0, z1) <= sceneZ + cb_thickness)
            {
                const float hitZ = clamp(sceneZ, min(z0, z1), max(z0, z1));
                const float denominator = (endQ - startQ) - hitZ * (endK - startK);
                const float hit = abs(denominator) > 1e-8f ? saturate((hitZ * startK - startQ) / denominator) : (entry + exit) * 0.5f;
                result.uv = (start + delta * hit) / cb_screenSize;
                result.pixel = pixel;
                const float hitDepth = lerp(startQ, endQ, hit) / lerp(startK, endK, hit);
                result.distance = abs(_direction.z) > 1e-5f ? max(0.0f, (hitDepth - _origin.z) / _direction.z) : rayLength * hit;
                result.bHit = true;
                return result;
            }
        }

        const bool bStepX = next.x <= next.y;
        const bool bStepY = next.y <= next.x;
        entry = exit;
        if (bStepX)
        {
            pixel.x += step.x;
            next.x += interval.x;
        }
        if (bStepY)
        {
            pixel.y += step.y;
            next.y += interval.y;
        }
    }
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    const int2 pixel = int2(input.posH.xy);
    if (t_depth.Load(int3(pixel, 0)) >= 1.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    const float4 normalAndReflectivity = t_normal.Load(int3(pixel, 0));
    if (normalAndReflectivity.a <= 0.001f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    const float3 N = normalize(normalAndReflectivity.xyz);
    const float3 P = ViewPosition(pixel);
    const float3 V = normalize(P);
    const float3 R = reflect(V, N);

    const float skyAlpha = cb_bSkyFallback > 0.5f ? 0.5f : 0.0f;

    const TraceResult hit = Trace(P + N * 0.01f, R, pixel);

    if (hit.bHit)
    {
        const float2 uv = hit.uv;
        const float2 edge = abs(uv * 2.0f - 1.0f);
        const float edgeFade = 1.0f - smoothstep(1.0f - cb_edgeFade, 1.0f, max(edge.x, edge.y));
        const float distanceFade = 1.0f - hit.distance / cb_maxDistance;

        const float3 color = t_scene.Load(int3(hit.pixel, 0)).rgb;
        const float confidence = saturate(edgeFade * distanceFade);
        const float skyWeight = skyAlpha * (1.0f - confidence);
        const float alpha = confidence + skyWeight;
        return float4((color * confidence + cb_skyColor * skyWeight) / max(alpha, 1e-4f), alpha);
    }

    return float4(cb_skyColor, skyAlpha);
}
