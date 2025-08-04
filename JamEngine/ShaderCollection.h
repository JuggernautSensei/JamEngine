#pragma once
#include "ShaderProgram.h"

namespace jam
{

class ShaderCollection
{
public:
    ShaderCollection() = delete;

    static ShaderProgram PBRGBufferShader();
    static ShaderProgram PBRForwardShader();
    static ShaderProgram PBRLightingShader();
    static ShaderProgram LightVolumeShader();
    static ShaderProgram SkyboxShader();
    static ShaderProgram FullScreenQuadShader();

    static ShaderProgram BloomCombineFilterShader();
    static ShaderProgram BloomDownFilterShader();
    static ShaderProgram BloomUpFilterShader();
    static ShaderProgram FogFilterShader();
    static ShaderProgram SamplingFilterShader();
    static ShaderProgram ToneMappingFilterUncharted2Shader();
    static ShaderProgram ToneMappingFilterReinhardShader();
    static ShaderProgram ToneMappingFilterWhitePreservingReinhardShader();
    static ShaderProgram ToneMappingFilterLumaBasedReinhardShader();
    static ShaderProgram ToneMappingFilterRombDaHouseShader();
    static ShaderProgram ToneMappingFilterFilmicShader();
    static ShaderProgram ToneMappingFilterLinearShader();
    static ShaderProgram FXAAFilterQuality0Shader();
    static ShaderProgram FXAAFilterQuality1Shader();
    static ShaderProgram FXAAFilterQuality2Shader();
    static ShaderProgram FXAAFilterQuality3Shader();
    static ShaderProgram FXAAFilterQuality4Shader();
    static ShaderProgram FXAAFilterQuality5Shader();

    static ShaderProgram CascadeShadowMappingShader();
    static ShaderProgram OmniShadowMappingShader();
    static ShaderProgram ShadowMappingShader();

    static ShaderProgram SSAOBlurHorizontalShader();
    static ShaderProgram SSAOBlurVerticalShader();
    static ShaderProgram SSAO16SampleShader();
    static ShaderProgram SSAO32SampleShader();
    static ShaderProgram SSAO64SampleShader();
};

}   // namespace jam
