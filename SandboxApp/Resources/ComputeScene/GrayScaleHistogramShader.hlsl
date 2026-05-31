#include "../Common/Common.hlsl"

Texture2D t_inTexture : register(t0);
RWStructuredBuffer<uint> b_outBuffer : register(u0);

groupshared uint g_localHistoram[256];

uint CalcGrayScaleU8(float4 _color)
{
    float gray = dot(_color.rgb, float3(0.299f, 0.587f, 0.114f));
    return (uint) (gray * 255.0f);
}

[numthreads(16, 16, 1)]
void CSMain(
    uint3 _dispatchThreadId : SV_DispatchThreadID,
    uint3 _groupThreadId : SV_GroupThreadID)
{
    // easy implement 
    // float4 rgba = t_inTexture.Load(_dispatchThreadId);
    // uint grayU8 = CalcGrayScaleU8(rgba);
    // InterlockedAdd(b_outBuffer[grayU8], 1);
    uint index = _groupThreadId.x + _groupThreadId.y * 16u;

    // 초기화
    g_localHistoram[index] = 0;
    GroupMemoryBarrierWithGroupSync();

    // 로컬 히스토그램 계산
    uint grayU8 = CalcGrayScaleU8(t_inTexture.Load(_dispatchThreadId));
    InterlockedAdd(g_localHistoram[grayU8], 1);
    GroupMemoryBarrierWithGroupSync();

    // 글로벌 히스토그램에 로컬 히스토그램 더하기
    InterlockedAdd(b_outBuffer[index], g_localHistoram[index]);
}
