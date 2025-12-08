#include "ShaderBase.hlsl"

struct PS_OUTPUT
{
    float4 albedo : SV_Target0; // RGB: albedo, A: alpha (RGBA8_UNORM)
    float4 material : SV_Target1; // R: metallic, G: roughness, B: ao, A: unused (RGBA8_UNORM)
    float4 normal : SV_Target2; // RGB: normal, A: unused (RGB10A2_UNORM)
    float4 emissive : SV_Target3; // RGB: emissive + gi + ..., A: unused (RG11B10_FLOAT)
};

PS_OUTPUT PSMain(PBR_PS_INPUT_STATIC_MESH _input)
{
    float4 albedo = (cb_materialFlags & JAM_MATERIAL_FLAGS_ALBEDO)
                  ? t_albedo.Sample(s_linearWrap, _input.texCoord0)
                  : float4(cb_albedo, 1.f);

    if (cb_materialFlags & JAM_MATERIAL_FLAGS_ALPHA_CUTOUT)
    {
        clip(albedo.a - k_alphaCutoutThreshold);
    }
    else
    {
        albedo.a = 1.f;
    }

    float ao = (cb_materialFlags & JAM_MATERIAL_FLAGS_AO)
             ? t_ao.Sample(s_linearWrap, _input.texCoord0).r
             : 1.0;
 
    float metallic = (cb_materialFlags & JAM_MATERIAL_FLAGS_METALLIC_R) ? t_metallic.Sample(s_linearWrap, _input.texCoord0).r :
                     (cb_materialFlags & JAM_MATERIAL_FLAGS_METALLIC_G) ? t_metallic.Sample(s_linearWrap, _input.texCoord0).g :
                     (cb_materialFlags & JAM_MATERIAL_FLAGS_METALLIC_B) ? t_metallic.Sample(s_linearWrap, _input.texCoord0).b :
                     cb_metallic;

    float roughness = (cb_materialFlags & JAM_MATERIAL_FLAGS_ROUGHNESS_R) ? t_roughness.Sample(s_linearWrap, _input.texCoord0).r :
                      (cb_materialFlags & JAM_MATERIAL_FLAGS_ROUGHNESS_G) ? t_roughness.Sample(s_linearWrap, _input.texCoord0).g :
                      (cb_materialFlags & JAM_MATERIAL_FLAGS_ROUGHNESS_B) ? t_roughness.Sample(s_linearWrap, _input.texCoord0).b :
                      cb_roughness;

    float3 emissive = (cb_materialFlags & JAM_MATERIAL_FLAGS_EMISSIVE)
                    ? t_emissive.Sample(s_linearWrap, _input.texCoord0).rgb * cb_emissiveTextureIntensity
                    : cb_emissive * cb_emissiveIntensity;
 
    float3 lightTexColor = (cb_materialFlags & JAM_MATERIAL_FLAGS_LIGHT_MAP)
                         ? t_lightmap.Sample(s_linearWrap, _input.texCoord1).rgb * cb_lightTextureIntencity
                         : float3(0.f, 0.f, 0.f);
 
    float3 normalW = CalcNormal(_input.tangentW, _input.normalW, _input.texCoord0);
    float3 unormNormal = normalW * 0.5f + 0.5f;

    PS_OUTPUT output;
    output.albedo = albedo;
    output.material = float4(metallic, roughness, ao, 1.f);
    output.normal = float4(unormNormal, 1.f);
    output.emissive = float4(emissive + lightTexColor, 1.f);
    return output;
}
