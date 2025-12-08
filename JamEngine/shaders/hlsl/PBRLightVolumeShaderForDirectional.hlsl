#include "ShaderBase.hlsl"

struct LIGHT_VOLUME_PS_INPUT
{
    float3 posW : POSITION;
    float4 posH : SV_POSITION;
};

LIGHT_VOLUME_PS_INPUT VSMain(VS_INPUT_POS_3D _input)
{
    LIGHT_VOLUME_PS_INPUT output;
    output.posW = _input.posL;
    output.posH = float4(_input.posL, 1.0f);
    return output;
}

float4 PSMain(LIGHT_VOLUME_PS_INPUT _input) : SV_TARGET
{
    float2 texelSize = float2(cb_texelX, cb_texelY);
    float2 texCoord = _input.posH.xy * texelSize;

    float3 posNdc;
    posNdc.x = texCoord.x * 2.f - 1.f;
    posNdc.y = 1.f - texCoord.y * 2.f;
    posNdc.z = t_depth.Sample(s_pointClamp, texCoord).r;

    float3 normal = t_gBufferNormal.Sample(s_linearWrap, texCoord).xyz * 2.f - 1.f;
    normal = normalize(normal);

    float4 unprojPos = mul(float4(posNdc, 1.f), cb_viewProjInv);
    float3 posW = unprojPos.xyz / unprojPos.w;

    float3 toLight = normalize(-cb_dirLightDirection);

    // final radiance
    float3 radiance = cb_dirLightRadiance * cb_dirLightIntensity;
    
    // compute output color
    float3 albedo = t_gBufferAlbedo.Sample(s_linearWrap, texCoord).rgb;
    float4 material = t_gBufferMaterial.Sample(s_linearWrap, texCoord);
    float metallic = material.r;
    float roughness = material.g;
    
    float3 toEye = normalize(cb_eye - posW);
    float3 output = DirectBRDF(toLight, toEye, normal, albedo, metallic, roughness) * radiance;
    return float4(output, 1.f);
}
