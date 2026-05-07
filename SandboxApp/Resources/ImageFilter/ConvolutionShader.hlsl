#include "ImageFilterCommon.hlsl"

float4 PSMain(PSInput _input) : SV_TARGET
{
    uint w, h;
    t_input0.GetDimensions(w, h);
    float texelX = 1.f / w;
    float texelY = 1.f / h;

    float4 color = 0.f;
    int hLen = cb_kernelLen / 2;

    for (int i = -hLen; i <= hLen; ++i)
    {
        float2 offset =
#ifdef HORIZONTAL
        float2(i * texelX, 0.f)
#else
        float2(0.f, i * texelY);
#endif

        float2 texcoord = _input.texcoord + offset;
        float4 sample = t_input0.Sample(s_linearClamp, texcoord);
        color += sample * GetWeight(i + hLen);
    }

    return color;
}