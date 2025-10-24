#include "pch.h"

#include "BasicRenderer.h"

#include "BasicRendererBase.h"
#include "CullingSystem.h"
#include "EmbeddedShaderGlue.h"
#include "Environment.h"
#include "Graphics.h"
#include "Light.h"
#include "Memory.h"
#include "MeshFactory.h"
#include "Profile.h"
#include "RenderGraph.h"
#include "Shaders/PrecompiledShaders/PrecompiledJamShader.h"
#include "Timer.h"

namespace jam
{

struct BasicRendererContext
{
    void UploadTransform(const Mat4& _world) const;
    void UploadMaterial(const Material& _material) const;
    void UploadPointLight(const PointLight& _light, const Transform& _transform) const;
    void UploadSpotLight(const SpotLight& _light, const Transform& _transform) const;

    void BakeSSAOKernel() const;
    void BakeSSAONoise();

    ShaderHandle pbrVS;
    ShaderHandle fullscreenVS;
    ShaderHandle pbrLightVolumeVS;
    ShaderHandle pbrEnvironmentLIghtVolumeVS;
    ShaderHandle csmVS;
    ShaderHandle osmVS;
    ShaderHandle shadowMappingVS;
    ShaderHandle skyboxVS;

    ShaderHandle csmLevel2GS;
    ShaderHandle csmLevel3GS;
    ShaderHandle csmLevel4GS;
    ShaderHandle osmGS;

    ShaderHandle pbrGBufferPS;
    ShaderHandle pbrLightingPS;
    ShaderHandle pbrLightVolumePS;
    ShaderHandle pbrEnvironmentLightVolumePS;
    ShaderHandle csmPS;
    ShaderHandle osmPS;
    ShaderHandle shadowMappingPS;
    ShaderHandle skyboxPS;
    ShaderHandle ssaoPS;
    ShaderHandle ssaoBlurXPS;
    ShaderHandle ssaoBlurYPS;
    ShaderHandle bloomXPS;
    ShaderHandle bloomYPS;
    ShaderHandle bloomCombinePS;
    ShaderHandle toneMappingUncharted2PS;
    ShaderHandle toneMappingLinearPS;
    ShaderHandle toneMappingReinhardPS;
    ShaderHandle toneMappingLumaBasedReinhardPS;
    ShaderHandle toneMappingWhitePreservingReinhardPS;
    ShaderHandle toneMappingRombDaHousePS;
    ShaderHandle toneMappingFilmicPS;
    ShaderHandle fxaaVeryLowPS;
    ShaderHandle fxaaLowPS;
    ShaderHandle fxaaMediumPS;
    ShaderHandle fxaaHighPS;
    ShaderHandle fxaaVeryHighPS;
    ShaderHandle fxaaUltraPS;
    ShaderHandle fogPS;
    ShaderHandle passThroughPS;

    ProgramHandle pbrGBufferProgram;
    ProgramHandle pbrLightingProgram;
    ProgramHandle pbrLightVolumeProgram;
    ProgramHandle pbrEnvironmentLightVolumeProgram;
    ProgramHandle csmLevel2CasterProgram;
    ProgramHandle csmLevel3CasterProgram;
    ProgramHandle csmLevel4CasterProgram;
    ProgramHandle osmCasterProgram;
    ProgramHandle shadowCasterProgram;
    ProgramHandle skyboxProgram;
    ProgramHandle ssaoProgram;
    ProgramHandle ssaoBlurXProgram;
    ProgramHandle ssaoBlurYProgram;

    ProgramHandle bloomXProgram;
    ProgramHandle bloomYProgram;
    ProgramHandle bloomCombineProgram;
    ProgramHandle toneMappingUncharted2Program;
    ProgramHandle toneMappingLinearProgram;
    ProgramHandle toneMappingReinhardProgram;
    ProgramHandle toneMappingLumaBasedReinhardProgram;
    ProgramHandle toneMappingWhitePreservingReinhardProgram;
    ProgramHandle toneMappingRombDaHouseProgram;
    ProgramHandle toneMappingFilmicProgram;
    ProgramHandle fxaaVeryLowProgram;
    ProgramHandle fxaaLowProgram;
    ProgramHandle fxaaMediumProgram;
    ProgramHandle fxaaHighProgram;
    ProgramHandle fxaaVeryHighProgram;
    ProgramHandle fxaaUltraProgram;
    ProgramHandle fogProgram;
    ProgramHandle passThroughProgram;

    ConstantBufferHandle transformCbh;
    ConstantBufferHandle cameraCbh;
    ConstantBufferHandle materialCbh;
    ConstantBufferHandle lightCbh;
    ConstantBufferHandle environmentCbh;
    ConstantBufferHandle shadowCasterCbh;
    ConstantBufferHandle csmCasterCbh;
    ConstantBufferHandle osmCasterCbh;
    ConstantBufferHandle ssaoCbh;

    TextureHandle ssaoNoiseTexture;

    Mesh fullScreenTri;
    Mesh lightVolumeFullScreenTri;
    Mesh lightVolumeUnitSphere;
    Mesh lightVolumeUnitCone;
    Mesh skyboxCube;

    SamplerState linearWrapSampler;
    SamplerState linearClampSampler;
    SamplerState pointWrapSampler;
    SamplerState pointClampSampler;
    SamplerState shadowLinearClampSampler;

    GfxState depthTestState;
    GfxState lightVolumeState;
    GfxState skyboxState;

    CullingSystem cullingSystem;
    Timer         timer;

    const Environment* pCurEnv = nullptr;

    RenderGraph graph;

} g_basicRendererContext;

void BasicRendererContext::BakeSSAOKernel() const
{
    Int32 kernelSize = 0;
    if (pCurEnv->ssaoKernelSize == eSSAOKernel::Kernel4x4)
        kernelSize = 16;
    else if (pCurEnv->ssaoKernelSize == eSSAOKernel::Kernel6x6)
        kernelSize = 36;
    else if (pCurEnv->ssaoKernelSize == eSSAOKernel::Kernel8x8)
        kernelSize = 64;

    CB_SSAO_KERNEL cb;
    for (Int32 i = 0; i < kernelSize; ++i)
    {
        // 반구
        Vec3 sample = Vec3 {
            CreateRandom<float>(-1.f, 1.f),
            CreateRandom<float>(-1.f, 1.f),
            CreateRandom<float>(0.f, 1.f)
        };
        sample.Normalize();
        sample *= CreateRandom<float>(0.f, 1.f);
        float sacle = static_cast<float>(i) / static_cast<float>(kernelSize);
        sacle       = Lerp(0.1f, 1.f, sacle * sacle);   // 가까운 샘플에 더 집중
        sample *= sacle;
        cb.cb_ssaoKernel[i] = sample;
    }

    Graphics::UploadBuffer(ssaoCbh, MemoryView { cb });
}

void BasicRendererContext::BakeSSAONoise()
{
    JAM_DESTROY_TEXTURE(ssaoNoiseTexture);

    constexpr UInt32 noiseSize = 4;

    Vec3 noiseData[noiseSize * noiseSize];
    for (UInt32 i = 0; i < noiseSize * noiseSize; ++i)
    {
        noiseData[i] = Vec3 {
            CreateRandom<float>(-1.f, 1.f),
            CreateRandom<float>(-1.f, 1.f),
            0.f
        };
    }

    MemoryView mem[] = {
        MemoryView { noiseData, sizeof(noiseData) }
    };

    ssaoNoiseTexture = Graphics::CreateTexture2D(noiseSize, noiseSize, eFormat::RGBA16_Float, 1, eTextureCreateFlags_None, mem);
}

void BasicRendererContext::UploadTransform(const Mat4& _world) const
{
    CB_TRANSFORM cb;
    cb.cb_world         = _world.Transpose();
    cb.cb_worldInvTrans = _world.Invert();
    Graphics::UploadBuffer(transformCbh, MemoryView { cb });
}

void BasicRendererContext::UploadMaterial(const Material& _material) const
{
    const MaterialParams& param = _material.GetParams();

    CB_MATERIAL cb          = {};
    cb.cb_ambient           = param.ambientColor;
    cb.cb_diffuse           = param.diffuseColor;
    cb.cb_specular          = param.specularColor;
    cb.cb_shininess         = param.shininess;
    cb.cb_albedo            = param.albedoColor;
    cb.cb_roughness         = param.roughness;
    cb.cb_metallic          = param.metallic;
    cb.cb_emissive          = param.emissiveColor;
    cb.cb_emissiveIntensity = param.emissiveIntensity;
    cb.cb_lightMapIntencity = param.lightMapIntensity;
    cb.cb_aoMapIntencity    = param.aoMapIntensity;
    cb.cb_bumpMapScaling    = param.vertexDisplacementMapScale;

    if (param.albedoMap)
    {
        param.albedoMap->Bind(eShader::PixelShader, t_albedoSlot);
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_ALBEDO;
    }

    if (param.normalMap)
    {
        param.normalMap->Bind(eShader::PixelShader, t_normalSlot);
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_NORMAL;
    }

    if (param.metallicMap)
    {
        param.metallicMap->Bind(eShader::PixelShader, t_metallicSlot);
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_METALLIC;
    }

    if (param.roughnessMap)
    {
        param.roughnessMap->Bind(eShader::PixelShader, t_roughnessSlot);
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_ROUGHNESS;
    }

    if (param.aoMap)
    {
        param.aoMap->Bind(eShader::PixelShader, t_aoSlot);
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_AO;
    }

    if (param.emissiveMap)
    {
        param.emissiveMap->Bind(eShader::PixelShader, t_emissiveSlot);
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_EMISSIVE;
    }

    if (param.vertexDisplacementMap)
    {
        param.vertexDisplacementMap->Bind(eShader::VertexShader, t_bumpMapSlot);
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_VERTEX_DISPLACEMENT_MAP;
    }

    if (param.lightMap)
    {
        param.lightMap->Bind(eShader::PixelShader, t_lightmapSlot);
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_LIGHT_MAP;
    }

    if (param.bFlipYNormalMap)
    {
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_FLIP_Y_NORMAL;
    }

    Graphics::UploadBuffer(materialCbh, MemoryView { cb });
}

void BasicRendererContext::UploadPointLight(const PointLight& _light, const Transform& _transform) const
{
    CB_LIGHT cb                 = {};
    cb.cb_lightType             = JAM_LIGHT_TYPE_POINT;
    cb.cb_lightPos              = _transform.translate;
    cb.cb_lightDir              = _transform.Forward();
    cb.cb_lightRadiance         = _light.radiance;
    cb.cb_lightIntensity        = _light.intensity;
    cb.cb_lightFallOffStart     = _light.fallOffStart;
    cb.cb_lightFallOffEnd       = _light.fallOffEnd;
    cb.cb_lightInnerConeHalfRad = 0;
    cb.cb_lightOuterConeHalfRad = 0;
    cb.cb_bShadowMapping        = _light.bCastShadow ? JAM_TRUE : JAM_FALSE;
    Graphics::UploadBuffer(lightCbh, MemoryView { cb });
}

void BasicRendererContext::UploadSpotLight(const SpotLight& _light, const Transform& _transform) const
{
    CB_LIGHT cb                 = {};
    cb.cb_lightType             = JAM_LIGHT_TYPE_POINT;
    cb.cb_lightPos              = _transform.translate;
    cb.cb_lightDir              = _transform.Forward();
    cb.cb_lightRadiance         = _light.radiance;
    cb.cb_lightIntensity        = _light.intensity;
    cb.cb_lightFallOffStart     = _light.fallOffStart;
    cb.cb_lightFallOffEnd       = _light.fallOffEnd;
    cb.cb_lightInnerConeHalfRad = _light.innerConeHalfRad;
    cb.cb_lightOuterConeHalfRad = _light.outerConeHalfRad;
    cb.cb_bShadowMapping        = _light.bCastShadow ? JAM_TRUE : JAM_FALSE;
    Graphics::UploadBuffer(lightCbh, MemoryView { cb });
}

}   // namespace jam

namespace jam
{

void BasicRenderer::Init()
{
    JAM_PROFILE_SCOPE_MS("RendererSystem::Init");

    // vertex shaders
    {
        g_basicRendererContext.pbrVS = Graphics::CreateShader(MemoryView { shader::PBRVS_Bytecode, shader::PBRVS_Size });
        Graphics::SetShaderName(g_basicRendererContext.pbrVS, "jam engine predefined shader: pbrVS");

        g_basicRendererContext.fullscreenVS = Graphics::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        Graphics::SetShaderName(g_basicRendererContext.fullscreenVS, "jam engine predefined shader: fullscreenVS");

        g_basicRendererContext.pbrLightVolumeVS = Graphics::CreateShader(MemoryView { shader::PBRLightVolumeShader_VS_Bytecode, shader::PBRLightVolumeShader_VS_Size });
        Graphics::SetShaderName(g_basicRendererContext.pbrLightVolumeVS, "jam engine predefined shader: pbrLightVolumeVS");

        g_basicRendererContext.pbrEnvironmentLIghtVolumeVS = Graphics::CreateShader(MemoryView { shader::PBRLightVolumeShaderForDirectional_VS_Bytecode, shader::PBRLightVolumeShaderForDirectional_VS_Size });
        Graphics::SetShaderName(g_basicRendererContext.pbrEnvironmentLIghtVolumeVS, "jam engine predefined shader: pbrEnvironmentLightVolumeVS");

        g_basicRendererContext.csmVS = Graphics::CreateShader(MemoryView { shader::CascadeShaderMappingShader_VS_Bytecode, shader::CascadeShaderMappingShader_VS_Size });
        Graphics::SetShaderName(g_basicRendererContext.csmVS, "jam engine predefined shader: csmVS");

        g_basicRendererContext.osmVS = Graphics::CreateShader(MemoryView { shader::OmniDirectionalShaderMappingShader_VS_Bytecode, shader::OmniDirectionalShaderMappingShader_VS_Size });
        Graphics::SetShaderName(g_basicRendererContext.osmVS, "jam engine predefined shader: osmVS");

        g_basicRendererContext.shadowMappingVS = Graphics::CreateShader(MemoryView { shader::ShadowMappingShader_VS_Bytecode, shader::ShadowMappingShader_VS_Size });
        Graphics::SetShaderName(g_basicRendererContext.shadowMappingVS, "jam engine predefined shader: shadowMappingVS");

        g_basicRendererContext.skyboxVS = Graphics::CreateShader(MemoryView { shader::SkyboxShader_VS_Bytecode, shader::SkyboxShader_VS_Size });
        Graphics::SetShaderName(g_basicRendererContext.skyboxVS, "jam engine predefined shader: skyboxVS");
    }

    // geometry shaders
    {
        g_basicRendererContext.csmLevel2GS = Graphics::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_2_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_2_GS_Size });
        Graphics::SetShaderName(g_basicRendererContext.csmLevel2GS, "jam engine predefined shader: csmLevel2GS");

        g_basicRendererContext.csmLevel3GS = Graphics::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Size });
        Graphics::SetShaderName(g_basicRendererContext.csmLevel3GS, "jam engine predefined shader: csmLevel3GS");

        g_basicRendererContext.csmLevel4GS = Graphics::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_4_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_4_GS_Size });
        Graphics::SetShaderName(g_basicRendererContext.csmLevel4GS, "jam engine predefined shader: csmLevel4GS");

        g_basicRendererContext.osmGS = Graphics::CreateShader(MemoryView { shader::OmniDirectionalShaderMappingShader_GS_Bytecode, shader::OmniDirectionalShaderMappingShader_GS_Size });
        Graphics::SetShaderName(g_basicRendererContext.osmGS, "jam engine predefined shader: osmGS");
    }

    // pixel shaders
    {
        g_basicRendererContext.pbrGBufferPS = Graphics::CreateShader(MemoryView { shader::PBRGBufferShader_PS_Bytecode, shader::PBRGBufferShader_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.pbrGBufferPS, "jam engine predefined shader: pbrGBufferPS");

        g_basicRendererContext.pbrLightingPS = Graphics::CreateShader(MemoryView { shader::PBRLightingShader_PS_Bytecode, shader::PBRLightingShader_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.pbrLightingPS, "jam engine predefined shader: pbrLightingPS");

        g_basicRendererContext.pbrLightVolumePS = Graphics::CreateShader(MemoryView { shader::PBRLightVolumeShader_PS_Bytecode, shader::PBRLightVolumeShader_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.pbrLightVolumePS, "jam engine predefined shader: pbrLightVolumePS");

        g_basicRendererContext.pbrEnvironmentLightVolumePS = Graphics::CreateShader(MemoryView { shader::PBRLightingShader_PS_Bytecode, shader::PBRLightingShader_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.pbrEnvironmentLightVolumePS, "jam engine predefined shader: pbrEnvironmentLightVolumePS");

        g_basicRendererContext.csmPS = Graphics::CreateShader(MemoryView { shader::CascadeShaderMappingShader_PS_Bytecode, shader::CascadeShaderMappingShader_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.csmPS, "jam engine predefined shader: csmPS");

        g_basicRendererContext.osmPS = Graphics::CreateShader(MemoryView { shader::OmniDirectionalShaderMappingShader_PS_Bytecode, shader::OmniDirectionalShaderMappingShader_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.osmPS, "jam engine predefined shader: osmPS");

        g_basicRendererContext.shadowMappingPS = Graphics::CreateShader(MemoryView { shader::ShadowMappingShader_PS_Bytecode, shader::ShadowMappingShader_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.shadowMappingPS, "jam engine predefined shader: shadowMappingPS");

        g_basicRendererContext.skyboxPS = Graphics::CreateShader(MemoryView { shader::SkyboxShader_PS_Bytecode, shader::SkyboxShader_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.skyboxPS, "jam engine predefined shader: skyboxPS");

        g_basicRendererContext.ssaoPS = Graphics::CreateShader(MemoryView { shader::SSAOPS_Bytecode, shader::SSAOPS_Size });
        Graphics::SetShaderName(g_basicRendererContext.ssaoPS, "jam engine predefined shader: ssaoPS");

        g_basicRendererContext.ssaoBlurXPS = Graphics::CreateShader(MemoryView { shader::SSAOBlurXPS_Bytecode, shader::SSAOBlurXPS_Size });
        Graphics::SetShaderName(g_basicRendererContext.ssaoBlurXPS, "jam engine predefined shader: ssaoBlurXPS");

        g_basicRendererContext.ssaoBlurYPS = Graphics::CreateShader(MemoryView { shader::SSAOBlurYPS_Bytecode, shader::SSAOBlurYPS_Size });
        Graphics::SetShaderName(g_basicRendererContext.ssaoBlurYPS, "jam engine predefined shader: ssaoBlurYPS");

        g_basicRendererContext.bloomXPS = Graphics::CreateShader(MemoryView { shader::BloomXPS_Bytecode, shader::BloomXPS_Size });
        Graphics::SetShaderName(g_basicRendererContext.bloomXPS, "jam engine predefined shader: bloomXPS");

        g_basicRendererContext.bloomYPS = Graphics::CreateShader(MemoryView { shader::BloomYPS_Bytecode, shader::BloomYPS_Size });
        Graphics::SetShaderName(g_basicRendererContext.bloomYPS, "jam engine predefined shader: bloomYPS");

        g_basicRendererContext.bloomCombinePS = Graphics::CreateShader(MemoryView { shader::BloomCombinePS_Bytecode, shader::BloomCombinePS_Size });
        Graphics::SetShaderName(g_basicRendererContext.bloomCombinePS, "jam engine predefined shader: bloomCombinePS");

        g_basicRendererContext.toneMappingUncharted2PS = Graphics::CreateShader(MemoryView { shader::ToneMappingPS_UNCHARTED2_Bytecode, shader::ToneMappingPS_UNCHARTED2_Size });
        Graphics::SetShaderName(g_basicRendererContext.toneMappingUncharted2PS, "jam engine predefined shader: toneMappingUncharted2PS");

        g_basicRendererContext.toneMappingLinearPS = Graphics::CreateShader(MemoryView { shader::ToneMappingPS_LINEAR_Bytecode, shader::ToneMappingPS_LINEAR_Size });
        Graphics::SetShaderName(g_basicRendererContext.toneMappingLinearPS, "jam engine predefined shader: toneMappingLinearPS");

        g_basicRendererContext.toneMappingReinhardPS = Graphics::CreateShader(MemoryView { shader::ToneMappingPS_REINHARD_Bytecode, shader::ToneMappingPS_REINHARD_Size });
        Graphics::SetShaderName(g_basicRendererContext.toneMappingReinhardPS, "jam engine predefined shader: toneMappingReinhardPS");

        g_basicRendererContext.toneMappingLumaBasedReinhardPS = Graphics::CreateShader(MemoryView { shader::ToneMappingPS_LUMA_BASED_REINHARD_Bytecode, shader::ToneMappingPS_LUMA_BASED_REINHARD_Size });
        Graphics::SetShaderName(g_basicRendererContext.toneMappingLumaBasedReinhardPS, "jam engine predefined shader: toneMappingLumaBasedReinhardPS");

        g_basicRendererContext.toneMappingWhitePreservingReinhardPS = Graphics::CreateShader(MemoryView { shader::ToneMappingPS_WHITE_PRESERVING_REINHARD_Bytecode, shader::ToneMappingPS_WHITE_PRESERVING_REINHARD_Size });
        Graphics::SetShaderName(g_basicRendererContext.toneMappingWhitePreservingReinhardPS, "jam engine predefined shader: toneMappingWhitePreservingReinhardPS");

        g_basicRendererContext.toneMappingRombDaHousePS = Graphics::CreateShader(MemoryView { shader::ToneMappingPS_ROMB_DA_HOUSE_Bytecode, shader::ToneMappingPS_ROMB_DA_HOUSE_Size });
        Graphics::SetShaderName(g_basicRendererContext.toneMappingRombDaHousePS, "jam engine predefined shader: toneMappingRombDaHousePS");

        g_basicRendererContext.toneMappingFilmicPS = Graphics::CreateShader(MemoryView { shader::ToneMappingPS_FILMIC_Bytecode, shader::ToneMappingPS_FILMIC_Size });
        Graphics::SetShaderName(g_basicRendererContext.toneMappingFilmicPS, "jam engine predefined shader: toneMappingFilmicPS");

        g_basicRendererContext.fxaaVeryLowPS = Graphics::CreateShader(MemoryView { shader::FXAA_VeryLow_PS_Bytecode, shader::FXAA_VeryLow_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.fxaaVeryLowPS, "jam engine predefined shader: fxaaVeryLowPS");

        g_basicRendererContext.fxaaLowPS = Graphics::CreateShader(MemoryView { shader::FXAA_VeryLow_PS_Bytecode, shader::FXAA_VeryLow_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.fxaaLowPS, "jam engine predefined shader: fxaaLowPS");

        g_basicRendererContext.fxaaMediumPS = Graphics::CreateShader(MemoryView { shader::FXAA_Medium_PS_Bytecode, shader::FXAA_Medium_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.fxaaMediumPS, "jam engine predefined shader: fxaaMediumPS");

        g_basicRendererContext.fxaaHighPS = Graphics::CreateShader(MemoryView { shader::FXAA_High_PS_Bytecode, shader::FXAA_High_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.fxaaHighPS, "jam engine predefined shader: fxaaHighPS");

        g_basicRendererContext.fxaaVeryHighPS = Graphics::CreateShader(MemoryView { shader::FXAA_VeryHigh_PS_Bytecode, shader::FXAA_VeryHigh_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.fxaaVeryHighPS, "jam engine predefined shader: fxaaVeryHighPS");

        g_basicRendererContext.fxaaUltraPS = Graphics::CreateShader(MemoryView { shader::FXAA_Ultra_PS_Bytecode, shader::FXAA_Ultra_PS_Size });
        Graphics::SetShaderName(g_basicRendererContext.fxaaUltraPS, "jam engine predefined shader: fxaaUltraPS");

        g_basicRendererContext.fogPS = Graphics::CreateShader(MemoryView { shader::FogPS_Bytecode, shader::FogPS_Size });
        Graphics::SetShaderName(g_basicRendererContext.fogPS, "jam engine predefined shader: fogPS");

        g_basicRendererContext.passThroughPS = Graphics::CreateShader(MemoryView { shader::PassThroughPS_Bytecode, shader::PassThroughPS_Size });
        Graphics::SetShaderName(g_basicRendererContext.passThroughPS, "jam engine predefined shader: passThroughPS");
    }

    // programs
    {
        g_basicRendererContext.pbrGBufferProgram                         = Graphics::CreateProgram(g_basicRendererContext.pbrVS, g_basicRendererContext.pbrGBufferPS);
        g_basicRendererContext.pbrLightingProgram                        = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.pbrLightingPS);
        g_basicRendererContext.pbrLightVolumeProgram                     = Graphics::CreateProgram(g_basicRendererContext.pbrLightVolumeVS, g_basicRendererContext.pbrLightVolumePS);
        g_basicRendererContext.pbrEnvironmentLightVolumeProgram          = Graphics::CreateProgram(g_basicRendererContext.pbrEnvironmentLIghtVolumeVS, g_basicRendererContext.pbrEnvironmentLightVolumePS);
        g_basicRendererContext.csmLevel2CasterProgram                    = Graphics::CreateProgram(g_basicRendererContext.csmVS, g_basicRendererContext.csmPS, g_basicRendererContext.csmLevel2GS, k_invalidHandle, k_invalidHandle);
        g_basicRendererContext.csmLevel3CasterProgram                    = Graphics::CreateProgram(g_basicRendererContext.csmVS, g_basicRendererContext.csmPS, g_basicRendererContext.csmLevel3GS, k_invalidHandle, k_invalidHandle);
        g_basicRendererContext.csmLevel4CasterProgram                    = Graphics::CreateProgram(g_basicRendererContext.csmVS, g_basicRendererContext.csmPS, g_basicRendererContext.csmLevel4GS, k_invalidHandle, k_invalidHandle);
        g_basicRendererContext.osmCasterProgram                          = Graphics::CreateProgram(g_basicRendererContext.osmVS, g_basicRendererContext.osmPS, g_basicRendererContext.osmGS, k_invalidHandle, k_invalidHandle);
        g_basicRendererContext.shadowCasterProgram                       = Graphics::CreateProgram(g_basicRendererContext.shadowMappingVS, g_basicRendererContext.shadowMappingPS);
        g_basicRendererContext.skyboxProgram                             = Graphics::CreateProgram(g_basicRendererContext.skyboxVS, g_basicRendererContext.skyboxPS);
        g_basicRendererContext.ssaoProgram                               = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.ssaoPS);
        g_basicRendererContext.ssaoBlurXProgram                          = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.ssaoBlurXPS);
        g_basicRendererContext.ssaoBlurYProgram                          = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.ssaoBlurYPS);
        g_basicRendererContext.bloomXProgram                             = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.bloomXPS);
        g_basicRendererContext.bloomYProgram                             = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.bloomYPS);
        g_basicRendererContext.bloomCombineProgram                       = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.bloomCombinePS);
        g_basicRendererContext.toneMappingUncharted2Program              = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.toneMappingUncharted2PS);
        g_basicRendererContext.toneMappingLinearProgram                  = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.toneMappingLinearPS);
        g_basicRendererContext.toneMappingReinhardProgram                = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.toneMappingReinhardPS);
        g_basicRendererContext.toneMappingLumaBasedReinhardProgram       = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.toneMappingLumaBasedReinhardPS);
        g_basicRendererContext.toneMappingWhitePreservingReinhardProgram = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.toneMappingWhitePreservingReinhardPS);
        g_basicRendererContext.toneMappingRombDaHouseProgram             = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.toneMappingRombDaHousePS);
        g_basicRendererContext.toneMappingFilmicProgram                  = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.toneMappingFilmicPS);
        g_basicRendererContext.fxaaVeryLowProgram                        = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaVeryLowPS);
        g_basicRendererContext.fxaaLowProgram                            = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaLowPS);
        g_basicRendererContext.fxaaMediumProgram                         = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaMediumPS);
        g_basicRendererContext.fxaaHighProgram                           = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaHighPS);
        g_basicRendererContext.fxaaVeryHighProgram                       = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaVeryHighPS);
        g_basicRendererContext.fxaaUltraProgram                          = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaUltraPS);
        g_basicRendererContext.fogProgram                                = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fogPS);
        g_basicRendererContext.passThroughProgram                        = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.passThroughPS);
    }

    // constant buffers
    {
        g_basicRendererContext.transformCbh = Graphics::CreateConstantBuffer(sizeof(CB_TRANSFORM));
        Graphics::SetConstantBufferName(g_basicRendererContext.transformCbh, "jam engine predefined constant buffer: CB_TRANSFORM");

        g_basicRendererContext.cameraCbh = Graphics::CreateConstantBuffer(sizeof(CB_CAMERA));
        Graphics::SetConstantBufferName(g_basicRendererContext.cameraCbh, "jam engine predefined constant buffer: CB_CAMERA");

        g_basicRendererContext.materialCbh = Graphics::CreateConstantBuffer(sizeof(CB_MATERIAL));
        Graphics::SetConstantBufferName(g_basicRendererContext.materialCbh, "jam engine predefined constant buffer: CB_MATERIAL");

        g_basicRendererContext.lightCbh = Graphics::CreateConstantBuffer(sizeof(CB_LIGHT));
        Graphics::SetConstantBufferName(g_basicRendererContext.lightCbh, "jam engine predefined constant buffer: CB_LIGHT");

        g_basicRendererContext.environmentCbh = Graphics::CreateConstantBuffer(sizeof(CB_ENVIRONMENT));
        Graphics::SetConstantBufferName(g_basicRendererContext.environmentCbh, "jam engine predefined constant buffer: CB_GLOBAL");

        g_basicRendererContext.shadowCasterCbh = Graphics::CreateConstantBuffer(sizeof(CB_SHADOW_CASTER));
        Graphics::SetConstantBufferName(g_basicRendererContext.shadowCasterCbh, "jam engine predefined constant buffer: CB_SHADOW_CASTER");

        g_basicRendererContext.csmCasterCbh = Graphics::CreateConstantBuffer(sizeof(CB_CASCADE_SHADOW_CASTER));
        Graphics::SetConstantBufferName(g_basicRendererContext.csmCasterCbh, "jam engine predefined constant buffer: CB_CASCADE_SHADOW_CASTER");

        g_basicRendererContext.osmCasterCbh = Graphics::CreateConstantBuffer(sizeof(CB_OMNIDIRECTIONAL_SHADOW_CASTER));
        Graphics::SetConstantBufferName(g_basicRendererContext.osmCasterCbh, "jam engine predefined constant buffer: CB_OMNIDIRECTIONAL_SHADOW_CASTER");

        g_basicRendererContext.ssaoCbh = Graphics::CreateConstantBuffer(sizeof(CB_SSAO_KERNEL));
        Graphics::SetConstantBufferName(g_basicRendererContext.ssaoCbh, "jam engine predefined constant buffer: CB_SSAO_KERNEL");
    }

    // full screen tri
    {
        VertexPosTex2D vertices[] = {
            { Vec2(-1.f, -1.f), Vec2(0.f,  1.f) },
            { Vec2(-1.f,  3.f), Vec2(0.f, -1.f) },
            {  Vec2(3.f, -1.f), Vec2(2.f,  1.f) }
        };
        UInt16 indices[] = { 0, 1, 2 };
        g_basicRendererContext.fullScreenTri.Load(MemoryView { vertices }, VertexPosTex2D::s_layout, MemoryView { indices });
    }

    // light volume full tri
    {
        VertexPos3D vertices[] = {
            { Vec3(-1.f, -1.f, 0.f) },
            { Vec3(-1.f, 3.f, 0.f) },
            { Vec3(3.f, -1.f, 0.f) }
        };
        UInt16 indices[] = { 0, 1, 2 };
        g_basicRendererContext.lightVolumeFullScreenTri.Load(MemoryView { vertices }, VertexPos3D::s_layout, MemoryView { indices });
    }

    // skybox cube
    {
        Vec3 whd                          = { 1.f, 1.f, 1.f };
        g_basicRendererContext.skyboxCube = CreateBoxMesh(whd, VertexPos3D::s_layout);
    }

    // unit sphere
    {
        g_basicRendererContext.lightVolumeUnitSphere = CreateSphereMesh(1.f, 2, VertexPos3D::s_layout);
    }

    // unit corn
    {
        g_basicRendererContext.lightVolumeUnitCone = CreateCone(1.f, 1.f, 20, VertexPos3D::s_layout);
    }

    // sampler
    {
        g_basicRendererContext.linearWrapSampler.filter   = eTextureFilter::MinLinear_MagLinear_MipLinear;
        g_basicRendererContext.linearWrapSampler.addressU = eTextureAddress::Wrap;
        g_basicRendererContext.linearWrapSampler.addressV = eTextureAddress::Wrap;
        g_basicRendererContext.linearWrapSampler.addressW = eTextureAddress::Wrap;

        g_basicRendererContext.linearClampSampler.filter   = eTextureFilter::MinLinear_MagLinear_MipLinear;
        g_basicRendererContext.linearClampSampler.addressU = eTextureAddress::Clamp;
        g_basicRendererContext.linearClampSampler.addressV = eTextureAddress::Clamp;
        g_basicRendererContext.linearClampSampler.addressW = eTextureAddress::Clamp;

        g_basicRendererContext.pointWrapSampler.filter   = eTextureFilter::MinPoint_MagPoint_MipPoint;
        g_basicRendererContext.pointWrapSampler.addressU = eTextureAddress::Wrap;
        g_basicRendererContext.pointWrapSampler.addressV = eTextureAddress::Wrap;
        g_basicRendererContext.pointWrapSampler.addressW = eTextureAddress::Wrap;

        g_basicRendererContext.pointClampSampler.filter   = eTextureFilter::MinPoint_MagPoint_MipPoint;
        g_basicRendererContext.pointClampSampler.addressU = eTextureAddress::Clamp;
        g_basicRendererContext.pointClampSampler.addressV = eTextureAddress::Clamp;
        g_basicRendererContext.pointClampSampler.addressW = eTextureAddress::Clamp;

        g_basicRendererContext.shadowLinearClampSampler.filter      = eTextureFilter::MinLinear_MagLinear_MipLinear;
        g_basicRendererContext.shadowLinearClampSampler.addressU    = eTextureAddress::Clamp;
        g_basicRendererContext.shadowLinearClampSampler.addressV    = eTextureAddress::Clamp;
        g_basicRendererContext.shadowLinearClampSampler.addressW    = eTextureAddress::Clamp;
        g_basicRendererContext.shadowLinearClampSampler.compareFunc = eCompareFunc::LessEqual;

        // 후면 컬링, 깊이 테스트 활성화, 깊이 쓰기 활성화
        g_basicRendererContext.depthTestState = GfxState::k_depthTest;

        // 전면 컬링, 깊이 비활성화
        g_basicRendererContext.lightVolumeState = GfxState { ePrimitive::TriangleList, eCullMode::Front, false, false, false, false, false, false, false, false, eCompareFunc::LessEqual, false };

        // 전면 컬링, 깊이 테스트 활성화, 깊이 쓰기 비활성화
        g_basicRendererContext.skyboxState = GfxState { ePrimitive::TriangleList, eCullMode::Front, true, false, false, false, false, false, false, false, eCompareFunc::LessEqual, false };
    }
}

void BasicRenderer::Shutdown()
{
    JAM_DESTROY_SHADER(g_basicRendererContext.pbrVS);
    JAM_DESTROY_SHADER(g_basicRendererContext.fullscreenVS);
    JAM_DESTROY_SHADER(g_basicRendererContext.pbrLightVolumeVS);
    JAM_DESTROY_SHADER(g_basicRendererContext.pbrEnvironmentLIghtVolumeVS);
    JAM_DESTROY_SHADER(g_basicRendererContext.csmVS);
    JAM_DESTROY_SHADER(g_basicRendererContext.osmVS);
    JAM_DESTROY_SHADER(g_basicRendererContext.shadowMappingVS);
    JAM_DESTROY_SHADER(g_basicRendererContext.skyboxVS);

    JAM_DESTROY_SHADER(g_basicRendererContext.csmLevel2GS);
    JAM_DESTROY_SHADER(g_basicRendererContext.csmLevel3GS);
    JAM_DESTROY_SHADER(g_basicRendererContext.csmLevel4GS);
    JAM_DESTROY_SHADER(g_basicRendererContext.osmGS);

    JAM_DESTROY_SHADER(g_basicRendererContext.pbrGBufferPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.pbrLightingPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.pbrLightVolumePS);
    JAM_DESTROY_SHADER(g_basicRendererContext.pbrEnvironmentLightVolumePS);
    JAM_DESTROY_SHADER(g_basicRendererContext.csmPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.osmPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.shadowMappingPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.skyboxPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.ssaoPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.ssaoBlurXPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.ssaoBlurYPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.bloomXPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.bloomYPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.bloomCombinePS);
    JAM_DESTROY_SHADER(g_basicRendererContext.toneMappingUncharted2PS);
    JAM_DESTROY_SHADER(g_basicRendererContext.toneMappingLinearPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.toneMappingReinhardPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.toneMappingLumaBasedReinhardPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.toneMappingWhitePreservingReinhardPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.toneMappingRombDaHousePS);
    JAM_DESTROY_SHADER(g_basicRendererContext.toneMappingFilmicPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.fxaaVeryLowPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.fxaaLowPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.fxaaMediumPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.fxaaHighPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.fxaaVeryHighPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.fxaaUltraPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.fogPS);
    JAM_DESTROY_SHADER(g_basicRendererContext.passThroughPS);

    JAM_DESTROY_PROGRAM(g_basicRendererContext.pbrGBufferProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.pbrLightingProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.pbrLightVolumeProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.pbrEnvironmentLightVolumeProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.csmLevel2CasterProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.csmLevel3CasterProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.csmLevel4CasterProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.osmCasterProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.shadowCasterProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.skyboxProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.ssaoProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.ssaoBlurXProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.ssaoBlurYProgram);

    JAM_DESTROY_PROGRAM(g_basicRendererContext.bloomXProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.bloomYProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.bloomCombineProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.toneMappingUncharted2Program);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.toneMappingLinearProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.toneMappingReinhardProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.toneMappingLumaBasedReinhardProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.toneMappingWhitePreservingReinhardProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.toneMappingRombDaHouseProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.toneMappingFilmicProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.fxaaVeryLowProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.fxaaLowProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.fxaaMediumProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.fxaaHighProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.fxaaVeryHighProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.fxaaUltraProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.fogProgram);
    JAM_DESTROY_PROGRAM(g_basicRendererContext.passThroughProgram);

    JAM_DESTROY_CONSTANT_BUFFER(g_basicRendererContext.transformCbh);
    JAM_DESTROY_CONSTANT_BUFFER(g_basicRendererContext.cameraCbh);
    JAM_DESTROY_CONSTANT_BUFFER(g_basicRendererContext.materialCbh);
    JAM_DESTROY_CONSTANT_BUFFER(g_basicRendererContext.lightCbh);
    JAM_DESTROY_CONSTANT_BUFFER(g_basicRendererContext.environmentCbh);
    JAM_DESTROY_CONSTANT_BUFFER(g_basicRendererContext.shadowCasterCbh);
    JAM_DESTROY_CONSTANT_BUFFER(g_basicRendererContext.csmCasterCbh);
    JAM_DESTROY_CONSTANT_BUFFER(g_basicRendererContext.osmCasterCbh);
    JAM_DESTROY_CONSTANT_BUFFER(g_basicRendererContext.ssaoCbh);

    JAM_DESTROY_TEXTURE(g_basicRendererContext.ssaoNoiseTexture);

    g_basicRendererContext.fullScreenTri.Unload();
    g_basicRendererContext.lightVolumeFullScreenTri.Unload();
    g_basicRendererContext.lightVolumeUnitSphere.Unload();
    g_basicRendererContext.lightVolumeUnitCone.Unload();
    g_basicRendererContext.skyboxCube.Unload();
}

void BasicRenderer::Render(const Vec3& _eyePos, const Mat4& _view, const Mat4& _proj, const Environment& _env)
{
    {
        g_basicRendererContext.timer.Reset();
        g_basicRendererContext.timer.Start();

        // camera buffer
        {
            CB_CAMERA cb;
            cb.cb_view        = _view.Transpose();
            cb.cb_proj        = _proj.Transpose();
            cb.cb_projViewInv = (cb.cb_proj * cb.cb_view).Invert();   // (_view * _proj).Invert().Transpose() == (_proj.Transpose() * _view.Transpose()).Invert()
            cb.cb_eyePos      = _eyePos;

            Graphics::UploadBuffer(g_basicRendererContext.cameraCbh, MemoryView { cb });
        }

        // environment buffer
        {
            CB_ENVIRONMENT cb;
            cb.cb_envFlags = JAM_ENVIRONMENT_FLAGS_NONE;

            cb.cb_sunDirection = _env.sunDirection;
            cb.cb_sunIntensity = _env.sunIntensity;
            cb.cb_sunColor     = _env.sunColor;

            if (_env.shadowCascadeLevel == eCascadeLevel::level2)
                cb.cb_shadowCascadeLevel = 2;
            else if (_env.shadowCascadeLevel == eCascadeLevel::level3)
                cb.cb_shadowCascadeLevel = 3;
            else if (_env.shadowCascadeLevel == eCascadeLevel::level4)
                cb.cb_shadowCascadeLevel = 4;

            for (Int32 i = 0; i < 4; ++i)
                cb.cb_shadowCascadeRages[i] = _env.shadowCascadeRanges[i];

            cb.cb_shadowBias = _env.shadowBias;

            cb.cb_irradianceMapIntensity = _env.irradianceMapIntensity;
            cb.cb_specularMapIntensity   = _env.specularMapIntensity;
            cb.cb_skyBoxColor            = _env.skyboxColor;

            if (_env.ssaoKernelSize == eSSAOKernel::Kernel4x4)
                cb.cb_ssaoKernelSize = 16;
            else if (_env.ssaoKernelSize == eSSAOKernel::Kernel6x6)
                cb.cb_ssaoKernelSize = 36;
            else if (_env.ssaoKernelSize == eSSAOKernel::Kernel8x8)
                cb.cb_ssaoKernelSize = 64;

            cb.cb_ssaoRadius = _env.ssaoRadius;
            cb.cb_ssaoBias   = _env.ssaoBias;

            cb.cb_fogColor             = _env.fogColor;
            cb.cb_fogDensity           = _env.fogDensity;
            cb.cb_fogFallOffStart      = _env.fogFallOffStart;
            cb.cb_fogFallOffEnd        = _env.fogFallOffEnd;
            cb.cb_fogHeightAttenuation = _env.fogHeightAttenuation;

            cb.cb_exposure = _env.exposure;
            cb.cb_gamma    = _env.gamma;

            cb.cb_bloomBlurRadius = _env.bloomBlurRadius;
            cb.cb_combineFactor   = _env.bloomCombineFactor;

            if (_env.bShadowEnabled)
                cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SUN_SHADOW;
            if (_env.bSSAOEnabled)
                cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SSAO;

            if (_env.skyboxType == eSkyboxType::Cubemap)
                cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SKYBOX_CUBEMAP;
            else if (_env.skyboxType == eSkyboxType::SolidColor)
                cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SKYBOX_SOLID_COLOR;

            Graphics::UploadBuffer(g_basicRendererContext.environmentCbh, MemoryView { cb });
        }

        g_basicRendererContext.pCurEnv = &_env;
    }

    // render
    g_basicRendererContext.graph.Execute();

    // end
    g_basicRendererContext.timer.Stop();
}

const Timer& BasicRenderer::GetRendererCPUProfileTimer()
{
    return g_basicRendererContext.timer;
}

CullingSystem& BasicRenderer::GetCullingSystem()
{
    return g_basicRendererContext.cullingSystem;
}

void BasicRenderer::Submit(const Mesh& _mesh, const Material& _material, const Transform& _transform)
{
    g_basicRendererContext.UploadMaterial(_material);
    g_basicRendererContext.UploadTransform(_transform.CreateWorld());
    _mesh.Submit();
}

void BasicRenderer::Submit(const Mesh& _mesh, const Transform& _transform)
{
    g_basicRendererContext.UploadTransform(_transform.CreateWorld());
    _mesh.Submit();
}

void BasicRenderer::CompileGraph()
{
    g_basicRendererContext.graph.Compile();
}

void BasicRenderer::AddPass(const RenderPass& _pass)
{
    g_basicRendererContext.graph.AddPass(_pass);
}

RenderPass BasicRenderer::CreatePBRGBufferPass(const PBRGBufferPassDesc& _desc)
{
    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.pbrGBufferProgram)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::VertexShader, s_linearClampSlot)
        .AddConstantBuffer(g_basicRendererContext.transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT)
        .AddConstantBuffer(g_basicRendererContext.cameraCbh, eShader::VertexShader, CB_CAMERA_SLOT)
        .AddConstantBuffer(g_basicRendererContext.materialCbh, eShader::PixelShader, CB_MATERIAL_SLOT)
        .AddConstantBuffer(g_basicRendererContext.environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(g_basicRendererContext.depthTestState)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0);

    return pass;
}

RenderPass BasicRenderer::CreatePBRLightingPass(const PBRLightPassDesc& _desc)
{
    const Environment* pEnv = g_basicRendererContext.pCurEnv;

    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.pbrLightingProgram)
        .AddSampler(g_basicRendererContext.pointClampSampler, eShader::PixelShader, s_pointClampSlot)
        .AddSampler(g_basicRendererContext.linearWrapSampler, eShader::PixelShader, s_linearWrapSlot)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddConstantBuffer(g_basicRendererContext.cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
        .AddConstantBuffer(g_basicRendererContext.environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
        .AddTexture(pEnv->irradianceMap->GetHandle(), eShader::PixelShader, t_irradianceSlot)
        .AddTexture(pEnv->specularMap->GetHandle(), eShader::PixelShader, t_specularSlot)
        .AddTexture(pEnv->brdfLUT->GetHandle(), eShader::PixelShader, t_brdfSlot)
        .AddTexture(_desc.normalRoughnessMetallicTexture, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot)
        .AddTexture(_desc.albedoAOTexture, eShader::PixelShader, t_gBufferAlbedoAOSlot)
        .AddTexture(_desc.additionalLightTexture, eShader::PixelShader, t_gBufferAdditionalLightSlot)
        .AddTexture(_desc.depthTexture, eShader::PixelShader, t_depthSlot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0);

    if (JAM_IS_VALID_HANDLE(_desc.additionalLightTexture))
    {
        pass.AddTexture(_desc.ssaoTexture, eShader::PixelShader, t_ssaoSlot);
    }

    return pass;
}

}   // namespace jam