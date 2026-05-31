#include "../Common/Common.hlsl"

cbuffer CBConstant : register(b0)
{
    uint cb_bApply;
    uint cb_radius; // max is 512
    PADDING2;
};

Texture2D t_inTexture : register(t0);
RWTexture2D<float4> t_outTexture : register(u0);

static const uint kGroupSize = 64;
static const uint kMaxRadius = 512;
static const uint kSharedSize = kGroupSize + kMaxRadius * 2;

groupshared float4 g_pixels[kSharedSize];

[numthreads(64, 1, 1)]
void CSMain(
    uint3 _dispatchThreadId : SV_DispatchThreadID,
    uint3 _groupThreadId : SV_GroupThreadID,
    uint3 _groupId : SV_GroupID)
{
    uint x = _dispatchThreadId.x;
    uint y = _dispatchThreadId.y;
    uint z = _dispatchThreadId.z;

    if (cb_bApply == 0)
    {
        t_outTexture[_dispatchThreadId.xy] = t_inTexture.Load(_dispatchThreadId);
        return;
    }

    uint w, h;
    t_inTexture.GetDimensions(w, h);

    // 자신
    g_pixels[_groupThreadId.x + cb_radius] = t_inTexture.Load(_dispatchThreadId);

    // 왼쪽
    for (uint i = _groupThreadId.x; i < cb_radius; i += kGroupSize)
    {
        int lx = (int) (_groupId.x * kGroupSize) - (int) cb_radius + (int) i;
        g_pixels[i] = t_inTexture.Load(int3(lx, y, z));
    }

    // 오른쪽 
    for (uint j = _groupThreadId.x; j < cb_radius; j += kGroupSize)
    {
        int rx = (int) (_groupId.x * kGroupSize) + (int) kGroupSize + (int) j;
        g_pixels[kGroupSize + cb_radius + j] = t_inTexture.Load(int3(rx, y, z));
    }

    GroupMemoryBarrierWithGroupSync();

    if (x >= w || y >= h)
    {
        return;
    }

    float4 sample = 0.f;
    int radius = (int) cb_radius;
    for (int k = -radius; k <= radius; ++k)
    {
        uint index = _groupThreadId.x + cb_radius + k;
        sample += g_pixels[index];
    }

    sample /= (float) (cb_radius * 2 + 1);
    t_outTexture[float2(x, y)] = sample;
}
