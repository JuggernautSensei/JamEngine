#include "pch.h"

#include "CompiledShaders.h"
#include "ShaderCollection.h"

#include "ShaderCompiler.h"
#include "ShaderProgram.h"

namespace
{

struct ShaderProgramManagerState
{
    ShaderProgramManagerState();

    // vs
    jam::ComPtr<ID3DBlob> pbrVS                                                = nullptr;
    jam::ComPtr<ID3DBlob> screenSpaceEffectVS                                  = nullptr;
    jam::ComPtr<ID3DBlob> shadowMappingCasterShaderVS                          = nullptr;
    jam::ComPtr<ID3DBlob> omniDirectionalAndCascadeShadowMappingCasterShaderVS = nullptr;
    jam::ComPtr<ID3DBlob> skyboxShaderVS                                       = nullptr;

    // ps
    jam::ComPtr<ID3DBlob> bloomCombineFilterPS                 = nullptr;
    jam::ComPtr<ID3DBlob> bloomDownFilterPS                    = nullptr;
    jam::ComPtr<ID3DBlob> bloomUpFilterPS                      = nullptr;
    jam::ComPtr<ID3DBlob> fogPS                                = nullptr;
    jam::ComPtr<ID3DBlob> samplingPS                           = nullptr;
    jam::ComPtr<ID3DBlob> toneMappingUncharted2PS              = nullptr;
    jam::ComPtr<ID3DBlob> toneMappingReinhardPS                = nullptr;
    jam::ComPtr<ID3DBlob> toneMappingWhitePreservingReinhardPS = nullptr;
    jam::ComPtr<ID3DBlob> toneMappingLumaBasedReinhardPS       = nullptr;
    jam::ComPtr<ID3DBlob> toneMappingRombDaHousePS             = nullptr;
    jam::ComPtr<ID3DBlob> toneMappingFilmicPS                  = nullptr;
    jam::ComPtr<ID3DBlob> toneMappingLinearPS                  = nullptr;
    jam::ComPtr<ID3DBlob> fxaaQuality0PS                       = nullptr;
    jam::ComPtr<ID3DBlob> fxaaQuality1PS                       = nullptr;
    jam::ComPtr<ID3DBlob> fxaaQuality2PS                       = nullptr;
    jam::ComPtr<ID3DBlob> fxaaQuality3PS                       = nullptr;
    jam::ComPtr<ID3DBlob> fxaaQuality4PS                       = nullptr;
    jam::ComPtr<ID3DBlob> fxaaQuality5PS                       = nullptr;

    jam::ComPtr<ID3DBlob> lightVolumePS                                    = nullptr;
    jam::ComPtr<ID3DBlob> pbrForwardPS                                     = nullptr;
    jam::ComPtr<ID3DBlob> pbrGBufferPS                                     = nullptr;
    jam::ComPtr<ID3DBlob> pbrLightingPS                                    = nullptr;
    jam::ComPtr<ID3DBlob> cascadeShadowMappingAndShadowMappingCastShaderPS = nullptr;
    jam::ComPtr<ID3DBlob> omniShadowMappingShadowCasterShaderPS            = nullptr;
    jam::ComPtr<ID3DBlob> skyboxShaderPS                                   = nullptr;
    jam::ComPtr<ID3DBlob> ssaoBlurHorizontalPS                             = nullptr;
    jam::ComPtr<ID3DBlob> ssaoBlurVerticalPS                               = nullptr;
    jam::ComPtr<ID3DBlob> ssao16PSSize                                     = nullptr;
    jam::ComPtr<ID3DBlob> ssao32PSSize                                     = nullptr;
    jam::ComPtr<ID3DBlob> ssao64PSSize                                     = nullptr;

    // gs
    jam::ComPtr<ID3DBlob> omniDirectionalShaderMappingCasterShaderGS = nullptr;
    jam::ComPtr<ID3DBlob> cascadeShaderMappingCasterShaderGSSize     = nullptr;

} g_shaderState;

ShaderProgramManagerState::ShaderProgramManagerState()
{
    using namespace jam;
    ShaderCompiler compiler;

    compiler.LoadCSO(k_pbrVS, k_pbrVSSize);
    compiler.GetCompiledShader(pbrVS.GetAddressOf());

    compiler.LoadCSO(k_screenSpaceEffectVS, k_screenSpaceEffectVSSize);
    compiler.GetCompiledShader(screenSpaceEffectVS.GetAddressOf());

    compiler.LoadCSO(k_shadowMappingCasterShaderVS, k_shadowMappingCasterShaderVSSize);
    compiler.GetCompiledShader(shadowMappingCasterShaderVS.GetAddressOf());

    compiler.LoadCSO(k_omniDirectionalAndCascadeShadowMappingCasterShaderVS, k_omniDirectionalAndCascadeShadowMappingCasterShaderVSSize);
    compiler.GetCompiledShader(omniDirectionalAndCascadeShadowMappingCasterShaderVS.GetAddressOf());

    compiler.LoadCSO(k_skyboxShaderVS, k_skyboxShaderVSSize);
    compiler.GetCompiledShader(skyboxShaderVS.GetAddressOf());

    // ps
    compiler.LoadCSO(k_bloomCombineFilterPS, k_bloomCombineFilterPSSize);
    compiler.GetCompiledShader(bloomCombineFilterPS.GetAddressOf());

    compiler.LoadCSO(k_bloomDownFilterPS, k_bloomDownFilterPSSize);
    compiler.GetCompiledShader(bloomDownFilterPS.GetAddressOf());

    compiler.LoadCSO(k_bloomUpFilterPS, k_bloomUpFilterPSSize);
    compiler.GetCompiledShader(bloomUpFilterPS.GetAddressOf());

    compiler.LoadCSO(k_fogPS, k_fogPSSize);
    compiler.GetCompiledShader(fogPS.GetAddressOf());

    compiler.LoadCSO(k_samplingPS, k_samplingPSSize);
    compiler.GetCompiledShader(samplingPS.GetAddressOf());

    compiler.LoadCSO(k_toneMappingUncharted2PS, k_toneMappingUncharted2PSSize);
    compiler.GetCompiledShader(toneMappingUncharted2PS.GetAddressOf());

    compiler.LoadCSO(k_toneMappingReinhardPS, k_toneMappingReinhardPSSize);
    compiler.GetCompiledShader(toneMappingReinhardPS.GetAddressOf());

    compiler.LoadCSO(k_toneMappingWhitePreservingReinhardPS, k_toneMappingWhitePreservingReinhardPSSize);
    compiler.GetCompiledShader(toneMappingWhitePreservingReinhardPS.GetAddressOf());

    compiler.LoadCSO(k_toneMappingLumaBasedReinhardPS, k_toneMappingLumaBasedReinhardPSSize);
    compiler.GetCompiledShader(toneMappingLumaBasedReinhardPS.GetAddressOf());

    compiler.LoadCSO(k_toneMappingRombDaHousePS, k_toneMappingRombDaHousePSSize);
    compiler.GetCompiledShader(toneMappingRombDaHousePS.GetAddressOf());

    compiler.LoadCSO(k_toneMappingFilmicPS, k_toneMappingFilmicPSSize);
    compiler.GetCompiledShader(toneMappingFilmicPS.GetAddressOf());

    compiler.LoadCSO(k_toneMappingLinearPS, k_toneMappingLinearPSSize);
    compiler.GetCompiledShader(toneMappingLinearPS.GetAddressOf());

    compiler.LoadCSO(k_fxaaQuality0PS, k_fxaaQuality0PSSize);
    compiler.GetCompiledShader(fxaaQuality0PS.GetAddressOf());

    compiler.LoadCSO(k_fxaaQuality1PS, k_fxaaQuality1PSSize);
    compiler.GetCompiledShader(fxaaQuality1PS.GetAddressOf());

    compiler.LoadCSO(k_fxaaQuality2PS, k_fxaaQuality2PSSize);
    compiler.GetCompiledShader(fxaaQuality2PS.GetAddressOf());

    compiler.LoadCSO(k_fxaaQuality3PS, k_fxaaQuality3PSSize);
    compiler.GetCompiledShader(fxaaQuality3PS.GetAddressOf());

    compiler.LoadCSO(k_fxaaQuality4PS, k_fxaaQuality4PSSize);
    compiler.GetCompiledShader(fxaaQuality4PS.GetAddressOf());

    compiler.LoadCSO(k_fxaaQuality5PS, k_fxaaQuality5PSSize);
    compiler.GetCompiledShader(fxaaQuality5PS.GetAddressOf());

    compiler.LoadCSO(k_lightVolumePS, k_lightVolumePSSize);
    compiler.GetCompiledShader(lightVolumePS.GetAddressOf());

    compiler.LoadCSO(k_pbrForwardPS, k_pbrForwardPSSize);
    compiler.GetCompiledShader(pbrForwardPS.GetAddressOf());

    compiler.LoadCSO(k_pbrGBufferPS, k_pbrGBufferPSSize);
    compiler.GetCompiledShader(pbrGBufferPS.GetAddressOf());

    compiler.LoadCSO(k_pbrLightingPS, k_pbrLightingPSSize);
    compiler.GetCompiledShader(pbrLightingPS.GetAddressOf());

    compiler.LoadCSO(k_cascadeShadowMappingAndShadowMappingCastShaderPS, k_cascadeShadowMappingAndShadowMappingCastShaderPSSize);
    compiler.GetCompiledShader(cascadeShadowMappingAndShadowMappingCastShaderPS.GetAddressOf());

    compiler.LoadCSO(k_omniShadowoMappingShadowCasterShaderPS, k_omniShadowoMappingShadowCasterShaderPSSize);
    compiler.GetCompiledShader(omniShadowMappingShadowCasterShaderPS.GetAddressOf());

    compiler.LoadCSO(k_skyboxShaderPS, k_skyboxShaderPSSize);
    compiler.GetCompiledShader(skyboxShaderPS.GetAddressOf());

    compiler.LoadCSO(k_ssaoBlurHorizontalPS, k_ssaoBlurHorizontalPSSize);
    compiler.GetCompiledShader(ssaoBlurHorizontalPS.GetAddressOf());

    compiler.LoadCSO(k_ssaoBlurVerticalPS, k_ssaoBlurVerticalPSSize);
    compiler.GetCompiledShader(ssaoBlurVerticalPS.GetAddressOf());

    compiler.LoadCSO(k_ssao16SamplePS, k_ssao16SamplePSSize);
    compiler.GetCompiledShader(ssao16PSSize.GetAddressOf());

    compiler.LoadCSO(k_ssao32SamplePS, k_ssao32SamplePSSize);
    compiler.GetCompiledShader(ssao32PSSize.GetAddressOf());

    compiler.LoadCSO(k_ssao64SamplePS, k_ssao64SamplePSSize);
    compiler.GetCompiledShader(ssao64PSSize.GetAddressOf());

    // gs
    compiler.LoadCSO(k_omniDirectionalShaderMappingCasterShaderGS, k_omniDirectionalShaderMappingCasterShaderGSSize);
    compiler.GetCompiledShader(omniDirectionalShaderMappingCasterShaderGS.GetAddressOf());

    compiler.LoadCSO(k_cascadeShaderMappingCasterShaderGS, k_cascadeShaderMappingCasterShaderGSSize);
    compiler.GetCompiledShader(cascadeShaderMappingCasterShaderGSSize.GetAddressOf());
}

}   // namespace

namespace jam
{

ShaderProgram ShaderCollection::GetPBRGBufferShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.pbrVS.Get(), g_shaderState.pbrGBufferPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetPBRForwardShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.pbrVS.Get(), g_shaderState.pbrForwardPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetPBRLightingShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.pbrVS.Get(), g_shaderState.pbrLightingPS.Get());
        return shader;
    }();
    return s_shader;
}
ShaderProgram ShaderCollection::GetLightVolumeShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.lightVolumePS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetSkyboxShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.skyboxShaderVS.Get(), g_shaderState.skyboxShaderPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetFullScreenQuadShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.samplingPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetBloomCombineFilterShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.bloomCombineFilterPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetBloomDownFilterShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.bloomDownFilterPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetBloomUpFilterShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.bloomUpFilterPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetFogFilterShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.fogPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetSamplingFilterShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.samplingPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetToneMappingFilterUncharted2Shader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.toneMappingUncharted2PS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetToneMappingFilterReinhardShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.toneMappingReinhardPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetToneMappingFilterWhitePreservingReinhardShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.toneMappingWhitePreservingReinhardPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetToneMappingFilterLumaBasedReinhardShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.toneMappingLumaBasedReinhardPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetToneMappingFilterRombDaHouseShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.toneMappingRombDaHousePS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetToneMappingFilterFilmicShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.toneMappingFilmicPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetToneMappingFilterLinearShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.toneMappingLinearPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetFXAAFilterQuality0Shader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.fxaaQuality0PS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetFXAAFilterQuality1Shader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.fxaaQuality1PS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetFXAAFilterQuality2Shader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.fxaaQuality2PS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetFXAAFilterQuality3Shader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.fxaaQuality3PS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetFXAAFilterQuality4Shader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.fxaaQuality4PS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetFXAAFilterQuality5Shader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.fxaaQuality5PS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetCascadeShadowMappingShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.omniDirectionalAndCascadeShadowMappingCasterShaderVS.Get(), g_shaderState.cascadeShadowMappingAndShadowMappingCastShaderPS.Get(), g_shaderState.cascadeShaderMappingCasterShaderGSSize.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetOmniShadowMappingShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.omniDirectionalAndCascadeShadowMappingCasterShaderVS.Get(), g_shaderState.omniShadowMappingShadowCasterShaderPS.Get(), g_shaderState.omniDirectionalShaderMappingCasterShaderGS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetShadowMappingShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.shadowMappingCasterShaderVS.Get(), g_shaderState.cascadeShadowMappingAndShadowMappingCastShaderPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetSSAOBlurHorizontalShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.ssaoBlurHorizontalPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetSSAOBlurVerticalShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.ssaoBlurVerticalPS.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetSSAO16SampleShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.ssao16PSSize.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetSSAO32SampleShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.ssao32PSSize.Get());
        return shader;
    }();
    return s_shader;
}

ShaderProgram ShaderCollection::GetSSAO64SampleShader()
{
    static ShaderProgram s_shader = []
    {
        ShaderProgram shader;
        shader.Initialize(g_shaderState.screenSpaceEffectVS.Get(), g_shaderState.ssao64PSSize.Get());
        return shader;
    }();
    return s_shader;
}

}   // namespace jam