#include "ShaderBase.hlsl"

static const float k_offset[5] = { -2.f, -1.f, 0.f, 1.f, 2.f };

float PSmain(FULLSCREEN_PS_INPUT input) : SV_TARGET
{
    float2 texelSize = float2(cb_texelX, cb_texelY);

    float sum = 0.f;
    for (int i = 0; i < 5; ++i)
    {
        float2 offset = float2(k_offset[i], 0.f) * texelSize;
        sum += t_ssao.Sample(s_linearWrap, input.texCoord0 + offset).r;
    }
    return sum / 5.f;
}