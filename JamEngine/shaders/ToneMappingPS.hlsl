#include "ShaderCommon.hlsl"

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
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 ReinhardToneMapping(float3 color, float exposure, float gamma)
{
    color *= exposure / (1.f + color / exposure);

    float3 gammaInv = float3(1.f, 1.f, 1.f) / gamma;
    color = pow(color, gammaInv);
    return color;
}

float3 LinearToneMapping(float3 color, float exposure, float gamma)
{
    color = clamp(exposure * color, 0.f, 1.f);
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 LumaBasedReinhardToneMapping(float3 color, float exposure, float gamma)
{
    color *= exposure;
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma / (1.0 + luma);
    color *= toneMappedLuma / max(luma, JAM_TOLERANCE);
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 WhitePreservingReinhardToneMapping(float3 color, float exposure, float gamma)
{
    float white = 2.0;
    color *= exposure;
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    float toneMappedLuma = luma * (1.0 + luma / (white * white)) / (1.0 + luma);
    color *= toneMappedLuma / max(luma, JAM_TOLERANCE);
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 RomBinDaHouseToneMapping(float3 color, float exposure, float gamma)
{
    color *= exposure;
    color = exp(-1.0 / (2.72 * color + 0.15));
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}

float3 FilmicToneMapping(float3 color, float exposure, float gamma)
{
    color *= exposure;
    color = max(float3(0.0, 0.0, 0.0), color - float3(0.004, 0.004, 0.004));
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    color = pow(color, float3(1.f, 1.f, 1.f) / gamma);
    return color;
}


float4 PSmain(SCREENSPACE_EFFECT_PS_INPUT input) : SV_TARGET
{
    float3 color = postProcessInputTexture1.Sample(samplerLinearClamp, input.texCoord).rgb;

#ifdef TONE_MAPPNIG_UNCHARTED2
    return float4(Uncharted2ToneMapping(color, cb_postProcessExposure, cb_postProcessGamma), 1.f);
#elif defined(TONE_MAPPNIG_REINHARD)
    return float4(ReinhardToneMapping(color, cb_postProcessExposure, cb_postProcessGamma), 1.f);
#elif defined(TONE_MAPPNIG_LUMA_BASED_REINHARD)
    return float4 (LumaBasedReinhardToneMapping(color, cb_postProcessExposure, cb_postProcessGamma), 1.f);
#elif defined(TONE_MAPPNIG_WHITE_PRESERVING_REINHARD)
    return float4(WhitePreservingReinhardToneMapping(color, cb_postProcessExposure, cb_postProcessGamma), 1.f);
#elif defined(TONE_MAPPNIG_ROMB_DA_HOUSE)
    return float4(RomBinDaHouseToneMapping(color, cb_postProcessExposure, cb_postProcessGamma), 1.f);
#elif defined(TONE_MAPPNIG_FILMIC)
    return float4(FilmicToneMapping(color, cb_postProcessExposure, cb_postProcessGamma), 1.f);
#else //defined(TONE_MAPPNIG_LINEAR)
    return float4(LinearToneMapping(color, cb_postProcessExposure, cb_postProcessGamma), 1.f);
#endif

}