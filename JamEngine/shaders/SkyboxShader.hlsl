#include "ShaderCommon.hlsl"

struct PS_INPUT
{
    float4 positionH : SV_POSITION;
    float3 positionL : POSITION;
};

PS_INPUT VSmain(VS_INPUT_VERTEX3 input)
{
    PS_INPUT output;
    float3 posV = mul(float4(input.position, 0.f), cb_cameraViewMat).xyz;
    output.positionH = mul(float4(posV, 1.f), cb_cameraProjMat);
    output.positionL = input.position;
    return output;
}

float4 PSmain(PS_INPUT input) : SV_TARGET
{
    return skyboxCubemap.Sample(samplerLinearWrap, input.positionL);
}