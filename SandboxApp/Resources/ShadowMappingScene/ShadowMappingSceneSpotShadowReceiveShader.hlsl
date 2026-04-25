#include "ShadowMappingSceneCommon.hlsl"

struct VSInput
{
    float3 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION0;
    float4 posClip : TEXCOORD0; // for gbuffer sampling
    float4 posLightClip : TEXCOORD1; // for shadow map sampling

    //float2 gbufferTexcoord : TEXCOORD0; // for gbuffer sampling
    //float2 shadowTexcoord : TEXCOORD1; // for shadow map sampling
    
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posW = mul(float4(input.posL, 1.f), cb_world).xyz;
    output.posH = mul(float4(output.posW, 1.f), mul(cb_view, cb_proj));

    // gbuffer texture coord
    output.posClip = output.posH;
    output.posLightClip = SPLAT4(0.f);
    if (cb_makeShadow)
    {
        output.posLightClip = mul(float4(output.posW, 1.f), cb_light[0]);
    }

    //output.gbufferTexcoordC = (output.posH.xy / output.posH.w) * 0.5f + 0.5f;
    //output.gbufferTexcoordC.y = 1.f - output.gbufferTexcoordC.y;

    // shadow map texture coord
    //float4 shadowPosH = mul(float4(output.posW, 1.f), cb_light[0]);
    //output.shadowTexcoordC = shadowPosH.xy / shadowPosH.w;
    //output.shadowTexcoordC = output.shadowTexcoordC * 0.5f + 0.5f;
    //output.shadowTexcoordC.y = 1.f - output.shadowTexcoordC.y;

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 posTex = CvtClipToTex(input.posClip);
    float3 baseColor = t_sceneColor.Sample(s_linearWrap, posTex).rgb;
    float3 N = t_sceneNormal.Sample(s_linearWrap, posTex).rgb;
    N = N * 2.f - 1.f; // [0, 1] -> [-1, 1]
    N = normalize(N);

    float dist = distance(cb_lightPos, input.posW);
    float3 L = (cb_lightPos - input.posW) / dist;
    float3 V = normalize(cb_eyePos - input.posW);
    float3 H = normalize(L + V);

    float nDotL = max(dot(N, L), 0.f);
    float nDotH = max(dot(N, H), 0.f);
    float att = 1.f - smoothstep(cb_lightFallOffStart, cb_lightFallOffEnd, dist);

    float3 localIL = kLightColor * (nDotL + pow(nDotH, 16.f)) * att;
    if (cb_makeShadow)
    {
        float3 posLightNdc = input.posLightClip.xyz / input.posLightClip.w;

        // get closest depth from light space
        float2 posLightTex = posLightNdc.xy * 0.5f + 0.5f;
        posLightTex.y = 1.f - posLightTex.y;
        float closestDepth = t_shadowMap.Sample(s_linearWrap, posLightTex).r;
        float currentDepth = posLightNdc.z;
        float bias = max(cb_shadowBias * (1.f - nDotL), cb_shadowBias * 0.1f);
        float shadow = currentDepth + bias > closestDepth ? 0.f : 1.f;
        localIL *= shadow;
    }

    float3 finalColor = baseColor * (kAmbientColor.rgb + localIL);
    return float4(finalColor, 1.f);
}