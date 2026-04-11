#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define PADDING1 float CONCAT(padding1, __LINE__)

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

cbuffer CBPBRSceneEntity : register(b0)
{
    float4x4 cb_world;
    float4x4 cb_worldInvTrans;
};

cbuffer CBPBRSceneCamera : register(b1)
{
    float3 cb_eyePos;
    PADDING1;
    float4x4 cb_viewProj;
};

struct Light
{
    float3 lightPos;
    PADDING1;
    float3 lightDir;
    float fallOffStart;
    float3 lightColor;
    float fallOffEnd;
    float innerConeCos;
    float outerConeCos;
    uint bEnabled;
    uint type;
};

cbuffer CBPBRSceneLight : register(b2)
{
    Light cb_light[4];
};

struct VSInput
{
    float3 pos : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
};

SamplerState s_linearWrap : register(s0);
SamplerState s_linearClamp : register(s1);

TextureCube t_iblIrradiance : register(t0);
TextureCube t_iblSpecular : register(t1);
Texture2D t_brdfLUT : register(t2);

Texture2D t_albedo : register(t3);
Texture2D t_aoRoughnessMetallic : register(t4);
Texture2D t_emissive : register(t5);
Texture2D t_normal : register(t6);

static const float k_pi = 3.14159265359f;
static const float3 k_f0 = float3(0.04f, 0.04f, 0.04f);
static const float k_maxReflectionLod = 4.f;

float DistributionGGX(float NdotH, float roughness) // D
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = k_pi * denom * denom;

    return nom / denom;
}

float SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    return nom / denom;
}

float GeometrySmith(float NdotV, float NdotL, float roughness) // G
{
    float ggx1 = SchlickGGX(NdotL, roughness);
    float ggx2 = SchlickGGX(NdotV, roughness);
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0) // F
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float r = 1.0f - roughness;
    return F0 + (max(float3(r, r, r), F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.pos, 1.0f), cb_world);
    output.posH = mul(posW, cb_viewProj);
    output.posW = posW.xyz;
    output.texcoord = input.texcoord;
    output.normalW = normalize(mul(float4(input.normal, 0.0f), cb_worldInvTrans).xyz);
    output.tangentW = normalize(mul(float4(input.tangent, 0.0f), cb_world).xyz);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 T = normalize(input.tangentW);
    float3 N = normalize(input.normalW);
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
  
    float3 sampleNormal = t_normal.Sample(s_linearWrap, input.texcoord).xyz * 2.0f - 1.0f;
    sampleNormal.y = -sampleNormal.y; // invert y for DirectX
    N = normalize(mul(sampleNormal, TBN));
    float3 albedo = t_albedo.Sample(s_linearWrap, input.texcoord).xyz;

    float3 aoRoughnessMetallic = t_aoRoughnessMetallic.Sample(s_linearWrap, input.texcoord).xyz;
    float ao = aoRoughnessMetallic.x;
    float roughness = aoRoughnessMetallic.y;
    float metallic = aoRoughnessMetallic.z;

    float3 V = normalize(cb_eyePos - input.posW);
    float NdotV = max(dot(N, V), 0.f);

    float3 lightCol = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        if (cb_light[i].bEnabled == 0)
        {
            continue;
        }

        // 광원 방향 계산
        float3 L = cb_light[i].type == LIGHT_TYPE_DIRECTIONAL ? -normalize(cb_light[i].lightDir) : normalize(cb_light[i].lightPos - input.posW);
        float3 H = normalize(L + V);

        // attanuation
        float dist = distance(cb_light[i].lightPos, input.posW);
        float attenuation = cb_light[i].type == LIGHT_TYPE_DIRECTIONAL ? 1.0f : smoothstep(cb_light[i].fallOffEnd, cb_light[i].fallOffStart, dist);

        // spot light
        float3 spotDir = normalize(cb_light[i].lightDir);
        float cosTheta = max(dot(-L, spotDir), 0.0f);
        float spotEffect = cb_light[i].type == LIGHT_TYPE_SPOT ? smoothstep(cb_light[i].outerConeCos, cb_light[i].innerConeCos, cosTheta) : 1.0f;
  
        float3 radiance = cb_light[i].lightColor * attenuation * spotEffect;

        float HdotV = max(dot(H, V), 0.0f);
        float NdotL = max(dot(N, L), 0.0f);
        float NdotH = max(dot(N, H), 0.0f);

        float D = DistributionGGX(NdotH, roughness);
        float G = GeometrySmith(NdotV, NdotL, roughness);
        float3 F = FresnelSchlick(HdotV, lerp(k_f0, albedo, metallic));
  
        float3 nominator = D * G * F;
        float denominator = 4.0f * NdotV * NdotL + 0.001f;
        float3 specular = nominator / denominator;
  
        float3 kS = F;
        float3 kD = 1.0f - kS;
        kD *= 1.0f - metallic;
  
        lightCol += (kD * albedo / k_pi + specular) * radiance * NdotL;
    }

    // IBL irradiance
    float3 kS = FresnelSchlickRoughness(NdotV, lerp(k_f0, albedo, metallic), roughness);
    float3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;
    float3 irradiance = t_iblIrradiance.Sample(s_linearWrap, N).rgb;
    float3 diffuse = irradiance * albedo;

    // IBL specular
    float3 R = normalize(reflect(-V, N));
    float3 prefilteredColor = t_iblSpecular.SampleLevel(s_linearWrap, R, roughness * k_maxReflectionLod).rgb;
    float2 brdf = t_brdfLUT.Sample(s_linearClamp, float2(NdotV, roughness)).rg;
    float3 specular = prefilteredColor * (kS * brdf.x + brdf.y);

    float3 ambient = (kD * diffuse + specular) * ao;
    float3 emissive = t_emissive.Sample(s_linearWrap, input.texcoord).xyz;
    float3 finalColor = ambient + lightCol + emissive;
    return float4(finalColor, 1.0f);
}