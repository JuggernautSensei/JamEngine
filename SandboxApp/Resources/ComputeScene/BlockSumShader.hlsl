StructuredBuffer<int> b_inBuffer : register(t0);
RWStructuredBuffer<int> b_outBuffer : register(u0);

groupshared int g_sharedBuffer[64];

[numthreads(64, 1, 1)]
void CSMain(
    uint3 id : SV_DispatchThreadID,
    uint3 groupId : SV_GroupID,
    uint3 groupThreadId : SV_GroupThreadID)
{
    g_sharedBuffer[groupThreadId.x] = b_inBuffer[id.x];
    GroupMemoryBarrierWithGroupSync();

    for (uint stride = 64 / 2; stride > 0; stride >>= 1)
    {
        if (groupThreadId.x < stride)
        {
            g_sharedBuffer[groupThreadId.x] += g_sharedBuffer[groupThreadId.x + stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (groupThreadId.x == 0)
    {
        b_outBuffer[groupId.x] = g_sharedBuffer[0];
    }
}