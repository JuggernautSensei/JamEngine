#include "ShaderBase.hlsl"

float3 Uncharted2ToneMapping(float3 color, float exposure, float gamma)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    color *= exposure;
    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    color /= white;
    color = saturate(color);
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 ReinhardToneMapping(float3 color, float exposure, float gamma)
{
    color *= exposure / (1.f + color / exposure);

    float3 gammaInv = float3(1.f, 1.f, 1.f) / gamma;

    color = saturate(color);
    color = pow(color, gammaInv);
    return color;
}

float3 LinearToneMapping(float3 color, float exposure, float gamma)
{
    color = clamp(exposure * color, 0.f, 1.f);
    color = pow(abs(color), float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 LumaBasedReinhardToneMapping(float3 color, float exposure, float gamma)
{
    color *= exposure;
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma / (1.0 + luma);
    color *= toneMappedLuma / max(luma, k_epsilon);
    color = saturate(color);
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 WhitePreservingReinhardToneMapping(float3 color, float exposure, float gamma)
{
    float white = 2.0;
    color *= exposure;
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma * (1.0 + luma / (white * white)) / (1.0 + luma);
    color *= toneMappedLuma / max(luma, k_epsilon);
    color = saturate(color);
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 RomBinDaHouseToneMapping(float3 color, float exposure, float gamma)
{
    color *= exposure;
    color = exp(-1.0 / (2.72 * color + 0.15));
    color = saturate(color);
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 FilmicToneMapping(float3 color, float exposure, float gamma)
{
    color *= exposure;
    color = max(float3(0.0, 0.0, 0.0), color - float3(0.004, 0.004, 0.004));
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    color = saturate(color);
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float4 PSMain(FULLSCREEN_PS_INPUT _input) : SV_TARGET
{
    float3 color = t_input0.Sample(s_linearClamp, _input.texCoord0).rgb;
    float3 toneMappedColor;

    switch (cb_toneMappingType)
    {
        case JAM_ENVIRONMENT_TONE_MAPPING_TYPE_LINEAR:
            toneMappedColor = LinearToneMapping(color, cb_exposure, cb_gamma);
            break;
        case JAM_ENVIRONMENT_TONE_MAPPING_TYPE_UNCHARTED2:
            toneMappedColor = Uncharted2ToneMapping(color, cb_exposure, cb_gamma);
            break;
        case JAM_ENVIRONMENT_TONE_MAPPING_TYPE_REINHARD:
            toneMappedColor = ReinhardToneMapping(color, cb_exposure, cb_gamma);
            break;
        case JAM_ENVIRONMENT_TONE_MAPPING_TYPE_LUMA_BASED_REINHARD:
            toneMappedColor = LumaBasedReinhardToneMapping(color, cb_exposure, cb_gamma);
            break;
        case JAM_ENVIRONMENT_TONE_MAPPING_TYPE_WHITE_PRESERVING_REINHARD:
            toneMappedColor = WhitePreservingReinhardToneMapping(color, cb_exposure, cb_gamma);
            break;
        case JAM_ENVIRONMENT_TONE_MAPPING_TYPE_ROMB_DA_HOUSE:
            toneMappedColor = RomBinDaHouseToneMapping(color, cb_exposure, cb_gamma);
            break;
        case JAM_ENVIRONMENT_TONE_MAPPING_TYPE_FILMIC:
            toneMappedColor = FilmicToneMapping(color, cb_exposure, cb_gamma);
            break;
        default:
            toneMappedColor = LinearToneMapping(color, cb_exposure, cb_gamma);
            break;
    }

    return float4(toneMappedColor, 1.f);
}
