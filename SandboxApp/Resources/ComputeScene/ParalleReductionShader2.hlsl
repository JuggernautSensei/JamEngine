RWStructuredBuffer<float> b_groupSums : register(u0);
RWStructuredBuffer<float> b_finalSum : register(u1);

groupshared float g_sum[256];

[numthreads(256, 1, 1)]
void CSMain(
    uint3 _dispatchThreadId : SV_DispatchThreadID,
    uint3 _groupThreadId : SV_GroupThreadID,
    uint3 _groupId : SV_GroupID)
{
    g_sum[_groupThreadId.x] = b_groupSums[_dispatchThreadId.x];
    GroupMemoryBarrierWithGroupSync();

    for (uint stride = 256u / 2u; stride >= 1u; stride /= 2u)
    {
        if (_groupThreadId.x < stride)
        {
            g_sum[_groupThreadId.x] = g_sum[_groupThreadId.x] + g_sum[_groupThreadId.x + stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (_groupThreadId.x == 0)
    {
        b_finalSum[_groupId.x] = g_sum[0];
    }
}