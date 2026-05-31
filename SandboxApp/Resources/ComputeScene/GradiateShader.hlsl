#include "../Common/Common.hlsl"

cbuffer CBConstant : register(b0)
{
    uint cb_leftTop;
    uint cb_rightTop;
    uint cb_leftBottom;
    uint cb_rightBottom;
};

RWTexture2D<float4> t_outTexture : register(u0);

[numthreads(16, 16, 1)]
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

    float t0 = (float) x / (w - 1);
    float t1 = (float) y / (h - 1);

    float4 fLeftTop = CvtRGBAToLinear(cb_leftTop);
    float4 fRightTop = CvtRGBAToLinear(cb_rightTop);
    float4 fLeftBottom = CvtRGBAToLinear(cb_leftBottom);
    float4 fRightBottom = CvtRGBAToLinear(cb_rightBottom);

    float4 c0 = lerp(fLeftTop, fRightTop, t0);
    float4 c1 = lerp(fLeftBottom, fRightBottom, t0);
    float4 c2 = lerp(c0, c1, t1);
    t_outTexture[_dispatchThreadId.xy] = c2;
}