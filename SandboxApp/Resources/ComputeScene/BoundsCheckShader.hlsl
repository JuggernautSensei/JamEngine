#include "../Common/Common.hlsl"

cbuffer CBConstant : register(b0)
{
    uint cb_count;
    PADDING3;
};

RWStructuredBuffer<float> b_outBuffer : register(u0);

[numthreads(64, 1, 1)]
void CSMain(uint3 _dispatchThreadId : SV_DispatchThreadID)
{
    if (_dispatchThreadId.x >= cb_count)
    {
        return;
    }

    b_outBuffer[_dispatchThreadId.x] = _dispatchThreadId.x * 2.f;
}