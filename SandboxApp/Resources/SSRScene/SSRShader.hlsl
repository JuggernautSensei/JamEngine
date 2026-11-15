#include "SSRCommon.hlsl"

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
            const float3 hitNormal = normalize(t_normal.Load(int3(pixel, 0)).xyz);
            const float facing = dot(hitNormal, _direction);
            if (facing < -0.02f)
            {
                const float sceneZ = ViewDepth(depth);
                const float2 ndc = (float2(pixel) + 0.5f) / cb_screenSize * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
                const float3 centerRay = float3(ndc / float2(cb_proj._11, cb_proj._22), 1.0f);
                const float centerDenominator = abs(dot(hitNormal, centerRay));
                const float footprint = dot(abs(hitNormal.xy), 1.0f / (cb_screenSize * abs(float2(cb_proj._11, cb_proj._22))));
                // Bound the entire tangent plane over this pixel, rather than
                // treating its center depth as a constant-depth box. This retains
                // oblique/curved surfaces without inflating every silhouette.
                const float nearZ = sceneZ * centerDenominator / max(centerDenominator + footprint, 1e-6f);
                const float farZ = sceneZ * centerDenominator / max(centerDenominator - footprint, 1e-6f);
                if (max(z0, z1) >= nearZ && min(z0, z1) <= farZ + cb_thickness)
                {
                    // A depth interval is only a conservative candidate. At a
                    // silhouette it also overlaps rays already behind the visible
                    // surface, producing stretched wedges and reflected backsides.
                    const float3 surface = centerRay * sceneZ;
                    const float distance = dot(surface - _origin, hitNormal) / facing;
                    const float3 position = _origin + _direction * distance;
                    const float4 projected = mul(float4(position, 1.0f), cb_proj);
                    const float2 hitPixel = float2(projected.x, -projected.y) / projected.w * cb_screenSize * 0.5f + cb_screenSize * 0.5f;
                    const float2 withinPixel = hitPixel - float2(pixel);
                    // Intersect the local tangent plane, then require the
                    // intersection to belong to the candidate pixel. Thickness
                    // must not invent geometry beyond a depth discontinuity.
                    if (distance > 0.02f && distance <= rayLength
                        && all(withinPixel >= -0.001f) && all(withinPixel <= 1.001f))
                    {
                        result.uv = hitPixel / cb_screenSize;
                        result.pixel = pixel;
                        result.distance = distance;
                        result.bHit = true;
                        return result;
                    }
                }
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

float4 FilterHit(TraceResult _hit, float3 _position)
{
    const float3 normal = normalize(t_normal.Load(int3(_hit.pixel, 0)).xyz);
    const float2 samplePosition = _hit.uv * cb_screenSize - 0.5f;
    const int2 base = int2(floor(samplePosition));
    const float2 fraction = frac(samplePosition);
    float3 color = 0.0f;
    float coverage = 0.0f;
    [unroll]
    for (int y = 0; y < 2; ++y)
    {
        [unroll]
        for (int x = 0; x < 2; ++x)
        {
            const int2 pixel = base + int2(x, y);
            if (all(pixel >= 0) && all(pixel < int2(cb_screenSize)) && t_depth.Load(int3(pixel, 0)) < 1.0f)
            {
                const float3 neighborNormal = normalize(t_normal.Load(int3(pixel, 0)).xyz);
                const float planeError = abs(dot(ViewPosition(pixel) - _position, normal));
                // Filter only samples belonging to the validated surface.
                // Missing silhouette coverage fades to the environment;
                // foreground/background colors must never bleed into a hit.
                if (dot(neighborNormal, normal) > 0.75f && planeError < max(0.01f, cb_thickness * 0.5f))
                {
                    const float weight = (x == 0 ? 1.0f - fraction.x : fraction.x) * (y == 0 ? 1.0f - fraction.y : fraction.y);
                    color += t_scene.Load(int3(pixel, 0)).rgb * weight;
                    coverage += weight;
                }
            }
        }
    }
    return float4(color / max(coverage, 1e-4f), coverage);
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

    // Rays heading back toward the camera immediately lose the visible depth
    // layer. Treat them as unavailable information, not reflected silhouettes.
    const float directionFade = smoothstep(0.0f, 0.25f, R.z);
    if (directionFade <= 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    const float3 origin = P + N * max(0.01f, P.z * 1e-4f);
    const TraceResult hit = Trace(origin, R, pixel);

    if (hit.bHit)
    {
        const float2 uv = hit.uv;
        const float2 edge = abs(uv * 2.0f - 1.0f);
        const float edgeFade = 1.0f - smoothstep(1.0f - cb_edgeFade, 1.0f, max(edge.x, edge.y));
        const float distanceFade = 1.0f - smoothstep(cb_maxDistance * 0.7f, cb_maxDistance, hit.distance);
        const float3 hitNormal = normalize(t_normal.Load(int3(hit.pixel, 0)).xyz);
        const float facingFade = smoothstep(0.02f, 0.2f, -dot(hitNormal, R));
        const float4 filtered = FilterHit(hit, origin + R * hit.distance);
        const float confidence = saturate(edgeFade * distanceFade * directionFade * facingFade * filtered.a);
        return float4(filtered.rgb, confidence);
    }

    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}
