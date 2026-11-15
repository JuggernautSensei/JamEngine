cbuffer CBEntity : register(b0)
{
    float4x4 cb_world;
    float3 cb_color;
    float padding1;
};

cbuffer CBCamera : register(b1)
{
    float3 cb_eyePos;
    float padding2;
    float4x4 cb_viewProj;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 normalW : NORMAL;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.pos, 1.0f), cb_world);
    output.posH = mul(posW, cb_viewProj);
    output.normalW = normalize(mul(float4(input.normal, 0.0f), cb_world).xyz);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 L = normalize(float3(-0.4f, 1.0f, -0.6f));
    float NdotL = max(dot(normalize(input.normalW), L), 0.0f);
    return float4(cb_color * (0.25f + 0.75f * NdotL), 1.0f);
}
