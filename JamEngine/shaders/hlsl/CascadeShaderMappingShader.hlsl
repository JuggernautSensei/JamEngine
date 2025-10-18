#include "ShaderBase.hlsl"

#ifndef CASCADE_LEVEL
#define CASCADE_LEVEL 3
#endif

struct GS_Input
{
    float4 posW : SV_POSITION;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    uint rtIndex : SV_RenderTargetArrayIndex;
};

GS_Input VSMain(VS_INPUT_POS_3D _input)
{
    GS_Input output;
    output.posW = mul(cb_world, float4(_input.posL, 1.f));
    return output;
}

[maxvertexcount(3 * CASCADE_LEVEL)]
void GSMain(triangle GS_Input _input[3], inout TriangleStream<PS_INPUT> _triStream)
{
    [unroll]
    for (int i = 0; i < CASCADE_LEVEL; i++)
    {
        PS_INPUT output;
        output.rtIndex = i;

        [unroll]
        for (int j = 0; j < 3; ++j)
        {
            output.posH = mul(cb_CSMProjView[i], _input[j].posW);
            _triStream.Append(output);
        }

        _triStream.RestartStrip();
    }
}

void PSMain(PS_INPUT input)
{
    // do nothing
}
 