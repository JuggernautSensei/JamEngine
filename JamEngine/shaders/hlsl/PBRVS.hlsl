#include "ShaderBase.hlsl"

PBR_PS_INPUT_STATIC_MESH VSMain(VS_INPUT_STATIC_MESH _input)
{
    PBR_PS_INPUT_STATIC_MESH output;

    float displacementMapScaling = 0.f;
    if (cb_textureBindFlags & JAM_TEXTURE_BIND_FLAGS_VERTEX_DISPLACEMENT_MAP)
    {
        displacementMapScaling = t_displacementMap.SampleLevel(s_linearClamp, _input.texCoord0, 0.f).r;
        displacementMapScaling = (displacementMapScaling - 0.5f) * 2.0f; // [0.0, 1.0] -> [-1.0, 1.0]
        displacementMapScaling *= cb_displacementMapScaling;
    }

    float3 posL = _input.posL + _input.normalL * displacementMapScaling;
    float4x4 vp = mul(cb_view, cb_proj);
    output.posW = mul(float4(posL, 1.f), cb_world).xyz;
    output.posH = mul(float4(output.posW, 1.f), vp);
    output.normalW = normalize(mul(cb_worldInvTrans, float4(_input.normalL, 0.f)).xyz);
    output.tangentW = normalize(mul(cb_world, float4(_input.tangentL, 0.f)).xyz);
    output.texCoord0 = _input.texCoord0;
    output.texCoord1 = _input.texCoord1;
    return output;
}