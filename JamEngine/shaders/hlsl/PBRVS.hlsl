#include "ShaderBase.hlsl"

PBR_PS_INPUT_STATIC_MESH VSMain(VS_INPUT_STATIC_MESH _input)
{
    PBR_PS_INPUT_STATIC_MESH output;

    float bumpMapScale = 0.f;
    if (cb_textureBindFlags & JAM_TEXTURE_BIND_FLAGS_BUMP_MAP)
    {
        bumpMapScale = t_bumpMap.SampleLevel(s_linearClamp, _input.texCoord0, 0.f).r;
        bumpMapScale = (bumpMapScale - 0.5f) * 2.0f; // [0.0, 1.0] -> [-1.0, 1.0]
        bumpMapScale *= cb_bumpMapScaling;
    }

    float3 posL = _input.posL + _input.normalL * bumpMapScale;
    float4x4 projView = mul(cb_proj, cb_view);
    output.posW = mul(cb_world, float4(posL, 1.f)).xyz;
    output.posH = mul(projView, float4(output.posW, 1.f));
    output.normalW = normalize(mul(cb_worldInvTrans, float4(_input.normalL, 0.f)).xyz);
    output.tangentW = normalize(mul(cb_world, float4(_input.tangentL, 0.f)).xyz);
    output.texCoord0 = _input.texCoord0;
    output.texCoord1 = _input.texCoord1;
    return output;
}