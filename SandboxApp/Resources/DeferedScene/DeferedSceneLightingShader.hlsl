#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define PADDING1 float CONCAT(padding, __LINE__)
#define PADDING2 float2 CONCAT(padding, __LINE__)
#define PADDING3 float3 CONCAT(padding, __LINE__)

cbuffer CBDeferedSceneCamera : register(b1)
{
    float3 cb_eyePos;
    PADDING1;
    float4x4 cb_viewProj;
    float4x4 cb_invViewProj;
};

cbuffer CBDeferedSceneLight : register(b2)
{
    float3 cb_lightPos;
    float cb_lightRadius;
    float3 cb_lightCol;
    PADDING1;
};

struct VSInput
{
    float3 pos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

Texture2D t_depthScene : register(t2);
Texture2D t_colorScene : register(t3);
Texture2D t_normalScene : register(t4);

SamplerState s_linearWrap : register(s0);

// full screen tri
// [-1, 1, 0] [3, 1, 0] [-1, -3, 0]
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.pos, 1.0f);
    output.texcoord = float2(output.posH.x, -output.posH.y) * 0.5f + 0.5f;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // normal
    float3 N = t_normalScene.Sample(s_linearWrap, input.texcoord).rgb * 2.f - 1.f;
    N = normalize(N);

    // world
    float D = t_depthScene.Sample(s_linearWrap, input.texcoord).r;
    float3 ndc = float3(input.texcoord.x * 2.0f - 1.0f, (1.0f - input.texcoord.y) * 2.0f - 1.0f, D);
    float4 world = mul(float4(ndc, 1.0f), cb_invViewProj);
    world.xyz /= world.w;

    // light color
    float dist = distance(cb_lightPos, world.xyz);
    float att = saturate(1.0f - (dist / cb_lightRadius));
    float3 lightCol = cb_lightCol * att * att;

    // lighting
    float3 albedo = t_colorScene.Sample(s_linearWrap, input.texcoord).xyz;
    float3 V = normalize(cb_eyePos - world.xyz);
    float3 L = normalize(cb_lightPos - world.xyz);
    float3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.f);
    float HdotN = max(dot(H, N), 0.f);
    float3 diffuse = lightCol * albedo * NdotL;
    float3 specular = lightCol * pow(HdotN, 16.f);
    return float4(diffuse + specular, 1.f);
}