#include "PBRCommon.hlsli"

struct LIGHT_VOLUME_PS_INPUT
{
    float3 posW : POSITION;
    float4 posH : SV_POSITION;
};

LIGHT_VOLUME_PS_INPUT VSmain(VS_INPUT_VERTEX3_POSONLY input)
{
    LIGHT_VOLUME_PS_INPUT output;

    if (cb_lightType == JAM_LIGHT_TYPE_DIRECTIONAL)
    {
        output.posW = input.positionL;
        output.posH = float4(input.positionL, 1.0f);
    }
    else
    {
        output.posW = mul(float4(input.positionL, 1.0f), cb_transformWorldMat).xyz;

        JAM_MATRIX viewProj = mul(cb_cameraViewMat, cb_cameraProjMat);
        output.posH = mul(float4(output.posW, 1.0f), viewProj);
    }

    return output;
}

float4 PSmain(LIGHT_VOLUME_PS_INPUT input) : SV_TARGET
{
    float2 posScreen = float2(input.posH.x, input.posH.y) + float2(0.5f, 0.5f);
    float2 texCoord;
    texCoord.x = posScreen.x / cb_globalScreenWidth;
    texCoord.y = posScreen.y / cb_globalScreenHeight;

    float3 posNDC;
    posNDC.x = texCoord.x * 2.f - 1.f;
    posNDC.y = 1.f - texCoord.y * 2.f;
   
    float3 pixelNorW = gBufferNormalTexture.Sample(samplerLinearClamp, texCoord).xyz;
    posNDC.z = depthTexture.Sample(samplerLinearClamp, texCoord).r;

    float4 pixelPosNoDiv = mul(float4(posNDC, 1.f), cb_cameraViewProjInvMat);
    float3 pixelPosW = pixelPosNoDiv.xyz / pixelPosNoDiv.w;

    float3 pixelToLight = normalize(cb_lightPosition - pixelPosW);
    pixelToLight = lerp(pixelToLight, -cb_lightDirection, cb_lightType == JAM_LIGHT_TYPE_DIRECTIONAL);

    float dist = length(cb_lightPosition - pixelPosW);
    float attenuation = ComputeAttenuation(dist, cb_lightFallOffStart, cb_lightFallOffEnd);
    attenuation = lerp(attenuation, 1.f, cb_lightType == JAM_LIGHT_TYPE_DIRECTIONAL);

    // Spotlight cone attenuation
    float spotAngleCos = max(dot(-pixelToLight, cb_lightDirection), 0.f);
    float cosInnerCone = cos(cb_lightInnerConeAngle);
    float cosOuterCone = cos(cb_lightOuterConeAngle);
    float spotFactor = (spotAngleCos - cosOuterCone) / max((cosInnerCone - cosOuterCone), JAM_TOLERANCE);
    spotFactor = max(spotFactor, 0.f);
    spotFactor = lerp(1.f, spotFactor, cb_lightType == JAM_LIGHT_TYPE_SPOT);
   
    // final radiance
    float3 radiance = cb_lightRadiance * cb_lightStrength
                    * attenuation * spotFactor;
    
    // compute output color
    float4 albedoMetallic = gBufferAlbedoRoughnessTexture.Sample(samplerLinearClamp, texCoord);
    float3 albedo = albedoMetallic.rgb;
    float roughness = albedoMetallic.a;
    float metallic = gBufferMetallicAOTexture.Sample(samplerLinearClamp, texCoord).r;
   
    float3 output = DirectBRDF(pixelToLight, pixelToLight, pixelNorW, albedo, metallic, roughness) * radiance;
    return float4(output, 1.f);
}
