#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define PADDING1 float CONCAT(padding1, __LINE__)
#define PADDING2 float2 CONCAT(padding2, __LINE__)
#define PADDING3 float3 CONCAT(padding3, __LINE__)

SamplerState s_linearClamp : register(s0);
SamplerState s_linearWrap : register(s1);
SamplerComparisonState s_shadowComparison : register(s2);

const static float kPi = 3.14159265358979323846f;
const static float k2Pi = 6.28318530717958647692f;
const static float kHalfPi = 1.57079632679489661923f;
const static float kEpsilon = 1e-6f;

float Square(int _x)
{
    return _x * _x;
}

float Square(float _x)
{
    return _x * _x;
}

float2 Square(float2 _x)
{
    return _x * _x;
}

float3 Square(float3 _x)
{
    return _x * _x;
}

float4 Square(float4 _x)
{
    return _x * _x;
}

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

float CalcDistanceAttenuation(float _dist, float _fallOffStart, float _fallOffEnd)
{
    return 1.f - smoothstep(_fallOffStart, _fallOffEnd, _dist);
}

float CalcSpotAttenuation(float3 _L, float3 _lightDir, float _spotInnerCos, float _spotOuterCos)
{
    float d = dot(-_L, _lightDir);
    return smoothstep(_spotOuterCos, _spotInnerCos, d);
}

float CalcBlinePhong(float3 _L, float3 _V, float3 _N, float _shininess)
{
    float3 H = normalize(_L + _V);
    float nDotL = max(dot(_N, _L), 0.f);
    float nDotH = max(dot(_N, H), 0.f);
    return nDotL + pow(nDotH, _shininess);
}

uint WangHash(uint _seed)
{
    _seed = (_seed ^ 61u) ^ (_seed >> 16u);
    _seed *= 9u;
    _seed ^= _seed >> 4u;
    _seed *= 0x27D4EB2Du;
    _seed ^= _seed >> 15u;
    return _seed;
}

float MakeRandomFloat(uint _seed, float _min = 0, float _max = 1.f)
{
    uint hash = WangHash(_seed);
    float f01 = (float) hash / (float) 0xFFFFFFFFu;
    return lerp(_min, _max, f01);
}

uint MakeRandomUInt(uint _seed, uint _min = 0, uint _max = 0xFFFFFFFFu)
{
    uint hash = WangHash(_seed);
    return (hash % (_max - _min + 1)) + _min;
}

float3 CalcTextureArrayLocationFromDir(float3 _dir)
{
    float3 absDir = abs(_dir);
    float maxAxis = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    int layer = 0;

    if (absDir.x >= absDir.y && absDir.x >= absDir.z)
    {
        maxAxis = absDir.
x;
        if (_dir.x > 0.0f)
        {
            layer = 0; // +X
            u = -_dir.
z;
            v = -_dir.y;
        }
        else
        {
            layer = 1; // -X
            u = _dir.
z;
            v = -_dir.y;
        }
    }
    else if (absDir.y >= absDir.x && absDir.y >= absDir.z)
    {
        maxAxis = absDir.
y;
        if (_dir.y > 0.0f)
        {
            layer = 2; // +Y
            u = _dir.
x;
            v = _dir.z;
        }
        else
        {
            layer = 3; // -Y
            u = _dir.
x;
            v = -_dir.z;
        }
    }
    else
    {
        maxAxis = absDir.
z;
        if (_dir.z > 0.0f)
        {
            layer = 4; // +Z
            u = _dir.
x;
            v = -_dir.y;
        }
        else
        {
            layer = 5; // -Z
            u = -_dir.
x;
            v = -_dir.y;
        }
    }

    float2 uv = float2(u, v) / maxAxis;
    uv = uv * 0.5f + 0.5f;
    return float3(uv, (float) layer);
}
