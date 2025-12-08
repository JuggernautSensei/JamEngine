#include "ShaderBase.hlsl"

struct LIGHT_VOLUME_PS_INPUT
{
    float3 posW : POSITION;
    float4 posH : SV_POSITION;
};

LIGHT_VOLUME_PS_INPUT VSMain(VS_INPUT_POS_3D _input)
{
    LIGHT_VOLUME_PS_INPUT output;
    output.posW = mul(float4(_input.posL, 1.f), cb_world).xyz;

    float4x4 vp = mul(cb_view, cb_proj);
    output.posH = mul(float4(output.posW, 1.f), vp);
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

    float3 toLight = normalize(cb_lightPos - posW);
    float dist = length(cb_lightPos - posW);
    float att = SmoothAttenuation(dist, cb_lightFallOffStart, cb_lightFallOffEnd);

    // spotlight cone attenuation
    float spot = 1.f;
    if (cb_lightType == JAM_LIGHT_TYPE_SPOT)
    {
        float spotAngleCos = max(dot(-toLight, cb_lightDir), 0.f);
        float cosInnerCone = cos(cb_lightInnerConeHalfRad);
        float cosOuterCone = cos(cb_lightOuterConeHalfRad);
        spot = max(0.f, (spotAngleCos - cosOuterCone) / max((cosInnerCone - cosOuterCone), k_epsilon));
    }
    
    // final radiance
    float3 radiance = cb_lightRadiance * cb_lightIntensity * att * spot;
    
    // compute output color
    float3 albedo = t_gBufferAlbedo.Sample(s_linearWrap, texCoord).rgb;
    float4 material = t_gBufferMaterial.Sample(s_linearWrap, texCoord);
    float metallic = material.r;
    float roughness = material.g;

    float3 toEye = normalize(cb_eye - posW);
    float3 output = DirectBRDF(toLight, toEye, normal, albedo, metallic, roughness) * radiance;
    return float4(output, 1.f);
}
