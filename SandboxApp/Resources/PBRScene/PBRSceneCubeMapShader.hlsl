#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define PADDING1 float CONCAT(padding, __LINE__)
#define PADDING2 float2 CONCAT(padding, __LINE__)
#define PADDING3 float3 CONCAT(padding, __LINE__)

#define CUBEMAP_NONE 0
#define CUBEMAP_IRRADIANCE 1
#define CUBEMAP_SPECULAR 2
#define CUBEMAP_ENVIRONMENT 3

cbuffer CBPBRSceneCamera : register(b1)
{
	float3 cb_eyePos;
        PADDING1;
	float4x4 cb_viewProj;
};

cbuffer CBPBRSceneCubeMap : register(b2)
{
	uint cb_type;
        PADDING3;
};

SamplerState s_linearWrap : register(s0);

TextureCube t_iblIrradiance : register(t0);
TextureCube t_iblSpecular : register(t1);
TextureCube t_iblEnv : register(t2);

struct VSInput
{
	float3 pos : POSITION;
};

struct PSInput
{
	float4 posH : SV_POSITION;
	float3 posL : POSITION;
};

PSInput VSMain(VSInput input)
{
	PSInput output;
	output.posH = float4(mul(float4(input.pos, 0.0f), cb_viewProj)).xyww;
	output.posL = input.pos;
	return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float3 dir = normalize(input.posL);
	if (cb_type == CUBEMAP_IRRADIANCE)
	{
		return t_iblIrradiance.Sample(s_linearWrap, dir);
	}
	else if (cb_type == CUBEMAP_SPECULAR)
	{
		return t_iblSpecular.Sample(s_linearWrap, dir);
	}
	else if (cb_type == CUBEMAP_ENVIRONMENT)
	{
		return t_iblEnv.Sample(s_linearWrap, dir);
	}
	else
	{
		return float4(0.2f, 0.2f, 0.2f, 1.0f);
	}
}