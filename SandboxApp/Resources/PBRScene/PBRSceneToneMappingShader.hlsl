struct VSInput
{
    float3 pos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

SamplerState s_linearClamp : register(s1);
Texture2D t_hdrScene : register(t0);

// full screen tri
// [-1, 1, 0] [3, 1, 0] [-1, -3, 0]
//                ↓
// [0, 0] [2, 0] [0, 2]
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.pos, 1.0f);
    output.texcoord = float2(output.posH.x, -output.posH.y) * 0.5f + 0.5f;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 col = t_hdrScene.Sample(s_linearClamp, input.texcoord).rgb;
    col = col / (col + 1.0f);
    col = pow(col, 1.0f / 2.2f);
    return float4(col, 1.0f);
}