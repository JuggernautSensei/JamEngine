#include "PBRCommon.hlsli"

float4 PSmain(PBR_PS_INPUT input) : SV_TARGET
{
    float3 albedo = (cb_textureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_ALBEDO)
                  ? albedoTexture.Sample(samplerLinearClamp, input.texCoord).rgb * cb_albedo
                  : cb_albedo;
  
    float ao = (cb_textureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_AO)
             ? aoTexture.Sample(samplerLinearClamp, input.texCoord).r
             : 1.0;
  
    float metallic = (cb_textureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_METALLIC)
                   ? metallicTexture.Sample(samplerLinearClamp, input.texCoord).r * cb_metallic
                   : cb_metallic;
  
    float roughness = (cb_textureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_ROUGHNESS)
                    ? roughnessTexture.Sample(samplerLinearClamp, input.texCoord).r * cb_roughness
                    : cb_roughness;
  
    float3 emission = (cb_textureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_EMISSIVE)
                    ? emissiveTexture.Sample(samplerLinearClamp, input.texCoord).rgb
                    : cb_emissive;
    
    emission *= cb_emissiveFactor;

    // light map
    float3 lightTexColor = (cb_textureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_LIGHT_MAP)
                         ? lightmapTexture.Sample(samplerLinearClamp, input.texCoord2).rgb * cb_materialLightmapStrength
                         : float3(1.f, 1.f, 1.f);

    albedo *= lightTexColor;

    float3 N = GetNormal(input.tangentW, input.bitangentW, input.normalW, input.texCoord);
    float3 E = normalize(cb_eye - input.posW);
    float3 ambientLighting = AmbientLightingByIBL(albedo, N, E, ao, metallic, roughness);

    // todo lighting

    float4 color = float4(ambientLighting + emission, 1.f);
    return color;
}