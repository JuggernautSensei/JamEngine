#include "ShaderBase.hlsl"

struct PS_OUTPUT
{
    float4 albedoAO                : SV_TARGET0; // RGBA unorm8 - Albedo.rgb, AO
    float4 normalRoughnessMetallic : SV_TARGET1; // RGBA float16 - Normal.xy (z reconstruct in shader), Roughness, Metallic
    float4 additionalLight         : SV_TARGET2; // RGBA float16 - misc
};

PS_OUTPUT PSMain(PBR_PS_INPUT_STATIC_MESH _input)
{
    float3 albedo = (cb_textureBindFlags & JAM_TEXTURE_BIND_FLAGS_ALBEDO)
                  ? t_albedo.Sample(s_linearClamp, _input.texCoord0).rgb * cb_albedo
                  : cb_albedo;
 
    float ao = (cb_textureBindFlags & JAM_TEXTURE_BIND_FLAGS_AO)
             ? t_ao.Sample(s_linearClamp, _input.texCoord0).r * cb_aoMapIntencity
             : 1.0;
 
    float metallic = (cb_textureBindFlags & JAM_TEXTURE_BIND_FLAGS_METALLIC)
                   ? t_metallic.Sample(s_linearClamp, _input.texCoord0).r * cb_metallic
                   : cb_metallic;
 
    float roughness = (cb_textureBindFlags & JAM_TEXTURE_BIND_FLAGS_ROUGHNESS)
                    ? t_roughness.Sample(s_linearClamp, _input.texCoord0).r * cb_roughness
                    : cb_roughness;
 
    float3 emission = (cb_textureBindFlags & JAM_TEXTURE_BIND_FLAGS_EMISSIVE)
                    ? t_emissive.Sample(s_linearClamp, _input.texCoord0).rgb * cb_emissiveIntensity
                    : cb_emissive;
 
    float3 lightTexColor = (cb_textureBindFlags & JAM_TEXTURE_BIND_FLAGS_LIGHT_MAP)
                         ? t_lightmap.Sample(s_linearClamp, _input.texCoord1).rgb * cb_lightMapIntencity
                         : float3(0.f, 0.f, 0.f);
 
    PS_OUTPUT output;
    output.albedoAO = float4(albedo, ao);
    float3 normal = CalcNormal(_input.tangentW, _input.normalW, _input.texCoord0);
    output.normalRoughnessMetallic = float4(normal.xy, roughness, metallic);
    output.additionalLight = float4(emission + lightTexColor, 1.f);
    return output;
}
