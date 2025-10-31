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
    // 인풋 텍스처 중 하나를 택해 좌표계 산출
    int width, height, dummy;
    t_gBufferAlbedoAO.GetDimensions(0, width, height, dummy);

    float2 texel = float2(1.f / width, 1.f / height);
    float2 texCoord = _input.posH.xy * texel;

    float3 posNdc;
    posNdc.x = texCoord.x * 2.f - 1.f;
    posNdc.y = 1.f - texCoord.y * 2.f;
    posNdc.z = t_depth.Sample(s_pointClamp, texCoord).r;

    float4 normalRoughness = t_gBufferNormalRoughness.Sample(s_linearClamp, texCoord);
    float3 normalW = normalRoughness.xyz;

    float4 unprojPos = mul(cb_viewProjInv, float4(posNdc, 1.f));
    float3 posW = unprojPos.xyz / unprojPos.w;

    float3 toLight = normalize(-cb_dirLightDirection);

    // final radiance
    float3 radiance = cb_dirLightColor * cb_dirLightIntensity;
    
    // compute output color
    float3 albedo = t_gBufferAlbedoAO.Sample(s_linearClamp, texCoord).rgb;
    float4 emissiveMetallic = t_gBufferEmissiveMetallic.Sample(s_linearClamp, texCoord);
    float roughness = normalRoughness.w;
    float metallic = emissiveMetallic.w;
    
    float3 toEye = normalize(cb_eyePos - posW);
    float3 output = DirectBRDF(toLight, toEye, normalW, albedo, metallic, roughness) * radiance;
    return float4(output, 1.f);
}
