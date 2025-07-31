#include "PBRCommon.hlsli"

PBR_PS_INPUT VSmain(VS_INPUT_VERTEX3 input)
{
    PBR_PS_INPUT output;

    float heightFactor = (cb_materialTextureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_DISPLACEMENT) ?
        ((displacementTexture.SampleLevel(samplerLinearClamp, input.uv0, 0.f).r - 0.5f) * 2.0f * cb_materialDisplacementStrength)
        : 0.0f;

    float3 localPos     = input.positionL + input.normal * heightFactor;
    JAM_MATRIX viewProj = mul(cb_cameraViewMat, cb_cameraProjMat);

    output.posW = mul(float4(localPos, 1.0f), cb_transformWorldMat).xyz;
    output.posH = mul(float4(output.posW, 1.0f), viewProj);

    output.normalW    = normalize(mul(float4(input.normal, 0.0f), cb_transformWorldInvTransposeMat).xyz);
    output.tangentW   = normalize(mul(float4(input.tangentL, 0.0f), cb_transformWorldMat).xyz);
    output.bitangentW = normalize(cross(output.normalW, output.tangentW));

    output.texCoord  = input.uv0;
    output.texCoord2 = input.uv1;

    return output;
}
