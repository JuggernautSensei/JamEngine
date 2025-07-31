#pragma once
#include "ShaderProgram.h"

namespace jam
{

class ShaderCollection
{
public:
    ShaderCollection() = delete;

    static ShaderProgram GetPBRGBufferShader();
    static ShaderProgram GetPBRForwardShader();
    static ShaderProgram GetPBRLightingShader();
    static ShaderProgram GetLightVolumeShader();
    static ShaderProgram GetSkyboxShader();
    static ShaderProgram GetFullScreenQuadShader();

    static ShaderProgram GetBloomCombineFilterShader();
    static ShaderProgram GetBloomDownFilterShader();
    static ShaderProgram GetBloomUpFilterShader();
    static ShaderProgram GetFogFilterShader();
    static ShaderProgram GetSamplingFilterShader();
    static ShaderProgram GetToneMappingFilterUncharted2Shader();
    static ShaderProgram GetToneMappingFilterReinhardShader();
    static ShaderProgram GetToneMappingFilterWhitePreservingReinhardShader();
    static ShaderProgram GetToneMappingFilterLumaBasedReinhardShader();
    static ShaderProgram GetToneMappingFilterRombDaHouseShader();
    static ShaderProgram GetToneMappingFilterFilmicShader();
    static ShaderProgram GetToneMappingFilterLinearShader();
    static ShaderProgram GetFXAAFilterQuality0Shader();
    static ShaderProgram GetFXAAFilterQuality1Shader();
    static ShaderProgram GetFXAAFilterQuality2Shader();
    static ShaderProgram GetFXAAFilterQuality3Shader();
    static ShaderProgram GetFXAAFilterQuality4Shader();
    static ShaderProgram GetFXAAFilterQuality5Shader();

    static ShaderProgram GetCascadeShadowMappingShader();
    static ShaderProgram GetOmniShadowMappingShader();
    static ShaderProgram GetShadowMappingShader();

    static ShaderProgram GetSSAOBlurHorizontalShader();
    static ShaderProgram GetSSAOBlurVerticalShader();
    static ShaderProgram GetSSAO16SampleShader();
    static ShaderProgram GetSSAO32SampleShader();
    static ShaderProgram GetSSAO64SampleShader();
};

}   // namespace jam
