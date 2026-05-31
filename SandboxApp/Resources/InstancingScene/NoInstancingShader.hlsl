cbuffer CBEntity : register(b0)
{
    float4x4 cb_world;
};

cbuffer CBCamera : register(b1)
{
    float3 cb_eyePos;
    float padding1;
    float4x4 cb_viewProj;
};

struct VSInput
{
    float3 pos : POSITION;
    uint4 color : COLOR;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    float4 posW = mul(float4(input.pos, 1.0f), cb_world);
    output.posH = mul(posW, cb_viewProj);
    output.color = input.color / 255.f;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}