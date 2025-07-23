#include "ShaderCommon.hlsl"

float4 PSmain(SCREENSPACE_EFFECT_PS_INPUT input) : SV_TARGET
{
    float3 sceneColor = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord).rgb;
    float depth = postProcessInputTexture2.Sample(samplerPointClamp, input.texCoord).r;

    float3 posNDC = float3(input.texCoord * 2.0f - 1.0f, depth);
    posNDC.y = -posNDC.y;
    float3 posWorld = UnProjectionNDCPos(posNDC, cb_cameraViewProjInvMat);

    float camToFragDist = length(posWorld - cb_cameraPosition);

    float dist = max(0.0, camToFragDist - cb_postProcessFogStartDistance);
    float fogDistanceFactor = 1.0 - exp(-cb_postProcessFogDensity * dist);

    float heightDiff = posWorld.y - cb_postProcessFogHeight;
    float fogHeightFactor = exp(-cb_postProcessFogHeightFallOff * max(heightDiff, 0.0));

    float opacity = lerp(0.f, cb_postProcessFogMaxOpacity, cb_postProcessFogOpacity);
    float fogFactor = saturate(fogDistanceFactor * fogHeightFactor * opacity);

    float3 finalColor = lerp(sceneColor, cb_postProcessFogColor, fogFactor);
    return float4(finalColor, 1.0);
}