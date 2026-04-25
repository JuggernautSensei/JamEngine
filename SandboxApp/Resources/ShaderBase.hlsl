#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define PADDING1 float CONCAT(padding1, __LINE__)
#define PADDING2 float2 CONCAT(padding2, __LINE__)
#define PADDING3 float3 CONCAT(padding3, __LINE__)

#define SPLAT2(_x) float2(_x, _x)
#define SPLAT3(_x) float3(_x, _x, _x)
#define SPLAT4(_x) float4(_x, _x, _x, _x)

float2 CvtClipToTex(float4 _posClip)
{
    float2 texcoord = _posClip.xy / _posClip.w;
    texcoord = texcoord * 0.5f + 0.5f;
    texcoord.y = 1.f - texcoord.y;
    return texcoord;
}
