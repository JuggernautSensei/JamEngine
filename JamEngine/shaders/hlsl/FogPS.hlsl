#include "ShaderBase.hlsl"

float4 PSMain(FULLSCREEN_PS_INPUT _input) : SV_TARGET
{
    float3 texColor = t_input0.Sample(s_linearClamp, _input.texCoord0).rgb;
    float depth = t_depth.Sample(s_pointClamp, _input.texCoord0).r;

    float3 posNdc;
    posNdc.x = _input.texCoord0.x * 2.f - 1.f;
    posNdc.y = 1.f - _input.texCoord0.y * 2.f;
    posNdc.z = depth;

    float4 unprojPos = mul(cb_viewProjInv, float4(posNdc, 1.f));
    float3 posW = unprojPos.xyz / unprojPos.w;

    float dist = distance(cb_eyePos, posW);

    float fogY = posW.y;
    float eyeY = cb_eyePos.y;
    float invFoxY = 1.f / max(fogY, k_epsilon);
    float fogFactor = invFoxY * cb_fogDensity / cb_fogHeightAttenuation
                    * exp2(max(-127.f, -eyeY * cb_fogHeightAttenuation))
                    * (1.f - exp2(clamp(-dist * fogY * cb_fogHeightAttenuation, -127.f, 127.f)));

    fogFactor = clamp(fogFactor, 0.f, 1.f);
    fogFactor = CalcSmoothAttenuation(fogFactor, cb_fogFallOffStart, cb_fogFallOffEnd);
    return float4(lerp(texColor, cb_fogColor.rgb, fogFactor), 1.f);
}
