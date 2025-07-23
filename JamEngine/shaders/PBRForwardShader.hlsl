#include "PBRCommon.hlsli"

float4 PSmain(PBR_PS_INPUT input) : SV_TARGET
{
    float3 albedo = (cb_materialTextureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_ALBEDO)
                  ? albedoTexture.Sample(samplerLinearClamp, input.texCoord).rgb * cb_materialAlbedo
                  : cb_materialAlbedo;
  
    float ao = (cb_materialTextureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_AO)
             ? aoTexture.Sample(samplerLinearClamp, input.texCoord).r
             : 1.0;
  
    float metallic = (cb_materialTextureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_METALLIC)
                   ? metallicTexture.Sample(samplerLinearClamp, input.texCoord).r * cb_materialMetallic
                   : cb_materialMetallic;
  
    float roughness = (cb_materialTextureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_ROUGHNESS)
                    ? roughnessTexture.Sample(samplerLinearClamp, input.texCoord).r * cb_materialRoughness
                    : cb_materialRoughness;
  
    float3 emission = (cb_materialTextureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_EMISSIVE)
                    ? emissiveTexture.Sample(samplerLinearClamp, input.texCoord).rgb
                    : cb_materialEmission;

    emission *= cb_materialEmissionStrength;

    // light map
    float3 lightTexColor = (cb_materialTextureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_LIGHT_MAP)
                         ? lightmapTexture.Sample(samplerLinearClamp, input.texCoord2).rgb * cb_materialLightmapStrength
                         : float3(1.f, 1.f, 1.f);

    albedo *= lightTexColor;

    float3 N = GetNormal(input.tangentW, input.bitangentW, input.normalW, input.texCoord);
    float3 E = normalize(cb_cameraPosition - input.posW);
    float3 ambientLighting = AmbientLightingByIBL(albedo, N, E, ao, metallic, roughness);

    // todo lighting

    float4 color = float4(ambientLighting + emission, 1.f);
    return color;
}