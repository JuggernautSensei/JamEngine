#include "ShadowMappingSceneCommon.hlsl"

struct VSInput
{
    float3 posL : POSITION;
};

struct GSInput
{
    float4 posW : SV_POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    uint rtIndex : SV_RenderTargetLayer;
};

GSInput VSMain(VSInput input)
{
    GSInput output;
    output.posW = mul(float4(input.posL, 1.0f), cb_world);
    return output;
}

[maxvertexcount(3 * SHADOW_CASCADE_LEVELS)]
void GSMain(triangle GSInput input[3], inout TriangleStream<PSInput> triStream)
{
    [unroll]
    for (int i = 0; i < SHADOW_CASCADE_LEVELS; i++)
    {
        PSInput output;

        [unroll]
        for (int j = 0; j < 3; j++)
        {
            output.posH = mul(input[j].posW, cb_light[i]);
            output.rtIndex = i;
            triStream.Append(output);
        }
    }
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(0.f, 0.f, 0.f, 0.f);
}