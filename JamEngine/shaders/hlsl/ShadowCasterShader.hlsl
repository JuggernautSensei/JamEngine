#include "ShaderCommon.hlsl"

struct GS_Input
{
    float4 posW : SV_POSITION;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION;

#if defined(OMNI_SHADOW_MAPPING_CASTER)
    float3 posW : POSITION;
    uint rtIndex : SV_RenderTargetArrayIndex;
#elif defined(CASCADE_SHADOW_MAPPING_CASTER)
    uint rtIndex : SV_RenderTargetArrayIndex;
#endif
};

#ifdef SHADOW_MAPPING_CASTER
PS_INPUT VSmain(VS_INPUT_VERTEX3 input)
{
    PS_INPUT output;
    float4 posW = mul(float4(input.positionL, 1.f), cb_transformWorldMat);

    JAM_MATRIX viewProj = cb_shadowCasterView * cb_shadowCasterProj;
    output.posH = mul(posW, viewProj);
    return output;
}
#else
GS_Input VSmain(VS_INPUT_VERTEX3 input)
{
    GS_Input output;
    output.posW = mul(float4(input.positionL, 1.f), cb_transformWorldMat);
    return output;
}
#endif

#ifdef OMNI_SHADOW_MAPPING_CASTER
[maxvertexcount(18)]
void GSmain(triangle GS_Input input[3],
            inout TriangleStream<PS_INPUT> triStream)
{
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        PS_INPUT output;
        output.rtIndex = i;

        [unroll]
        for (int j = 0; j < 3; ++j)
        { 
            output.posH = mul(input[j].posW, cb_omniDirectionalShadowCasterViewProjMat[i]);
            output.posW = input[j].posW.xyz;
            triStream.Append(output);
        }

        triStream.RestartStrip();
    }
}
#endif

#ifdef CASCADE_SHADOW_MAPPING_CASTER
[maxvertexcount(3 * JAM_CASCADE_SHADOW_SPLIT_LEVEL)]
void GSmain(triangle GS_Input input[3],
            inout TriangleStream<PS_INPUT> triStream)
{
    [unroll]
    for (int i = 0; i < JAM_CASCADE_SHADOW_SPLIT_LEVEL; i++)
    {
        PS_INPUT output;
        output.rtIndex = i;

        JAM_MATRIX viewProj = cb_cascadeShadowCasterViewMat[i] * cb_cascadeShadowCasterProjMat[i];

        [unroll]
        for (int j = 0; j < 3; ++j)
        {
            output.posH = mul(input[j].posW, viewProj);
            triStream.Append(output);
        }

        triStream.RestartStrip();
    }
}
#endif

float PSmain(PS_INPUT input) : SV_DEPTH
{
#ifdef OMNI_SHADOW_MAPPING_CASTER
    return distance(input.posW, cb_omniDirectionalShadowCasterLightPositionW) / cb_omniDirectionalShadowCasterLightRange;
#else
    return input.posH.z;
#endif
}
 