RWTexture2D<float4> t_outTexture : register(u0);

[numthreads(8, 8, 1)]
void CSMain(
    uint3 _groupId : SV_GroupID,
    uint3 _threadIdInGroup : SV_GroupThreadID)
{
    float3 color;
    color.r = _groupId.x / 31.f;
    color.g = _groupId.y / 31.f;
    color.b = 0.f;
    t_outTexture[_groupId.xy * 8 + _threadIdInGroup.xy] = float4(color, 1.f);
}