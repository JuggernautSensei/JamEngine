Texture2D t_albedo : register(t0);

SamplerState s_linearWrap : register(s0);

cbuffer CBEntity : register(b0)
{
    float4x4 cb_world;
    float4x4 cb_invTransWorld;
    float3 cb_baseColor;
    float cb_reflectivity;
    float cb_bUseTexture;
    float3 cb_padding;
};

cbuffer CBCamera : register(b1)
{
    float4x4 cb_view;
    float4x4 cb_viewProj;
};

struct VSInput
{
    float3 pos : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normalV : NORMAL;
};

struct PSOutput
{
    float4 color : SV_TARGET0;
    float4 normal : SV_TARGET1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.pos, 1.0f), cb_world);
    output.posH = mul(posW, cb_viewProj);
    output.texcoord = input.texcoord;

    float3 normalW = normalize(mul(float4(input.normal, 0.0f), cb_invTransWorld).xyz);
    output.normalV = mul(float4(normalW, 0.0f), cb_view).xyz;
    return output;
}

PSOutput PSMain(PSInput input)
{
    float3 albedo = cb_baseColor;
    if (cb_bUseTexture > 0.5f)
    {
        albedo *= t_albedo.Sample(s_linearWrap, input.texcoord).rgb;
    }

    PSOutput output;
    output.color = float4(albedo, 1.0f);
    output.normal = float4(normalize(input.normalV), cb_reflectivity);
    return output;
}
