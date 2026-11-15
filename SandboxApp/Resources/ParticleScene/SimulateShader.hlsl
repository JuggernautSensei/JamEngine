#include "ParticleCommon.hlsl"

RWStructuredBuffer<Particle> b_particles : register(u0);

uint Hash(uint value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

float Random(inout uint seed)
{
    seed = Hash(seed + 0x9e3779b9u);
    return (seed >> 8) * (1.f / 16777216.f);
}

Particle Spawn(uint seed, bool initial)
{
    Particle p;
    p.position.x = (Random(seed) * 2.f - 1.f) * cb_extent.x;
    p.position.z = (Random(seed) * 2.f - 1.f) * cb_extent.z;
    p.position.y = initial ? lerp(0.1f, cb_extent.y, Random(seed)) : cb_extent.y;
    p.lifetime = cb_effect == 0 ? lerp(18.f, 28.f, Random(seed)) : lerp(10.f, 16.f, Random(seed));
    p.age = initial ? Random(seed) * p.lifetime * 0.4f : 0.f;
    p.velocity = float3(0.f, -(cb_effect == 0 ? lerp(0.6f, 1.4f, Random(seed)) : lerp(1.4f, 2.4f, Random(seed))), 0.f);
    p.angle = Random(seed) * 6.2831853f;
    p.angularVelocity = (Random(seed) * 2.f - 1.f) * (cb_effect == 0 ? 1.f : 5.f);
    p.size = cb_effect == 0 ? lerp(0.025f, 0.065f, Random(seed)) : lerp(0.055f, 0.11f, Random(seed));
    p.color = float4(1.f, 1.f, 1.f, 1.f);
    if (cb_effect == 1)
    {
        const float3 palette[6] = {
            float3(1.f, 0.16f, 0.3f), float3(1.f, 0.7f, 0.08f),
            float3(0.14f, 0.85f, 0.66f), float3(0.2f, 0.55f, 1.f),
            float3(0.8f, 0.3f, 1.f), float3(1.f, 0.9f, 0.55f)
        };
        p.color.rgb = palette[min((uint)(Random(seed) * 6.f), 5u)];
    }
    p.seed = seed | 1u;
    return p;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= cb_count) return;

    // 새 슬롯은 이전 내용을 읽지 않음
    if (id.x >= cb_previousCount)
    {
        b_particles[id.x] = Spawn(Hash(id.x + cb_seed * 747796405u), true);
        return;
    }
    if (cb_deltaTime <= 0.f) return;

    Particle p = b_particles[id.x];
    const float phase = (p.seed & 65535u) * (6.2831853f / 65536.f);
    const float dt = min(cb_deltaTime, 1.f / 15.f);
    const float targetSpeed = cb_effect == 0 ? 1.1f : 2.5f;
    p.velocity.y += (-targetSpeed * cb_fallSpeed - p.velocity.y) * min(dt * 1.5f, 1.f);
    p.velocity.x = cb_wind * (0.7f + 0.5f * sin(cb_time * 0.7f + phase));
    p.velocity.z = cb_wind * 0.25f * cos(cb_time * 0.5f + phase);
    const float flutter = cb_effect == 0 ? 0.2f : 0.7f;
    p.position += (p.velocity + float3(sin(p.angle) * flutter, 0.f, cos(p.angle) * flutter)) * dt;
    p.position.x -= floor((p.position.x + cb_extent.x) / (2.f * cb_extent.x)) * (2.f * cb_extent.x);
    p.position.z -= floor((p.position.z + cb_extent.z) / (2.f * cb_extent.z)) * (2.f * cb_extent.z);
    p.angle = frac((p.angle + p.angularVelocity * dt) / 6.2831853f) * 6.2831853f;
    p.age += dt;
    if (p.age >= p.lifetime || p.position.y <= 0.f)
        p = Spawn(p.seed, false);
    b_particles[id.x] = p;
}
