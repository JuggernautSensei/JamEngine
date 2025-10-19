#include "ShaderBase.hlsl"

static const float g_offset[5] = { -2.f, -1.f, 0.f, 1.f, 2.f };

float PSmain(FULLSCREEN_PS_INPUT input) : SV_TARGET
{
    uint width, height, numberOfLevels;
    t_ssao.GetDimensions(0, width, height, numberOfLevels);
    float2 texelSize = 1.0 / float2(width, height);

    float sum = 0.f;
    for (int i = 0; i < 5; ++i)
    {
        float2 offset = float2(g_offset[i], 0.f) * texelSize;
        sum += t_ssao.Sample(s_linearClamp, input.texCoord0 + offset).r;
    }
    return sum / 5.f;
}