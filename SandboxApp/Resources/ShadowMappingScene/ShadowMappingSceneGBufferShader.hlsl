#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define PADDING1 float CONCAT(padding1, __LINE__)

cbuffer CBShadowMappingSceneEntity : register(b0)
{
    float4x4 cb_world;
    float4x4 cb_worldInvTrans;
};

cbuffer CBShadowMappingSceneCamera : register(b1)
{
    float4x4 cb_view;
    float4x4 cb_proj;
    float3 cb_eyePos;
    PADDING1;
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
    output.posW = mul(float4(input.pos, 1.0f), cb_world).xyz;
    output.posH = mul(float4(output.posW, 1.f), mul(cb_view, cb_proj));
    output.normalW = normalize(mul(float4(input.normal, 0.0f), cb_worldInvTrans).xyz);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normalW);
    N = N * 0.5f + 0.5f; // [-1, 1] -> [0, 1]
    return float4(N, 1.f);
}