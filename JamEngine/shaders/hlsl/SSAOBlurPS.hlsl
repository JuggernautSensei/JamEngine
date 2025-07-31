#include "ShaderCommon.hlsl"

static const float g_offset[5] = { -2.f, -1.f, 0.f, 1.f, 2.f };
static const float g_weight[5] = { 0.06136, 0.24477, 0.38774, 0.24477, 0.06136 };
static const float g_sigmaDepth = 0.05f;

float Gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

float PSmain(SCREENSPACE_EFFECT_PS_INPUT input) : SV_TARGET
{
    float centerDepth = depthTexture.Sample(samplerLinearClamp, input.texCoord);

    uint width, height, numberOfLevels;
    ssaoInputTexture.GetDimensions(0, width, height, numberOfLevels);
    float2 texelSize = 1.0 / float2(width, height);

    float sum = 0.0;
    float weightSum = 0.0;

#ifdef SSAO_BLUR_HORIZONTAL

    for (int i = 0; i < 5; ++i)
    {
        float2 offset = float2(g_offset[i], 0.0) * texelSize;
        float2 uv = input.texCoord + offset;

        float ao = ssaoInputTexture.Sample(samplerLinearClamp, uv).r;
        float depth = depthTexture.Sample(samplerLinearClamp, uv);

        float depthWeight = Gaussian(depth - centerDepth, g_sigmaDepth);
        float weight = g_weight[i] * depthWeight;

        sum += ao * weight;
        weightSum += weight;
    }

#elif defined(SSAO_BLUR_VERTICAL)
    for (int i = 0; i < 5; ++i)
    {
        float2 offset = float2(0.0, g_offset[i]) * texelSize;
        float2 uv = input.texCoord + offset;

        float ao = ssaoInputTexture.Sample(samplerLinearClamp, uv).r;
        float depth = depthTexture.Sample(samplerLinearClamp, uv);

        float depthWeight = Gaussian(depth - centerDepth, g_sigmaDepth);
        float weight = g_weight[i] * depthWeight;

        sum += ao * weight;
        weightSum += weight;
    }
#endif

    return sum / weightSum;
}