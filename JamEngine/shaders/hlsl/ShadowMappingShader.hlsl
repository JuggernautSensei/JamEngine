#include "ShaderBase.hlsl"

struct PS_INPUT
{
    float4 posH : SV_POSITION;
};

PS_INPUT VSMain(VS_INPUT_POS_3D _input)
{
    PS_INPUT output;
    float4 posW = mul(cb_world, float4(_input.posL, 1.f));
    output.posH = mul(cb_shaderMappingProjView, posW);
    return output;
}

void PSMain(PS_INPUT input)
{
    // do nothing
}
 