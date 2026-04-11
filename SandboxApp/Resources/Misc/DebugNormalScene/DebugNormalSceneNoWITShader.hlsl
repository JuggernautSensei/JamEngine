cbuffer CBDebugNormalScene_NoWIT : register(b0)
{
    float4x4 world;
};

cbuffer CBDebugNormalSceneCamera : register(b1)
{
    float4x4 viewProj;
    float3 eyePos;
    float padding1;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posW = mul(float4(input.pos, 1.0f), world).xyz;
    output.normalW = mul(float4(input.normal, 0.0f), world).xyz;
    output.posH = mul(float4(output.posW, 1.0f), viewProj);
    return output;
}

const static float3 k_lightDir = normalize(float3(-1.f, -1.f, 1.f));
const static float3 k_lightColor = float3(1.f, 1.f, 1.f);
const static float3 k_ambientColor = float3(0.1f, 0.1f, 0.1f);
const static float3 k_diffuseColor = float3(1.f, 1.f, 1.f);
const static float3 k_specularColor = float3(1.f, 1.f, 1.f);

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normalW);
    float3 V = normalize(eyePos - input.posW);
    float3 L = normalize(-k_lightDir);
    float3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.f);
    float NdotH = max(dot(N, H), 0.f);
    float3 diffuse = k_diffuseColor * NdotL;
    float3 specular = k_specularColor * pow(NdotH, 16.f);
    float3 color = k_ambientColor + diffuse + specular;
    return float4(color, 1.f);
}