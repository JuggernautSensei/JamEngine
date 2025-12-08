#include "ShaderBase.hlsl"

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float3 dirV : TEXCOORD0;
};

PS_INPUT VSMain(VS_INPUT_POS_3D input)
{
    PS_INPUT output;
    output.dirV = input.posL;
    float4x4 vp = mul(cb_view, cb_proj);
    output.posH = mul(float4(output.dirV, 0.f), vp).xyww;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    if (cb_envFlags & JAM_ENVIRONMENT_FLAGS_SKYBOX_CUBEMAP)
    {
        float3 dir = normalize(input.dirV);
        return t_skybox.Sample(s_linearWrap, dir);
    }
    else if (cb_envFlags & JAM_ENVIRONMENT_FLAGS_SKYBOX_SOLID_COLOR)
    {
        return float4(cb_skyBoxSolidColor, 1.f);
    }
    else
    {
        return float4(1.f, 1.f, 1.f, 1.f);
    }
}
