#include "../Common/Common.hlsl"

cbuffer Params
{
    int cb_threshold;
    PADDING3;
}

StructuredBuffer<int> b_inBuffer : register(t0);
RWStructuredBuffer<uint> b_counter : register(u0);

[numthreads(64, 1, 1)]
void CSMain(uint3 _id : SV_DispatchThreadID)
{
    int value = b_inBuffer[_id.x];
    if (value > cb_threshold)
    {
        InterlockedAdd(b_counter[0], 1);
    }
}