#define KERNEL_SIZE 64

Texture2D t_normal : register(t0);
Texture2D<float> t_depth : register(t1);
Texture2D t_noise : register(t2);

cbuffer CBSSAO : register(b0)
{
    float4x4 cb_proj;
    float4x4 cb_invProj;
    float4 cb_kernel[KERNEL_SIZE];
    float2 cb_screenSize;
    float cb_radius;
    float cb_bias;
    float cb_power;
    float3 cb_padding;
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

float3 ViewPosition(int2 _pixel)
{
    const float depth = t_depth.Load(int3(_pixel, 0));
    const float2 uv = (float2(_pixel) + 0.5f) / cb_screenSize;
    const float2 ndc = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
    const float4 posV = mul(float4(ndc, depth, 1.0f), cb_invProj);
    return posV.xyz / posV.w;
}

float PSMain(PSInput input) : SV_TARGET
{
    const int2 pixel = int2(input.posH.xy);
    if (t_depth.Load(int3(pixel, 0)) >= 1.0f)
    {
        return 1.0f;
    }

    const float3 P = ViewPosition(pixel);
    const float3 N = normalize(t_normal.Load(int3(pixel, 0)).xyz);

    // 4x4 노이즈로 커널을 픽셀마다 회전
    const float3 randomVec = t_noise.Load(int3(int2(uint2(pixel) % 4u), 0)).xyz;
    const float3 T = normalize(randomVec - N * dot(randomVec, N));
    const float3 B = cross(N, T);
    const float3x3 TBN = float3x3(T, B, N);

    float occlusion = 0.0f;
    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
        const float3 samplePos = P + mul(cb_kernel[i].xyz, TBN) * cb_radius;

        float4 clip = mul(float4(samplePos, 1.0f), cb_proj);
        clip.xy /= clip.w;
        const float2 uv = float2(clip.x * 0.5f + 0.5f, 0.5f - clip.y * 0.5f);
        const int2 samplePixel = clamp(int2(uv * cb_screenSize), int2(0, 0), int2(cb_screenSize) - 1);

        const float sceneDepth = ViewPosition(samplePixel).z;

        // 샘플이 장면 표면 뒤에 있으면 가려진 것. 멀리 떨어진 표면은 range check로 제외
        const float rangeCheck = smoothstep(0.0f, 1.0f, cb_radius / abs(P.z - sceneDepth));
        occlusion += (sceneDepth <= samplePos.z - cb_bias ? 1.0f : 0.0f) * rangeCheck;
    }

    return pow(saturate(1.0f - occlusion / KERNEL_SIZE), cb_power);
}
