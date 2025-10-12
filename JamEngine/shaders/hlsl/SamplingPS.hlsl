struct SCREENSPACE_EFFECT_PS_INPUT
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

Texture2D postProcessInputTexture1 : register(t0);
SamplerState samplerLinearClamp : register(s0);

float4 PSmain(SCREENSPACE_EFFECT_PS_INPUT input) : SV_TARGET
{
    float4 finalColor = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord);
    return finalColor;
}