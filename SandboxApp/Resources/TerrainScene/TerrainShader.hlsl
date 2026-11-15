Texture2D<float> t_height : register(t0);
Texture2D t_grass : register(t1);
Texture2D t_rock : register(t2);
Texture2D t_snow : register(t3);
Texture2D t_dirt : register(t4);

SamplerState s_linearWrap : register(s0);

cbuffer CBTerrain : register(b0)
{
    float cb_terrainSize;
    float cb_heightScale;
    float cb_heightMapSize;
    float cb_tileSize;
    float3 cb_lightDir;
    float cb_snowHeight;
    float cb_rockSlopeMin;
    float cb_rockSlopeMax;
    float cb_snowBlend;
    float cb_debugView;
    float cb_dirtHeight;
    float3 cb_padding;
};

cbuffer CBCamera : register(b1)
{
    float3 cb_eyePos;
    float padding1;
    float4x4 cb_viewProj;
};

struct VSInput
{
    float2 gridPos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float height01 : TEXCOORD0;
};

float Height(int2 _texel)
{
    const int maxIndex = (int)cb_heightMapSize - 1;
    _texel = clamp(_texel, int2(0, 0), int2(maxIndex, maxIndex));
    return t_height.Load(int3(_texel, 0));
}

PSInput VSMain(VSInput input)
{
    // 정점 하나가 하이트맵 텍셀 하나에 대응
    const int2 texel = (int2)round(input.gridPos * (cb_heightMapSize - 1.0f));
    const float h = Height(texel);

    const float2 worldXZ = (input.gridPos - 0.5f) * cb_terrainSize;

    // 중앙 차분 노멀
    const float cell = cb_terrainSize / (cb_heightMapSize - 1.0f);
    const float hl = Height(texel + int2(-1, 0));
    const float hr = Height(texel + int2(1, 0));
    const float hd = Height(texel + int2(0, -1));
    const float hu = Height(texel + int2(0, 1));
    const float3 normal = normalize(float3((hl - hr) * cb_heightScale, 2.0f * cell, (hd - hu) * cb_heightScale));

    PSInput output;
    output.posW = float3(worldXZ.x, h * cb_heightScale, worldXZ.y);
    output.posH = mul(float4(output.posW, 1.0f), cb_viewProj);
    output.normalW = normal;
    output.height01 = h;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    const float3 N = normalize(input.normalW);

    // 경사 0: 평평, 1: 절벽
    const float slope = 1.0f - N.y;

    const float rockW = smoothstep(cb_rockSlopeMin, cb_rockSlopeMax, slope);
    const float snowW = smoothstep(cb_snowHeight, cb_snowHeight + cb_snowBlend, input.height01) * (1.0f - rockW);
    const float dirtW = (1.0f - smoothstep(0.0f, cb_dirtHeight, input.height01)) * (1.0f - rockW) * (1.0f - snowW);
    const float grassW = saturate(1.0f - rockW - snowW - dirtW);
    const float sum = max(rockW + snowW + dirtW + grassW, 1e-4f);

    const int debugView = (int)cb_debugView;
    if (debugView == 1)
    {
        return float4(input.height01.xxx, 1.0f);
    }
    if (debugView == 2)
    {
        return float4(N * 0.5f + 0.5f, 1.0f);
    }
    if (debugView == 3)
    {
        return float4(rockW, snowW, dirtW, 1.0f);
    }

    const float2 uv = input.posW.xz / cb_tileSize;
    const float3 grass = t_grass.Sample(s_linearWrap, uv).rgb;
    const float3 rock = t_rock.Sample(s_linearWrap, uv).rgb;
    const float3 snow = t_snow.Sample(s_linearWrap, uv).rgb;
    const float3 dirt = t_dirt.Sample(s_linearWrap, uv).rgb;
    const float3 albedo = (grass * grassW + rock * rockW + snow * snowW + dirt * dirtW) / sum;

    const float NdotL = saturate(dot(N, cb_lightDir));
    return float4(albedo * (0.25f + 0.75f * NdotL), 1.0f);
}
