#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define PADDING1 float CONCAT(padding1, __LINE__)
#define PADDING2 float2 CONCAT(padding2, __LINE__)
#define PADDING3 float3 CONCAT(padding3, __LINE__)

SamplerState s_linearClamp : register(s0);
SamplerState s_linearWrap : register(s1);
SamplerComparisonState s_shadowComparison : register(s2);

float2 CvtNdcToTex(float2 _posNdcXy)
{
    float2 texcoord = _posNdcXy * 0.5f + 0.5f;
    texcoord.y = 1.f - texcoord.y;
    return texcoord;
}

float3 CvtNdcToWorld(float3 _posNdc, float4x4 _invViewProj)
{
    float4 posClip = float4(_posNdc, 1.f);
    float4 posWorld = mul(posClip, _invViewProj);
    posWorld.xyz /= posWorld.w;
    return posWorld.xyz;
}

float3 CvtNdcToWorld(float2 _posNdcXy, float _depthNdc, float4x4 _invViewProj)
{
    float4 posClip = float4(_posNdcXy, _depthNdc, 1.f);
    float4 posWorld = mul(posClip, _invViewProj);
    posWorld.xyz /= posWorld.w;
    return posWorld.xyz;
}

float2 CvtTexToNdc(float2 _texcoord)
{
    return float2(_texcoord.x * 2.f - 1.f, 1.f - _texcoord.y * 2.f);
}

float2 CvtClipToTex(float4 _posClip)
{
    float2 texcoord = _posClip.xy / _posClip.w;
    texcoord = texcoord * 0.5f + 0.5f;
    texcoord.y = 1.f - texcoord.y;
    return texcoord;
}

float3 CvtClipToHomo(float4 _posClip)
{
    return _posClip.xyz / _posClip.w;
}

float4 CvtRGBAToLinear(uint _rgba)
{
    float4 color;
    color.r = ((_rgba >> 24) & 0xFF) / 255.f;
    color.g = ((_rgba >> 16) & 0xFF) / 255.f;
    color.b = ((_rgba >> 8) & 0xFF) / 255.f;
    color.a = ((_rgba >> 0) & 0xFF) / 255.f;
    return color;
}

