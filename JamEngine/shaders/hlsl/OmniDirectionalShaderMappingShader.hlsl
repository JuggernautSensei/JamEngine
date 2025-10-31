#include "ShaderBase.hlsl"

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
    output.posW = mul(float4(_input.posL, 1.f), cb_world);
    return output;
}

[maxvertexcount(18 /* 3 * 6 */)]
void GSMain(triangle GS_Input _input[3], inout TriangleStream<PS_INPUT> _triStream)
{
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        PS_INPUT output;
        output.rtIndex = i;

        [unroll]
        for (int j = 0; j < 3; ++j)
        {
            output.posH = mul(_input[j].posW, cb_odsViewProj[i]);
            _triStream.Append(output);
        }
        _triStream.RestartStrip();
    }
}

void PSMain(PS_INPUT input)
{
    // do nothing
}
 