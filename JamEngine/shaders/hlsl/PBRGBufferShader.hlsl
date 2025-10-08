#include "PBRCommon.hlsli"

struct PS_Output
{
    float4 normalW : SV_TARGET0;
    float4 albedoRoughness : SV_TARGET1;
    float4 metallicAO : SV_TARGET2;
    float4 emission : SV_TARGET3;
};

PS_Output PSmain(PBR_PS_INPUT input)
{
    PS_Output output;

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
 
    float3 lightTexColor = (cb_textureBindFlags & JAM_MATERIAL_TEXTURE_BIND_FLAGS_LIGHT_MAP)
                         ? lightmapTexture.Sample(samplerLinearClamp, input.texCoord2).rgb * cb_materialLightmapStrength
                         : float3(1.f, 1.f, 1.f);
 
    albedo *= lightTexColor;

    output.normalW         = float4(GetNormal(input.tangentW, input.bitangentW, input.normalW, input.texCoord), 1.f);
    output.albedoRoughness = float4(albedo, roughness);
    output.metallicAO      = float4(metallic, ao, 0.f, 1.f);
    output.emission        = float4(emission, cb_emissiveFactor);
    return output;
}