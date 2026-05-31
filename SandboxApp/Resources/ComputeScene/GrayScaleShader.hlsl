#include "../Common/Common.hlsl"

cbuffer CBConstant : register(b0)
{
    uint bGrayScale;
    PADDING3;
}

Texture2D t_inTexture : register(t0);
RWTexture2D<unorm float4> t_outTexture : register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 _dispatchThreadId : SV_DispatchThreadID)
{
    uint w, h;
    t_outTexture.GetDimensions(w, h);

    uint x = _dispatchThreadId.x;
    uint y = _dispatchThreadId.y;

    if (x >= w || y >= h)
    {
        return;
    }

    float3 rgb = t_inTexture.Load(_dispatchThreadId).rgb;
    if (bGrayScale > 0)
    {
        rgb = dot(rgb, float3(0.299, 0.587, 0.114));
    }
    t_outTexture[_dispatchThreadId.xy] = float4(rgb, 1.0);
}