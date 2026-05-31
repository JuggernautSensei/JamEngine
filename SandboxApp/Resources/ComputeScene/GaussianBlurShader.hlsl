#include "../Common/Common.hlsl"

cbuffer CBConstant : register(b0)
{
    uint cb_bApply;
    PADDING3;
    float4 cb_weights[2];
};

Texture2D t_inTexture : register(t0);
RWTexture2D<unorm float4> t_tmpTexture : register(u0);
RWTexture2D<unorm float4> t_outTexture : register(u1);

#ifdef VERTICAL
groupshared float4 g_pixels[20][16]; // [y][x]
#else
groupshared float4 g_pixels[16][20]; // [y][x]
#endif

float GetWeight(uint _index)
{
    return cb_weights[_index / 4u][_index % 4u];
}

[numthreads(16, 16, 1)]
void CSMain(
    uint3 _dispatchThreadId : SV_DispatchThreadID,
    uint3 _groupThreadId : SV_GroupThreadID)
{
    if (cb_bApply == 0)
    {
        t_outTexture[_dispatchThreadId.xy] = t_inTexture.Load(_dispatchThreadId);
        return;
    }

    uint w, _;
    t_inTexture.GetDimensions(w, _);

#ifdef VERTICAL
    g_pixels[_groupThreadId.y + 2][_groupThreadId.x] = t_tmpTexture[uint2(_dispatchThreadId.x, _dispatchThreadId.y)];

    // 위쪽 경계
    if (_groupThreadId.y < 2)
    {
        uint yy = (_dispatchThreadId.y < 2) ? 0 : _dispatchThreadId.y - 2;
        g_pixels[_groupThreadId.y][_groupThreadId.x] = t_tmpTexture[uint2(_dispatchThreadId.x, yy)];
    }

    // 아래쪽 경계
    if (_groupThreadId.y > 13)
    {
        uint yy = (_dispatchThreadId.y > _ - 3) ? _ - 1 : _dispatchThreadId.y + 2;
        g_pixels[_groupThreadId.y + 4][_groupThreadId.x] = t_tmpTexture[uint2(_dispatchThreadId.x, yy)];
    }

    GroupMemoryBarrierWithGroupSync();

    float4 sum = 0.f;
    for (uint i = 0; i < 5; ++i)
    {
        sum += g_pixels[_groupThreadId.y + i][_groupThreadId.x] * GetWeight(i);
    }
    t_outTexture[uint2(_dispatchThreadId.x, _dispatchThreadId.y)] = sum;

#else
    g_pixels[_groupThreadId.y][_groupThreadId.x + 2] = t_inTexture.Load(_dispatchThreadId);

    // 왼쪽 경계
    if (_groupThreadId.x < 2)
    {
        uint xx = (_dispatchThreadId.x < 2) ? 0 : _dispatchThreadId.x - 2;
        g_pixels[_groupThreadId.y][_groupThreadId.x] = t_inTexture.Load(uint3(xx, _dispatchThreadId.y, _dispatchThreadId.z));
    }

    // 오른쪽 경계
    if (_groupThreadId.x > 13)
    {
        uint xx = (_dispatchThreadId.x > w - 3) ? w - 1 : _dispatchThreadId.x + 2;
        g_pixels[_groupThreadId.y][_groupThreadId.x + 4] = t_inTexture.Load(uint3(xx, _dispatchThreadId.y, _dispatchThreadId.z));
    }

    GroupMemoryBarrierWithGroupSync();

    float4 sum = 0.f;
    for (uint i = 0; i < 5; ++i)
    {
        sum += g_pixels[_groupThreadId.y][_groupThreadId.x + i] * GetWeight(i);
    }
    t_tmpTexture[uint2(_dispatchThreadId.x, _dispatchThreadId.y)] = sum;
#endif
}