cbuffer CBEntity : register(b0)
{
    float4x4 cb_world;
    float4x4 cb_invTransWorld;
    float3 cb_color;
    uint cb_objectId;
};

cbuffer CBCamera : register(b1)
{
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
    float3 posW : TEXCOORD0;
    float3 normalW : TEXCOORD1;
};

struct PSOutput
{
    float4 color : SV_TARGET0;
    uint objectId : SV_TARGET1;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    const float4 worldPos = mul(float4(input.pos, 1.f), cb_world);
    output.posH = mul(worldPos, cb_viewProj);
    output.posW = worldPos.xyz;
    output.normalW = mul(float4(input.normal, 0.f), cb_invTransWorld).xyz;
    return output;
}

PSOutput PSMain(PSInput input)
{
    const float3 lightDir = normalize(float3(-0.4f, 0.8f, -0.6f));
    const float diffuse = saturate(dot(normalize(input.normalW), lightDir));
    float3 color = cb_color * (0.4f + 0.6f * diffuse);
    if (cb_objectId == 0)
    {
        const float2 uv = input.posW.xz;
        const float2 grid = abs(frac(uv - 0.5f) - 0.5f) / max(fwidth(uv), 0.0001f);
        color *= 1.f - (1.f - saturate(min(grid.x, grid.y))) * 0.18f;
    }
    PSOutput output;
    output.color = float4(color, 1.f);
    output.objectId = cb_objectId;
    return output;
}
