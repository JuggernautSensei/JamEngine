Texture2D<float> t_ssao : register(t0);

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

// 노이즈 타일(4x4)과 같은 크기의 박스 블러
float PSMain(PSInput input) : SV_TARGET
{
    uint width, height;
    t_ssao.GetDimensions(width, height);
    const int2 maxPixel = int2(width, height) - 1;
    const int2 pixel = int2(input.posH.xy);

    float sum = 0.0f;
    for (int y = -2; y < 2; ++y)
    {
        for (int x = -2; x < 2; ++x)
        {
            sum += t_ssao.Load(int3(clamp(pixel + int2(x, y), int2(0, 0), maxPixel), 0));
        }
    }
    return sum / 16.0f;
}
