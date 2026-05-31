StructuredBuffer<uint> b_inBuffer : register(t0);
RWStructuredBuffer<uint> b_maxBuffer : register(u0);

[numthreads(64, 1, 1)]
void CSMain(
    uint3 _dispatchThreadId : SV_DispatchThreadID)
{
    InterlockedMax(b_maxBuffer[0], b_inBuffer[_dispatchThreadId.x]);
}