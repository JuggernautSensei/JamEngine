#include "../Common/Common.hlsl"

cbuffer CBConstant : register(b0)
{
    uint cb_bApply;
    float cb_threshold;
    PADDING2;
}

Texture2D t_inTexture;
RWTexture2D<float4> t_outTexture;

[numthreads(16, 16, 1)]
void CSMain(uint3 _dispatchThreadId : SV_DispatchThreadID)
{
    uint w, h;
    t_inTexture.GetDimensions(w, h);

    uint x = _dispatchThreadId.x;
    uint y = _dispatchThreadId.y;
    if (x >= w || y >= h)
    {
        return;
    }

    float3 rgb = t_inTexture.Load(_dispatchThreadId).rgb;
    if (cb_bApply)
    {
        
        float radiance = dot(rgb, float3(0.299f, 0.587f, 0.114f));
        if (radiance >= cb_threshold)
        {
            rgb = float3(1.f, 1.f, 1.f);
        }
        else
        {
            rgb = float3(0.f, 0.f, 0.f);
        }
    }

    t_outTexture[_dispatchThreadId.xy] = float4(rgb, 1.f);
}