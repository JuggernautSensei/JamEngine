cbuffer CBInstancingSceneEntity : register(b0)
{
    float3 cb_baseColor;
    float cb_shiness;
};

cbuffer CBInstancingSceneCamera : register(b1)
{
    float3 cb_eyePos;
    float padding1;
    float4x4 cb_viewProj;
};

cbuffer CBInstancingSceneLight : register(b2)
{
    float3 cb_lightDir;
    float padding2;
    float3 cb_lightColor;
    float padding3;
    float3 cb_ambientColor;
    float padding4;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4x4 instanceWorld : INSTANCE0;
    float4x4 instanceWorldInvTrans : INSTANCE4;
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
    float4 posW = mul(float4(input.pos, 1.0f), input.instanceWorld);
    output.posH = mul(posW, cb_viewProj);
    output.posW = posW.xyz;
    output.normalW = normalize(mul(float4(input.normal, 0.0f), input.instanceWorldInvTrans).xyz);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normalW);
    float3 L = normalize(-cb_lightDir);
    float3 E = normalize(cb_eyePos - input.posW);
    float3 H = normalize(L + E); // half way vector for blinn-phong specular
    float NdotL = max(dot(N, L), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    float3 diffuse = cb_lightColor * cb_baseColor * NdotL;
    float3 specular = cb_lightColor * pow(NdotH, cb_shiness); // blinn-phong specular with shininess of 16
    float3 ambient = cb_ambientColor * cb_baseColor;
    return float4(diffuse + specular + ambient, 1.0f);
}
