struct VSInput
{
    float2 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D t_input : register(t0);
SamplerState s_linearWrap : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.posL, 0.f, 1.f);
    output.texCoord = input.posL * 0.5f + 0.5f;
    output.texCoord.y = 1.f - output.texCoord.y;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return t_input.Sample(s_linearWrap, input.texCoord);
}