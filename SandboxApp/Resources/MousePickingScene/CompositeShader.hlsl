Texture2D<float4> t_color : register(t0);
Texture2D<uint> t_id : register(t1);

cbuffer CBSelection : register(b0)
{
    uint cb_selectedId;
    uint cb_showIds;
    int cb_outlineWidth;
    uint cb_padding;
};

struct VSInput
{
    float2 pos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.pos, 0.f, 1.f);
    output.uv = input.pos * float2(0.5f, -0.5f) + 0.5f;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    uint width, height;
    t_id.GetDimensions(width, height);
    const int2 size = int2(width, height);
    const int2 pixel = clamp(int2(input.uv * size), int2(0, 0), size - 1);
    const uint id = t_id.Load(int3(pixel, 0));
    float3 color = t_color.Load(int3(pixel, 0)).rgb;
    if (cb_showIds != 0)
    {
        const uint hash = id * 1664525u + 1013904223u;
        color = id == 0 ? float3(0.f, 0.f, 0.f) : 0.25f + 0.75f * float3(hash & 255u, (hash >> 8) & 255u, (hash >> 16) & 255u) / 255.f;
    }
    if (cb_selectedId != 0 && id == cb_selectedId)
    {
        const int radius = clamp(cb_outlineWidth, 1, 5);
        bool edge = false;
        [loop]
        for (int y = -radius; y <= radius && !edge; ++y)
        {
            [loop]
            for (int x = -radius; x <= radius && !edge; ++x)
            {
                const int2 neighbor = clamp(pixel + int2(x, y), int2(0, 0), size - 1);
                edge = edge || t_id.Load(int3(neighbor, 0)) != id;
            }
        }
        const float3 gold = float3(1.f, 0.8f, 0.05f);
        color = edge ? gold : lerp(color, gold, 0.08f);
    }
    return float4(color, 1.f);
}
