#include "ShaderBase.hlsl"

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float3 dirV : TEXCOORD0;
};

PS_INPUT VSMain(VS_INPUT_POS_3D input)
{
    PS_INPUT output;

    output.dirV = mul(cb_view, float4(input.posL, 0.f)).xyz;
    output.posH = mul(cb_proj, float4(output.dirV, 1.f));
    output.posH.z = output.posH.w;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float3 dir = normalize(input.dirV);
    return t_skybox.Sample(s_linearClamp, dir);
}
