#include "pch.h"

#include "CompiledShaders.h"
#include "ShaderRegistry.h"

#include "ShaderCompiler.h"

namespace
{
//
//struct ProgramManagerState
//{
//    ProgramManagerState();
//
//    // vs
//    jam::ComPtr<ID3DBlob> pbrVS                                                = nullptr;
//    jam::ComPtr<ID3DBlob> screenSpaceEffectVS                                  = nullptr;
//    jam::ComPtr<ID3DBlob> shadowMappingCasterShaderVS                          = nullptr;
//    jam::ComPtr<ID3DBlob> omniDirectionalAndCascadeShadowMappingCasterShaderVS = nullptr;
//    jam::ComPtr<ID3DBlob> skyboxShaderVS                                       = nullptr;
//
//    // ps
//    jam::ComPtr<ID3DBlob> bloomCombineFilterPS                 = nullptr;
//    jam::ComPtr<ID3DBlob> bloomDownFilterPS                    = nullptr;
//    jam::ComPtr<ID3DBlob> bloomUpFilterPS                      = nullptr;
//    jam::ComPtr<ID3DBlob> fogPS                                = nullptr;
//    jam::ComPtr<ID3DBlob> samplingPS                           = nullptr;
//    jam::ComPtr<ID3DBlob> toneMappingUncharted2PS              = nullptr;
//    jam::ComPtr<ID3DBlob> toneMappingReinhardPS                = nullptr;
//    jam::ComPtr<ID3DBlob> toneMappingWhitePreservingReinhardPS = nullptr;
//    jam::ComPtr<ID3DBlob> toneMappingLumaBasedReinhardPS       = nullptr;
//    jam::ComPtr<ID3DBlob> toneMappingRombDaHousePS             = nullptr;
//    jam::ComPtr<ID3DBlob> toneMappingFilmicPS                  = nullptr;
//    jam::ComPtr<ID3DBlob> toneMappingLinearPS                  = nullptr;
//    jam::ComPtr<ID3DBlob> fxaaQuality0PS                       = nullptr;
//    jam::ComPtr<ID3DBlob> fxaaQuality1PS                       = nullptr;
//    jam::ComPtr<ID3DBlob> fxaaQuality2PS                       = nullptr;
//    jam::ComPtr<ID3DBlob> fxaaQuality3PS                       = nullptr;
//    jam::ComPtr<ID3DBlob> fxaaQuality4PS                       = nullptr;
//    jam::ComPtr<ID3DBlob> fxaaQuality5PS                       = nullptr;
//
//    jam::ComPtr<ID3DBlob> lightVolumePS                                    = nullptr;
//    jam::ComPtr<ID3DBlob> pbrForwardPS                                     = nullptr;
//    jam::ComPtr<ID3DBlob> pbrGBufferPS                                     = nullptr;
//    jam::ComPtr<ID3DBlob> pbrLightingPS                                    = nullptr;
//    jam::ComPtr<ID3DBlob> cascadeShadowMappingAndShadowMappingCastShaderPS = nullptr;
//    jam::ComPtr<ID3DBlob> omniShadowMappingShadowCasterShaderPS            = nullptr;
//    jam::ComPtr<ID3DBlob> skyboxShaderPS                                   = nullptr;
//    jam::ComPtr<ID3DBlob> ssaoBlurHorizontalPS                             = nullptr;
//    jam::ComPtr<ID3DBlob> ssaoBlurVerticalPS                               = nullptr;
//    jam::ComPtr<ID3DBlob> ssao16PSSize                                     = nullptr;
//    jam::ComPtr<ID3DBlob> ssao32PSSize                                     = nullptr;
//    jam::ComPtr<ID3DBlob> ssao64PSSize                                     = nullptr;
//
//    // gs
//    jam::ComPtr<ID3DBlob> omniDirectionalShaderMappingCasterShaderGS = nullptr;
//    jam::ComPtr<ID3DBlob> cascadeShaderMappingCasterShaderGSSize     = nullptr;
//
//} g_shaderState;
//
//ProgramManagerState::ProgramManagerState()
//{
//    using namespace jam;
//    ShaderCompiler compiler;
//
//    compiler.LoadCSO(k_pbrVS, k_pbrVSSize);
//    compiler.GetCompiledShader(pbrVS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_screenSpaceEffectVS, k_screenSpaceEffectVSSize);
//    compiler.GetCompiledShader(screenSpaceEffectVS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_shadowMappingCasterShaderVS, k_shadowMappingCasterShaderVSSize);
//    compiler.GetCompiledShader(shadowMappingCasterShaderVS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_omniDirectionalAndCascadeShadowMappingCasterShaderVS, k_omniDirectionalAndCascadeShadowMappingCasterShaderVSSize);
//    compiler.GetCompiledShader(omniDirectionalAndCascadeShadowMappingCasterShaderVS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_skyboxShaderVS, k_skyboxShaderVSSize);
//    compiler.GetCompiledShader(skyboxShaderVS.ReleaseAndGetAddressOf());
//
//    // ps
//    compiler.LoadCSO(k_bloomCombineFilterPS, k_bloomCombineFilterPSSize);
//    compiler.GetCompiledShader(bloomCombineFilterPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_bloomDownFilterPS, k_bloomDownFilterPSSize);
//    compiler.GetCompiledShader(bloomDownFilterPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_bloomUpFilterPS, k_bloomUpFilterPSSize);
//    compiler.GetCompiledShader(bloomUpFilterPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_fogPS, k_fogPSSize);
//    compiler.GetCompiledShader(fogPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_samplingPS, k_samplingPSSize);
//    compiler.GetCompiledShader(samplingPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_toneMappingUncharted2PS, k_toneMappingUncharted2PSSize);
//    compiler.GetCompiledShader(toneMappingUncharted2PS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_toneMappingReinhardPS, k_toneMappingReinhardPSSize);
//    compiler.GetCompiledShader(toneMappingReinhardPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_toneMappingWhitePreservingReinhardPS, k_toneMappingWhitePreservingReinhardPSSize);
//    compiler.GetCompiledShader(toneMappingWhitePreservingReinhardPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_toneMappingLumaBasedReinhardPS, k_toneMappingLumaBasedReinhardPSSize);
//    compiler.GetCompiledShader(toneMappingLumaBasedReinhardPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_toneMappingRombDaHousePS, k_toneMappingRombDaHousePSSize);
//    compiler.GetCompiledShader(toneMappingRombDaHousePS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_toneMappingFilmicPS, k_toneMappingFilmicPSSize);
//    compiler.GetCompiledShader(toneMappingFilmicPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_toneMappingLinearPS, k_toneMappingLinearPSSize);
//    compiler.GetCompiledShader(toneMappingLinearPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_fxaaQuality0PS, k_fxaaQuality0PSSize);
//    compiler.GetCompiledShader(fxaaQuality0PS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_fxaaQuality1PS, k_fxaaQuality1PSSize);
//    compiler.GetCompiledShader(fxaaQuality1PS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_fxaaQuality2PS, k_fxaaQuality2PSSize);
//    compiler.GetCompiledShader(fxaaQuality2PS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_fxaaQuality3PS, k_fxaaQuality3PSSize);
//    compiler.GetCompiledShader(fxaaQuality3PS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_fxaaQuality4PS, k_fxaaQuality4PSSize);
//    compiler.GetCompiledShader(fxaaQuality4PS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_fxaaQuality5PS, k_fxaaQuality5PSSize);
//    compiler.GetCompiledShader(fxaaQuality5PS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_lightVolumePS, k_lightVolumePSSize);
//    compiler.GetCompiledShader(lightVolumePS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_pbrForwardPS, k_pbrForwardPSSize);
//    compiler.GetCompiledShader(pbrForwardPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_pbrGBufferPS, k_pbrGBufferPSSize);
//    compiler.GetCompiledShader(pbrGBufferPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_pbrLightingPS, k_pbrLightingPSSize);
//    compiler.GetCompiledShader(pbrLightingPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_cascadeShadowMappingAndShadowMappingCastShaderPS, k_cascadeShadowMappingAndShadowMappingCastShaderPSSize);
//    compiler.GetCompiledShader(cascadeShadowMappingAndShadowMappingCastShaderPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_omniShadowoMappingShadowCasterShaderPS, k_omniShadowoMappingShadowCasterShaderPSSize);
//    compiler.GetCompiledShader(omniShadowMappingShadowCasterShaderPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_skyboxShaderPS, k_skyboxShaderPSSize);
//    compiler.GetCompiledShader(skyboxShaderPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_ssaoBlurHorizontalPS, k_ssaoBlurHorizontalPSSize);
//    compiler.GetCompiledShader(ssaoBlurHorizontalPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_ssaoBlurVerticalPS, k_ssaoBlurVerticalPSSize);
//    compiler.GetCompiledShader(ssaoBlurVerticalPS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_ssao16SamplePS, k_ssao16SamplePSSize);
//    compiler.GetCompiledShader(ssao16PSSize.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_ssao32SamplePS, k_ssao32SamplePSSize);
//    compiler.GetCompiledShader(ssao32PSSize.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_ssao64SamplePS, k_ssao64SamplePSSize);
//    compiler.GetCompiledShader(ssao64PSSize.ReleaseAndGetAddressOf());
//
//    // gs
//    compiler.LoadCSO(k_omniDirectionalShaderMappingCasterShaderGS, k_omniDirectionalShaderMappingCasterShaderGSSize);
//    compiler.GetCompiledShader(omniDirectionalShaderMappingCasterShaderGS.ReleaseAndGetAddressOf());
//
//    compiler.LoadCSO(k_cascadeShaderMappingCasterShaderGS, k_cascadeShaderMappingCasterShaderGSSize);
//    compiler.GetCompiledShader(cascadeShaderMappingCasterShaderGSSize.ReleaseAndGetAddressOf());
//}
//
//}   // namespace
//
//namespace jam
//{
//
//Program ShaderRegistry::PBRGBufferShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.pbrVS.GetResourceRef(), g_shaderState.pbrGBufferPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::PBRForwardShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.pbrVS.GetResourceRef(), g_shaderState.pbrForwardPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::PBRLightingShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.pbrVS.GetResourceRef(), g_shaderState.pbrLightingPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//Program ShaderRegistry::LightVolumeShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.lightVolumePS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::SkyboxShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.skyboxShaderVS.GetResourceRef(), g_shaderState.skyboxShaderPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::FullScreenQuadShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.samplingPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::BloomCombineFilterShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.bloomCombineFilterPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::BloomDownFilterShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.bloomDownFilterPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::BloomUpFilterShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.bloomUpFilterPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::FogFilterShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.fogPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::SamplingFilterShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.samplingPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::ToneMappingFilterUncharted2Shader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.toneMappingUncharted2PS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::ToneMappingFilterReinhardShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.toneMappingReinhardPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::ToneMappingFilterWhitePreservingReinhardShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.toneMappingWhitePreservingReinhardPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::ToneMappingFilterLumaBasedReinhardShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.toneMappingLumaBasedReinhardPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::ToneMappingFilterRombDaHouseShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.toneMappingRombDaHousePS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::ToneMappingFilterFilmicShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.toneMappingFilmicPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::ToneMappingFilterLinearShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.toneMappingLinearPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::FXAAFilterQuality0Shader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.fxaaQuality0PS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::FXAAFilterQuality1Shader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.fxaaQuality1PS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::FXAAFilterQuality2Shader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.fxaaQuality2PS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::FXAAFilterQuality3Shader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.fxaaQuality3PS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::FXAAFilterQuality4Shader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.fxaaQuality4PS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::FXAAFilterQuality5Shader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.fxaaQuality5PS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::CascadeShadowMappingShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.omniDirectionalAndCascadeShadowMappingCasterShaderVS.GetResourceRef(), g_shaderState.cascadeShadowMappingAndShadowMappingCastShaderPS.GetResourceRef(), g_shaderState.cascadeShaderMappingCasterShaderGSSize.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::OmniShadowMappingShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.omniDirectionalAndCascadeShadowMappingCasterShaderVS.GetResourceRef(), g_shaderState.omniShadowMappingShadowCasterShaderPS.GetResourceRef(), g_shaderState.omniDirectionalShaderMappingCasterShaderGS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::ShadowMappingShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.shadowMappingCasterShaderVS.GetResourceRef(), g_shaderState.cascadeShadowMappingAndShadowMappingCastShaderPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::SSAOBlurHorizontalShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.ssaoBlurHorizontalPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::SSAOBlurVerticalShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.ssaoBlurVerticalPS.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::SSAO16SampleShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.ssao16PSSize.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::SSAO32SampleShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.ssao32PSSize.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}
//
//Program ShaderRegistry::SSAO64SampleShader()
//{
//    static Program s_shader = []
//    {
//        Program shader;
//        shader.Init(g_shaderState.screenSpaceEffectVS.GetResourceRef(), g_shaderState.ssao64PSSize.GetResourceRef());
//        return shader;
//    }();
//    return s_shader;
//}

}   // namespace jam