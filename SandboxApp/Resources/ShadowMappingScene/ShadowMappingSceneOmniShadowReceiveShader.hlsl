#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define PADDING1 float CONCAT(padding1, __LINE__)

Texture2D t_sceneNormal;
Texture2D t_sceneLight; 
SamplerState s_linearWrap;

struct VSInput
{
    float2 posL : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.posL, 0.f, 1.f);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
}