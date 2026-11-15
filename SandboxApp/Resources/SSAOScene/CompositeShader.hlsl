Texture2D t_color : register(t0);
Texture2D t_normal : register(t1);
Texture2D<float> t_depth : register(t2);
Texture2D<float> t_ssao : register(t3);

cbuffer CBComposite : register(b0)
{
    float3 cb_lightDirV;
    float cb_ambient;
    float cb_bUseAo;
    float cb_bAoOnly;
    float2 cb_padding;
};

struct VSInput
{
    float2 pos : POSITION;
};

struct PSInput
{
    float4 posH : SV_POSITION;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.posH = float4(input.pos, 0.0f, 1.0f);
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    const int3 pixel = int3(int2(input.posH.xy), 0);
    if (t_depth.Load(pixel) >= 1.0f)
    {
        return float4(0.02f, 0.03f, 0.05f, 1.0f);
    }

    const float3 albedo = t_color.Load(pixel).rgb;
    const float3 N = normalize(t_normal.Load(pixel).xyz);
    const float ao = cb_bUseAo > 0.5f ? t_ssao.Load(pixel) : 1.0f;

    if (cb_bAoOnly > 0.5f)
    {
        return float4(ao.xxx, 1.0f);
    }

    // AO는 주변광에만 적용
    const float diffuse = saturate(dot(N, cb_lightDirV));
    const float3 color = albedo * (cb_ambient * ao + (1.0f - cb_ambient) * diffuse);
    return float4(color, 1.0f);
}
