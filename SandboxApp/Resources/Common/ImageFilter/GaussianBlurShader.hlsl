#include "ImageFilterCommon.hlsl"

#define GAUSSIAN_BLUR_HORIZONTAL    0
#define GAUSSIAN_BLUR_VERTICAL      1

// default is vertical blur
// #define GAUSSIAN_BLUR_DIRECTION GAUSSIAN_BLUR_HORIZONTAL 
// #define GAUSSIAN_BLUR_DIRECTION GAUSSIAN_BLUR_VERTICAL << default

// #define GAUSSIAN_FILTER_LEN 3
// #define GAUSSIAN_FILTER_LEN 5 << default
// #define GAUSSIAN_FILTER_LEN 7
// #define GAUSSIAN_FILTER_LEN 9

#ifndef GAUSSIAN_FILTER_LEN
#define GAUSSIAN_FILTER_LEN 5
#endif

#ifndef GAUSSIAN_BLUR_DIRECTION
#define GAUSSIAN_BLUR_DIRECTION GAUSSIAN_BLUR_VERTICAL
#endif

#if GAUSSIAN_FILTER_LEN == 3
static const float kFilter[] = { 0.225610, 0.548780, 0.225610 };
#elif GAUSSIAN_FILTER_LEN == 5
static const float kFilter[] = { 0.092421, 0.241376, 0.332406, 0.241376, 0.092421 };
#elif GAUSSIAN_FILTER_LEN == 7
static const float kFilter[] = { 0.054781, 0.123924, 0.202241, 0.238109, 0.202241, 0.123924, 0.054781 };
#elif GAUSSIAN_FILTER_LEN == 9
static const float kFilter[] = { 0.038182, 0.076227, 0.124904, 0.167979, 0.185417, 0.167979, 0.124904, 0.076227, 0.038182 };
#else
#error "unsupported filter length"
#endif

float4 PSMain(PSInput _input)
{
    uint w, h;
    t_input0.GetDimensions(w, h);
    float texelX = 1.f / w;
    float texelY = 1.f / h;

    float4 color = 0.f;
    int hLen = (int) (GAUSSIAN_FILTER_LEN / 2);

    [unroll]
    for (int i = -hLen; i <= hLen; ++i)
    {
        float2 offset =
#if GAUSSIAN_BLUR_DIRECTION == GAUSSIAN_BLUR_HORIZONTAL 
        float2(i * texelX, 0.f);
#else 
        float2(0.f, i * texelY);
#endif

        float2 texcoord = _input.texcoord + offset;
        float4 sample = t_input0.Sample(s_linearClamp, texcoord);
        color += sample * kFilter[i + hLen];
    }

    return color;
}