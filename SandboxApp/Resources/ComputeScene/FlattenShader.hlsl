#include "../Common/Common.hlsl"

cbuffer CBConstant : register(b0)
{
    uint cb_width;
    uint cb_height;
    PADDING2;
}

RWStructuredBuffer<uint2> b_outBuffer : register(u0);

[numthreads(8, 8, 1)]
void CSMain(
    uint3 _dispatchThreadId : SV_DispatchThreadID)
{
    if (_dispatchThreadId.x < cb_width && _dispatchThreadId.y < cb_height)
    {
        int index = _dispatchThreadId.y * cb_width + _dispatchThreadId.x;
        b_outBuffer[index] = uint2(_dispatchThreadId.y, _dispatchThreadId.x);
    }
}