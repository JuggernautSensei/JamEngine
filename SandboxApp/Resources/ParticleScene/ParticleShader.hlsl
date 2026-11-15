#include "ParticleCommon.hlsl"
#include "Camera.hlsl"

StructuredBuffer<Particle> b_particles : register(t0);

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

PSInput VSMain(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    const float2 corners[6] = {
        float2(-1.f, -1.f), float2(-1.f, 1.f), float2(1.f, 1.f),
        float2(-1.f, -1.f), float2(1.f, 1.f), float2(1.f, -1.f)
    };
    const Particle p = b_particles[instanceId];
    const float2 corner = corners[vertexId];
    float3 right = cb_cameraRight;
    float3 up = cb_cameraUp;
    float shade = 1.f;
    float2 scale = p.size * cb_size;
    if (cb_effect == 1)
    {
        const float tilt = p.angle * 1.7f + (p.seed & 255u);
        right = float3(cos(p.angle), 0.f, sin(p.angle));
        up = float3(-sin(p.angle) * sin(tilt), cos(tilt), cos(p.angle) * sin(tilt));
        const float3 normal = cross(right, up);
        shade = 0.45f + 0.55f * abs(dot(normal, normalize(float3(-0.4f, 0.8f, -0.3f))));
        scale.x *= 0.55f;
    }
    const float3 worldPos = p.position + (right * corner.x * scale.x + up * corner.y * scale.y);
    const float distanceFade = exp(-length(p.position - cb_eye) * 0.025f);
    PSInput output;
    output.posH = mul(float4(worldPos, 1.f), cb_viewProj);
    output.uv = corner;
    output.color = float4(p.color.rgb * shade, 1.f);
    if (cb_effect == 0)
        output.color.a = 0.85f * distanceFade * saturate(p.position.y * 3.f);
    else
        output.color.rgb = lerp(float3(0.055f, 0.075f, 0.12f), output.color.rgb, distanceFade);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 color = input.color;
    if (cb_effect == 0)
    {
        const float r2 = dot(input.uv, input.uv);
        clip(1.f - r2);
        color.a *= 1.f - smoothstep(0.1f, 1.f, r2);
    }
    return color;
}
