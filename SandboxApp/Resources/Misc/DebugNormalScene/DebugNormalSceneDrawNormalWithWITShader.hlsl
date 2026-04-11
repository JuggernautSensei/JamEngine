cbuffer CBDebugNormalScene_WithWIT : register(b0)
{
    float4x4 world;
    float4x4 worldInvTrans;
};

cbuffer CBDebugNormalSceneCamera : register(b1)
{
    float4x4 viewProj;
    float3 eyePos;
    float padding1;
};

struct VSInput
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
};

struct GSInput
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 color : COLOR;
};

GSInput VSMain(VSInput input)
{
    GSInput output;
    output.posL = input.posL;
    output.normalL = input.normalL;
    return output;
}

[maxvertexcount(2)]
void GSMain(point GSInput inputs[1], inout LineStream<PSInput> lineStream)
{
    GSInput input = inputs[0];

    float3 posW = mul(float4(input.posL, 1.0f), world).xyz;
    float3 N = normalize(mul(float4(input.normalL, 0.0f), worldInvTrans).xyz);

    PSInput output0;
    output0.posH = mul(float4(posW, 1.0f), viewProj);
    output0.color = float3(1.f, 0.f, 0.f);
    lineStream.Append(output0);

    PSInput output1;
    output1.posH = mul(float4(posW + N * 0.25f, 1.0f), viewProj);
    output1.color = float3(1.f, 1.f, 0.f);
    lineStream.Append(output1);
}

const static float3 k_lightDir = normalize(float3(-1.f, -1.f, 1.f));
const static float3 k_lightColor = float3(1.f, 1.f, 1.f);
const static float3 k_ambientColor = float3(0.1f, 0.1f, 0.1f);
const static float3 k_diffuseColor = float3(1.f, 1.f, 1.f);
const static float3 k_specularColor = float3(1.f, 1.f, 1.f);

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.color, 1.f);
}