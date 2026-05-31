#include "../Common/Common.hlsl"

cbuffer CBConstant : register(b0)
{
    int cb_value;
    PADDING3;
};

RWStructuredBuffer<uint> b_outBuffer : register(u0);

[numthreads(64, 1, 1)]
void CSMain(
    uint3 _dispatchThreadId : SV_DispatchThreadID)
{
    b_outBuffer[_dispatchThreadId.x] = _dispatchThreadId.x + cb_value;
}