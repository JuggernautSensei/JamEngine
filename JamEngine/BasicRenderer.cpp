#include "pch.h"

#include "BasicRenderer.h"

#include "BasicRendererBase.h"
#include "CullingSystem.h"
#include "EmbeddedShaderGlue.h"
#include "Environment.h"
#include "Graphics.h"
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
    void UploadPointLight(const Vec3& _radiance, float _lightIntensity, const Vec3& _pos, float _fallOffStart, float _fallOffEnd, bool _bShadow) const;
    void UploadSpotLight(const Vec3& _radiance, float _lightIntensity, const Vec3& _pos, const Vec3& _dir, float _fallOffStart, float _fallOffEnd, float _innerConeAngle, float _outerConeAngle, bool _bShadow) const;

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
    ShaderHandle toneMappingPS;
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
    ProgramHandle toneMappingProgram;
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

    TextureHandle ssaoNoiseTexture;

    Timer              timer;
    RenderGraph        graph;
    const Environment* pCurEnv = nullptr;

} g_basicRendererContext;

void BasicRendererContext::BakeSSAOKernel() const
{
    Vec3 output[64];
    for (Int32 i = 0; i < 64; ++i)
    {
        // 반구
        Vec3 sample = Vec3 {
            CreateRandom<float>(-1.f, 1.f),
            CreateRandom<float>(-1.f, 1.f),
            CreateRandom<float>(0.f, 1.f)
        };
        sample.Normalize();
        sample *= CreateRandom<float>(0.f, 1.f);
        float sacle = static_cast<float>(i) / static_cast<float>(64);
        sacle       = Lerp(0.1f, 1.f, sacle * sacle);   // 가까운 샘플에 더 집중
        sample *= sacle;

        output[i] = sample;
    }

    Graphics::UploadBuffer(ssaoCbh, MemoryView { output });
}

void BasicRendererContext::BakeSSAONoise()
{
    JAM_DESTROY_TEXTURE(ssaoNoiseTexture);

    constexpr UInt32 k_noiseSize = 4;

    Float16 noiseData[4 * k_noiseSize * k_noiseSize];
    for (UInt32 i = 0; i < k_noiseSize * k_noiseSize; i = i + 4)
    {
        noiseData[i]     = ToFloat16(CreateRandom<float>(-1.f, 1.f));
        noiseData[i + 1] = ToFloat16(CreateRandom<float>(-1.f, 1.f));
        noiseData[i + 2] = ToFloat16(0.f);
        noiseData[i + 3] = ToFloat16(0.f);
    }

    MemoryView mem[] = {
        MemoryView { noiseData, sizeof(noiseData) }
    };

    ssaoNoiseTexture = Graphics::CreateTexture2D(k_noiseSize, k_noiseSize, eFormat::RGBA16_Float, 1, eTextureCreateFlags_None, mem);
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

void BasicRendererContext::UploadPointLight(const Vec3& _radiance, const float _lightIntensity, const Vec3& _pos, const float _fallOffStart, const float _fallOffEnd, const bool _bShadow) const
{
    CB_LIGHT cb                 = {};
    cb.cb_lightType             = JAM_LIGHT_TYPE_POINT;
    cb.cb_lightPos              = _radiance;
    cb.cb_lightDir              = Vec3::Forward;
    cb.cb_lightRadiance         = _radiance;
    cb.cb_lightIntensity        = _lightIntensity;
    cb.cb_lightFallOffStart     = _fallOffStart;
    cb.cb_lightFallOffEnd       = _fallOffEnd;
    cb.cb_lightInnerConeHalfRad = 0;
    cb.cb_lightOuterConeHalfRad = 0;
    cb.cb_bShadowMapping        = _bShadow ? JAM_TRUE : JAM_FALSE;
    Graphics::UploadBuffer(lightCbh, MemoryView { cb });
}

void BasicRendererContext::UploadSpotLight(const Vec3& _radiance, const float _lightIntensity, const Vec3& _pos, const Vec3& _dir, const float _fallOffStart, const float _fallOffEnd, const float _innerConeAngle, const float _outerConeAngle, const bool _bShadow) const
{
    CB_LIGHT cb                 = {};
    cb.cb_lightType             = JAM_LIGHT_TYPE_POINT;
    cb.cb_lightPos              = _pos;
    cb.cb_lightDir              = _dir;
    cb.cb_lightRadiance         = _radiance;
    cb.cb_lightIntensity        = _lightIntensity;
    cb.cb_lightFallOffStart     = _fallOffStart;
    cb.cb_lightFallOffEnd       = _fallOffEnd;
    cb.cb_lightInnerConeHalfRad = _innerConeAngle;
    cb.cb_lightOuterConeHalfRad = _outerConeAngle;
    cb.cb_bShadowMapping        = _bShadow ? JAM_TRUE : JAM_FALSE;
    Graphics::UploadBuffer(lightCbh, MemoryView { cb });
}

}   // namespace jam

// executors
namespace jam
{

struct FullScreenTriExecutor
{
    void operator()() const
    {
        g_basicRendererContext.fullScreenTri.Submit();
    }
};

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

        g_basicRendererContext.toneMappingPS = Graphics::CreateShader(MemoryView { shader::ToneMappingPS_Bytecode, shader::ToneMappingPS_Size });
        Graphics::SetShaderName(g_basicRendererContext.toneMappingPS, "jam engine predefined shader: toneMappingPS");

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
        g_basicRendererContext.pbrGBufferProgram                = Graphics::CreateProgram(g_basicRendererContext.pbrVS, g_basicRendererContext.pbrGBufferPS);
        g_basicRendererContext.pbrLightingProgram               = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.pbrLightingPS);
        g_basicRendererContext.pbrLightVolumeProgram            = Graphics::CreateProgram(g_basicRendererContext.pbrLightVolumeVS, g_basicRendererContext.pbrLightVolumePS);
        g_basicRendererContext.pbrEnvironmentLightVolumeProgram = Graphics::CreateProgram(g_basicRendererContext.pbrEnvironmentLIghtVolumeVS, g_basicRendererContext.pbrEnvironmentLightVolumePS);
        g_basicRendererContext.csmLevel2CasterProgram           = Graphics::CreateProgram(g_basicRendererContext.csmVS, g_basicRendererContext.csmPS, g_basicRendererContext.csmLevel2GS, k_invalidHandle, k_invalidHandle);
        g_basicRendererContext.csmLevel3CasterProgram           = Graphics::CreateProgram(g_basicRendererContext.csmVS, g_basicRendererContext.csmPS, g_basicRendererContext.csmLevel3GS, k_invalidHandle, k_invalidHandle);
        g_basicRendererContext.csmLevel4CasterProgram           = Graphics::CreateProgram(g_basicRendererContext.csmVS, g_basicRendererContext.csmPS, g_basicRendererContext.csmLevel4GS, k_invalidHandle, k_invalidHandle);
        g_basicRendererContext.osmCasterProgram                 = Graphics::CreateProgram(g_basicRendererContext.osmVS, g_basicRendererContext.osmPS, g_basicRendererContext.osmGS, k_invalidHandle, k_invalidHandle);
        g_basicRendererContext.shadowCasterProgram              = Graphics::CreateProgram(g_basicRendererContext.shadowMappingVS, g_basicRendererContext.shadowMappingPS);
        g_basicRendererContext.skyboxProgram                    = Graphics::CreateProgram(g_basicRendererContext.skyboxVS, g_basicRendererContext.skyboxPS);
        g_basicRendererContext.ssaoProgram                      = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.ssaoPS);
        g_basicRendererContext.ssaoBlurXProgram                 = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.ssaoBlurXPS);
        g_basicRendererContext.ssaoBlurYProgram                 = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.ssaoBlurYPS);
        g_basicRendererContext.bloomXProgram                    = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.bloomXPS);
        g_basicRendererContext.bloomYProgram                    = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.bloomYPS);
        g_basicRendererContext.bloomCombineProgram              = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.bloomCombinePS);
        g_basicRendererContext.toneMappingProgram               = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.toneMappingPS);
        g_basicRendererContext.fxaaVeryLowProgram               = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaVeryLowPS);
        g_basicRendererContext.fxaaLowProgram                   = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaLowPS);
        g_basicRendererContext.fxaaMediumProgram                = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaMediumPS);
        g_basicRendererContext.fxaaHighProgram                  = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaHighPS);
        g_basicRendererContext.fxaaVeryHighProgram              = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaVeryHighPS);
        g_basicRendererContext.fxaaUltraProgram                 = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fxaaUltraPS);
        g_basicRendererContext.fogProgram                       = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.fogPS);
        g_basicRendererContext.passThroughProgram               = Graphics::CreateProgram(g_basicRendererContext.fullscreenVS, g_basicRendererContext.passThroughPS);
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

    // ssao
    {
        g_basicRendererContext.BakeSSAOKernel();
        g_basicRendererContext.BakeSSAONoise();
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
    JAM_DESTROY_SHADER(g_basicRendererContext.toneMappingPS);
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
    JAM_DESTROY_PROGRAM(g_basicRendererContext.toneMappingProgram);
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
    g_basicRendererContext.timer.Reset();
    g_basicRendererContext.timer.Start();

    {
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

const Timer& BasicRenderer::GetRenderCPUProfileTimer()
{
    return g_basicRendererContext.timer;
}

void BasicRenderer::UploadCascadeShadowCasterData(const Frustum& _viewFrustumInW)
{
    const Environment* pEnv = g_basicRendererContext.pCurEnv;

    Int32 cascadeLevel = 0;
    if (pEnv->shadowCascadeLevel == eCascadeLevel::level2)
        cascadeLevel = 2;
    else if (pEnv->shadowCascadeLevel == eCascadeLevel::level3)
        cascadeLevel = 3;
    else if (pEnv->shadowCascadeLevel == eCascadeLevel::level4)
        cascadeLevel = 4;

    CB_CASCADE_SHADOW_CASTER cb;
    const float              nearZ    = _viewFrustumInW.GetNear();
    const float              farZ     = _viewFrustumInW.GetFar();
    const float              invRange = 1.f / (farZ - nearZ);

    Frustum cascadeFrustums[4] = {};
    float   startScale         = 0.f;
    for (Int32 i = 0; i < cascadeLevel; ++i)
    {
        // 각 캐스케이드 프러스텀 생성
        const float scale  = Clamp(pEnv->shadowCascadeRanges[i] * invRange, startScale, 1.f);
        cascadeFrustums[i] = Frustum::CreateSubFrustum(_viewFrustumInW, startScale, scale);
        startScale         = scale;

        // 프러스텀의 코너, 중심
        FrustumCorners corners = cascadeFrustums[i].GetCorners();
        Vec3           center  = Vec3::Zero;
        for (const Vec3& corner: corners.points)
            center += corner;
        center /= 8.f;

        // 라이트 뷰 행렬
        Mat4 lightV = CreateViewLookTo(center, pEnv->sunDirection);

        // 프러스텀의 8개 꼭짓점을 라이트 뷰 공간으로 변환 후 AABB 생성
        AABB lightAABB = {};
        for (Int32 j = 0; j < 8; ++j)
        {
            Vec3 p    = Vec3::Transform(corners.points[j], lightV);
            lightAABB = AABB::Merge(lightAABB, p);
        }

        // z축 범위 약간 확장
        if (lightAABB.min.z < 0.f)
            lightAABB.min.z *= pEnv->shadowCascadeExtendZScale;
        else
            lightAABB.min.z /= pEnv->shadowCascadeExtendZScale;

        if (lightAABB.max.z < 0.f)
            lightAABB.max.z /= pEnv->shadowCascadeExtendZScale;
        else
            lightAABB.max.z *= pEnv->shadowCascadeExtendZScale;

        // 라이트 투영 행렬
        Mat4 lightP          = CreateOrthographic(lightAABB.Width(), lightAABB.Height(), lightAABB.min.z, lightAABB.max.z);
        cb.cb_CSMProjView[i] = (lightV * lightP).Transpose();
    }

    Graphics::UploadBuffer(g_basicRendererContext.csmCasterCbh, MemoryView { cb });
}

void BasicRenderer::UploadOmniDirectionalShadowCasterData(const Vec3& _pos, const float _range)
{
    Mat4 lightP = CreatePerspectiveFov(ToRad(90.f), 1.f, 0.01f, _range);

    CB_OMNIDIRECTIONAL_SHADOW_CASTER cb;
    cb.cb_ODSProjView[EnumToInt(eCubeFace::Right)]  = (CreateViewLookTo(_pos, Vec3::Right) * lightP).Transpose();
    cb.cb_ODSProjView[EnumToInt(eCubeFace::Left)]   = (CreateViewLookTo(_pos, Vec3::Left) * lightP).Transpose();
    cb.cb_ODSProjView[EnumToInt(eCubeFace::Top)]    = (CreateViewLookTo(_pos, Vec3::Up) * lightP).Transpose();
    cb.cb_ODSProjView[EnumToInt(eCubeFace::Bottom)] = (CreateViewLookTo(_pos, Vec3::Down) * lightP).Transpose();
    cb.cb_ODSProjView[EnumToInt(eCubeFace::Front)]  = (CreateViewLookTo(_pos, Vec3::Forward) * lightP).Transpose();
    cb.cb_ODSProjView[EnumToInt(eCubeFace::Back)]   = (CreateViewLookTo(_pos, Vec3::Backward) * lightP).Transpose();
    Graphics::UploadBuffer(g_basicRendererContext.osmCasterCbh, MemoryView { cb });
}

void BasicRenderer::BakeSSAONoiseTexture()
{
    g_basicRendererContext.BakeSSAONoise();
}

void BasicRenderer::BakeSSAOKernel()
{
    g_basicRendererContext.BakeSSAOKernel();
}

void BasicRenderer::Submit(const Mesh& _mesh, const Material& _material, const Transform& _transform)
{
    g_basicRendererContext.UploadMaterial(_material);
    g_basicRendererContext.UploadTransform(_transform.GetMatrix());
    _mesh.Submit();
}

void BasicRenderer::Submit(const Mesh& _mesh, const Transform& _transform)
{
    g_basicRendererContext.UploadTransform(_transform.GetMatrix());
    _mesh.Submit();
}

void BasicRenderer::SubmitPointLight(const Vec3& _radiance, const float _lightIntensity, const Vec3& _pos, const float _fallOffStart, const float _fallOffEnd)
{
    Mat4 lightMat = Mat4::CreateScale(_fallOffEnd) * Mat4::CreateTranslation(_pos);

    g_basicRendererContext.UploadPointLight(_radiance, _lightIntensity, _pos, _fallOffStart, _fallOffEnd, false);
    g_basicRendererContext.UploadTransform(lightMat);
    g_basicRendererContext.lightVolumeUnitSphere.Submit();
}

void BasicRenderer::SubmitPointLightWithShadow(const Vec3& _radiance, const float _lightIntensity, const Vec3& _pos, const float _fallOffStart, const float _fallOffEnd, const TextureHandle _shadowMap)
{
    JAM_ASSERT(!JAM_IS_VALID_HANDLE(_shadowMap), "Not implemented yet");
    Graphics::SetTexture(_shadowMap, eShader::PixelShader, t_shadowSlot);
    SubmitPointLight(_radiance, _lightIntensity, _pos, _fallOffStart, _fallOffEnd);
}

void BasicRenderer::SubmitSpotLight(const Vec3& _radiance, const float _lightIntensity, const Vec3& _pos, const Vec3& _dir, const float _fallOffStart, const float _fallOffEnd, const float _innerConeAngle, const float _outerConeAngle)
{
    float h        = _fallOffEnd;
    float t        = std::tanf(_outerConeAngle);
    float w        = t * h;
    Vec3  pos      = _pos + _dir * h * 0.5f;
    Quat  q        = Quat::FromToRotation(Vec3::Up, _dir);
    Mat4  lightMat = Mat4::CreateScale(Vec3(w, h, w)) * Mat4::CreateFromQuaternion(q) * Mat4::CreateTranslation(pos);
    g_basicRendererContext.UploadTransform(lightMat);
    g_basicRendererContext.UploadSpotLight(_radiance, _lightIntensity, _pos, _dir, _fallOffStart, _fallOffEnd, _innerConeAngle, _outerConeAngle, false);
    g_basicRendererContext.lightVolumeUnitCone.Submit();
}

void BasicRenderer::SubmitSpotLightWithShadow(const Vec3& _radiance, const float _lightIntensity, const Vec3& _pos, const Vec3& _dir, const float _fallOffStart, const float _fallOffEnd, const float _innerConeAngle, const float _outerConeAngle, const TextureHandle _shadowMap)
{
    JAM_ASSERT(!JAM_IS_VALID_HANDLE(_shadowMap), "Not implemented yet");
    Graphics::SetTexture(_shadowMap, eShader::PixelShader, t_shadowSlot);
    SubmitSpotLight(_radiance, _lightIntensity, _pos, _dir, _fallOffStart, _fallOffEnd, _innerConeAngle, _outerConeAngle);
}

void BasicRenderer::ClearRenderGraph()
{
    g_basicRendererContext.graph = RenderGraph {};
}

void BasicRenderer::CompileRenderGraph()
{
    g_basicRendererContext.graph.Compile();
}

void BasicRenderer::AddPass(const RenderPass& _pass)
{
    g_basicRendererContext.graph.AddPass(_pass);
}

TextureHandle BasicRenderer::CreateTexture2D(const Int32 _width, const Int32 _height, const eFormat _format, const bool _bRTVAttachable, const bool _bDSVAttachable)
{
    eTextureCreateFlags flags = eTextureCreateFlags_None;
    if (_bRTVAttachable) flags |= eTextureCreateFlags_RenderTarget;
    if (_bDSVAttachable) flags |= eTextureCreateFlags_DepthStencil;
    return Graphics::CreateTexture2D(_width, _height, _format, 1, flags);
}

TextureHandle BasicRenderer::CreateTextureCube(const Int32 _width, const Int32 _height, const eFormat _format, const bool _bRTVAttachable, const bool _bDSVAttachable)
{
    eTextureCreateFlags flags = eTextureCreateFlags_None;
    if (_bRTVAttachable) flags |= eTextureCreateFlags_RenderTarget;
    if (_bDSVAttachable) flags |= eTextureCreateFlags_DepthStencil;
    return Graphics::CreateTextureCube(_width, _height, _format, 1, flags);
}

TextureHandle BasicRenderer::CreateTextureArray(const Int32 _width, const Int32 _height, const Int32 _arraySize, const eFormat _format, const bool _bRTVAttachable, const bool _bDSVAttachable)
{
    eTextureCreateFlags flags = eTextureCreateFlags_None;
    if (_bRTVAttachable) flags |= eTextureCreateFlags_RenderTarget;
    if (_bDSVAttachable) flags |= eTextureCreateFlags_DepthStencil;
    return Graphics::CreateTexture2D(_width, _height, _format, _arraySize, flags);
}

GBufferDesc BasicRenderer::CreateGBuffer(const Int32 _width, const Int32 _height)
{
    GBufferDesc desc;
    desc.albedoAOTexture                = CreateTexture2D(_width, _height, eFormat::RGBA8_UNorm, true, false);
    desc.normalRoughnessMetallicTexture = CreateTexture2D(_width, _height, eFormat::RGBA16_Float, true, false);
    desc.additionalLightTexture         = CreateTexture2D(_width, _height, eFormat::RGBA16_Float, true, false);
    desc.depthTexture                   = CreateTexture2D(_width, _height, eFormat::D24_UNorm_S8_UInt, false, true);
    return desc;
}

TextureHandle BasicRenderer::CreateShadowMap(const Int32 _sideLength)
{
    return CreateTexture2D(_sideLength, _sideLength, eFormat::D32_Float, false, true);
}

TextureHandle BasicRenderer::CreateShadowCubeMap(const Int32 _sideLength)
{
    return CreateTextureCube(_sideLength, _sideLength, eFormat::D32_Float, false, true);
}

TextureHandle BasicRenderer::CreateShadowCascadeMap(const Int32 _sideLength, const Int32 _cascadeCount)
{
    return CreateTextureArray(_sideLength, _sideLength, _cascadeCount, eFormat::D32_Float, false, true);
}

TextureHandle BasicRenderer::CreateSSAOTexture(const Int32 _width, const Int32 _height)
{
    return CreateTexture2D(_width, _height, eFormat::R8_UNorm, true, false);
}

TextureHandle BasicRenderer::CreatePostProcessTexture(const Int32 _width, const Int32 _height, const bool _bHdr)
{
    eFormat format = _bHdr ? eFormat::RGBA16_Float : eFormat::RGBA8_UNorm;
    return CreateTexture2D(_width, _height, format, true, false);
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
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(_desc.executor);

    return pass;
}

RenderPass BasicRenderer::CreatePBRLightPass(const PBRLightPassDesc& _desc)
{
    const Environment* pEnv        = g_basicRendererContext.pCurEnv;
    const GBufferDesc& gBufferDesc = _desc.gBufferDesc;

    TextureHandle irrMapTexh  = pEnv->irradianceMap ? pEnv->irradianceMap->GetHandle() : k_invalidHandle;
    TextureHandle specMapTexh = pEnv->specularMap ? pEnv->specularMap->GetHandle() : k_invalidHandle;
    TextureHandle brdfLUTTexh = pEnv->brdfLUT ? pEnv->brdfLUT->GetHandle() : k_invalidHandle;

    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.pbrLightingProgram)
        .AddSampler(g_basicRendererContext.pointClampSampler, eShader::PixelShader, s_pointClampSlot)
        .AddSampler(g_basicRendererContext.linearWrapSampler, eShader::PixelShader, s_linearWrapSlot)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddConstantBuffer(g_basicRendererContext.cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
        .AddConstantBuffer(g_basicRendererContext.environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
        .AddTexture(irrMapTexh, eShader::PixelShader, t_irradianceSlot)
        .AddTexture(specMapTexh, eShader::PixelShader, t_specularSlot)
        .AddTexture(brdfLUTTexh, eShader::PixelShader, t_brdfSlot)
        .AddTexture(gBufferDesc.normalRoughnessMetallicTexture, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot)
        .AddTexture(gBufferDesc.albedoAOTexture, eShader::PixelShader, t_gBufferAlbedoAOSlot)
        .AddTexture(gBufferDesc.additionalLightTexture, eShader::PixelShader, t_gBufferAdditionalLightSlot)
        .AddTexture(gBufferDesc.depthTexture, eShader::PixelShader, t_depthSlot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(FullScreenTriExecutor {});

    if (JAM_IS_VALID_HANDLE(_desc.ssaoTexture))
        pass.AddTexture(_desc.ssaoTexture, eShader::PixelShader, t_ssaoSlot);

    return pass;
}

RenderPass BasicRenderer::CreatePBRLightVolumePass(const PBRLightVolumePassDesc& _desc)
{
    const GBufferDesc& gBufferDesc = _desc.gBufferDesc;

    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.pbrLightVolumeProgram)
        .AddSampler(g_basicRendererContext.pointClampSampler, eShader::PixelShader, s_pointClampSlot)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddConstantBuffer(g_basicRendererContext.transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT)
        .AddConstantBuffer(g_basicRendererContext.cameraCbh, eShader::VertexShader, CB_CAMERA_SLOT)
        .AddConstantBuffer(g_basicRendererContext.cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
        .AddConstantBuffer(g_basicRendererContext.lightCbh, eShader::PixelShader, CB_LIGHT_SLOT)
        .AddTexture(gBufferDesc.normalRoughnessMetallicTexture, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot)
        .AddTexture(gBufferDesc.albedoAOTexture, eShader::PixelShader, t_gBufferAlbedoAOSlot)
        .AddTexture(gBufferDesc.additionalLightTexture, eShader::PixelShader, t_gBufferAdditionalLightSlot)
        .AddTexture(gBufferDesc.depthTexture, eShader::PixelShader, t_depthSlot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(g_basicRendererContext.lightVolumeState)
        .SetBlendState(0, BlendState::k_additive)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0);

    return pass;
}

RenderPass BasicRenderer::CreatePBREnvironmentLightVolumePass(const PBREnvironmentLightVolumePassDesc& _desc)
{
    struct Executor
    {
        void operator()() const
        {
            g_basicRendererContext.lightVolumeFullScreenTri.Submit();
        }
    };

    const GBufferDesc& gBufferDesc = _desc.gBufferDesc;

    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.pbrLightVolumeProgram)
        .AddSampler(g_basicRendererContext.pointClampSampler, eShader::PixelShader, s_pointClampSlot)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddConstantBuffer(g_basicRendererContext.cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
        .AddConstantBuffer(g_basicRendererContext.environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
        .AddTexture(_desc.shadowMap, eShader::PixelShader, t_shadowSlot)
        .AddTexture(gBufferDesc.albedoAOTexture, eShader::PixelShader, t_gBufferAlbedoAOSlot)
        .AddTexture(gBufferDesc.normalRoughnessMetallicTexture, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot)
        .AddTexture(gBufferDesc.additionalLightTexture, eShader::PixelShader, t_gBufferAdditionalLightSlot)
        .AddTexture(gBufferDesc.depthTexture, eShader::PixelShader, t_depthSlot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(g_basicRendererContext.lightVolumeState)
        .SetBlendState(0, BlendState::k_additive)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(Executor {});

    return pass;
}

RenderPass BasicRenderer::CreateSkyboxPass(const SkyboxPassDesc& _desc)
{
    struct SkyboxExecutor
    {
        void operator()() const
        {
            g_basicRendererContext.skyboxCube.Submit();
        }
    };

    const Environment* pEnv       = g_basicRendererContext.pCurEnv;
    TextureHandle      skyboxTexh = pEnv->skyboxTexture ? pEnv->skyboxTexture->GetHandle() : k_invalidHandle;

    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.skyboxProgram)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddConstantBuffer(g_basicRendererContext.cameraCbh, eShader::VertexShader, CB_CAMERA_SLOT)
        .AddTexture(skyboxTexh, eShader::PixelShader, t_skyboxSlot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(g_basicRendererContext.skyboxState)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(SkyboxExecutor {});

    return pass;
}

RenderPass BasicRenderer::CreateShadowCasterPass(const ShadowCasterPassDesc& _desc)
{
    RenderPass pass;
    pass.AddConstantBuffer(g_basicRendererContext.transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT)
        .AddConstantBuffer(g_basicRendererContext.shadowCasterCbh, eShader::VertexShader, CB_SHADOW_CASTER_SLOT)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(g_basicRendererContext.depthTestState)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(_desc.executor);

    return pass;
}

RenderPass BasicRenderer::CreateOmniDirectionalShadowCasterPass(const ShadowCasterPassDesc& _desc)
{
    RenderPass pass;
    pass.AddConstantBuffer(g_basicRendererContext.transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT)
        .AddConstantBuffer(g_basicRendererContext.osmCasterCbh, eShader::GeometryShader, CB_OMNIDIRECTIONAL_SHADOW_CASTER_SLOT)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(g_basicRendererContext.depthTestState)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(_desc.executor)
        .SetProgram(g_basicRendererContext.osmCasterProgram);

    return pass;
}

RenderPass BasicRenderer::CreateCascadeShadowCasterPass(const ShadowCasterPassDesc& _desc)
{
    RenderPass pass;
    pass.AddConstantBuffer(g_basicRendererContext.transformCbh, eShader::VertexShader, CB_TRANSFORM_SLOT)
        .AddConstantBuffer(g_basicRendererContext.csmCasterCbh, eShader::GeometryShader, CB_CASCADE_SHADOW_CASTER_SLOT)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(g_basicRendererContext.depthTestState)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(_desc.executor);

    eCascadeLevel cascadeLevel = g_basicRendererContext.pCurEnv->shadowCascadeLevel;
    if (cascadeLevel == eCascadeLevel::level2)
        pass.SetProgram(g_basicRendererContext.csmLevel2CasterProgram);
    else if (cascadeLevel == eCascadeLevel::level3)
        pass.SetProgram(g_basicRendererContext.csmLevel3CasterProgram);
    else if (cascadeLevel == eCascadeLevel::level4)
        pass.SetProgram(g_basicRendererContext.csmLevel4CasterProgram);

    return pass;
}

RenderPass BasicRenderer::CreateSSAOPass(const SSAOPassDesc& _desc)
{
    const GBufferDesc& gBufferDesc = _desc.gBufferDesc;

    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.ssaoProgram)
        .AddSampler(g_basicRendererContext.pointClampSampler, eShader::PixelShader, s_pointClampSlot)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearWrapSlot)
        .AddConstantBuffer(g_basicRendererContext.cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
        .AddConstantBuffer(g_basicRendererContext.ssaoCbh, eShader::PixelShader, CB_SSAO_KERNEL_SLOT)
        .AddConstantBuffer(g_basicRendererContext.environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
        .AddTexture(gBufferDesc.normalRoughnessMetallicTexture, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot)
        .AddTexture(gBufferDesc.depthTexture, eShader::PixelShader, t_depthSlot)
        .AddTexture(g_basicRendererContext.ssaoNoiseTexture, eShader::PixelShader, t_ssaoNoiseSlot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(FullScreenTriExecutor {});

    return pass;
}

RenderPass BasicRenderer::CreateSSAOBBlurPass(const SSAOBlurPassDesc& _desc)
{
    RenderPass pass;
    pass.SetProgram(_desc.bHorizontal ? g_basicRendererContext.ssaoBlurXProgram : g_basicRendererContext.ssaoBlurYProgram)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddTexture(_desc.ssaoTexture, eShader::PixelShader, t_ssaoSlot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(FullScreenTriExecutor {});

    return pass;
}

RenderPass BasicRenderer::CreateBlurPass(const BlurPassDesc& _desc)
{
    RenderPass pass;
    pass.SetProgram(_desc.bHorizontal ? g_basicRendererContext.bloomXProgram : g_basicRendererContext.bloomYProgram)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input0Slot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(FullScreenTriExecutor {});

    return pass;
}

RenderPass BasicRenderer::CreateCombinePass(const CombinePassDesc& _desc)
{
    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.bloomCombineProgram)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddTexture(_desc.inputTexture0, eShader::PixelShader, t_input0Slot)
        .AddTexture(_desc.inputTexture1, eShader::PixelShader, t_input1Slot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(FullScreenTriExecutor {});

    return pass;
}

RenderPass BasicRenderer::CreateToneMappingPass(const ToneMappingPassDesc& _desc)
{
    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.toneMappingProgram)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input0Slot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(FullScreenTriExecutor {});

    return pass;
}

RenderPass BasicRenderer::CreateFXAAPass(const FXAAPassDesc& _desc)
{
    RenderPass pass;
    pass.AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input0Slot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(FullScreenTriExecutor {});

    eFxaaQuality quality = g_basicRendererContext.pCurEnv->fxaaQuality;
    switch (quality)
    {
        case eFxaaQuality::VeryLow:
            pass.SetProgram(g_basicRendererContext.fxaaVeryLowProgram);
            break;
        case eFxaaQuality::Low:
            pass.SetProgram(g_basicRendererContext.fxaaLowProgram);
            break;
        case eFxaaQuality::Medium:
            pass.SetProgram(g_basicRendererContext.fxaaMediumProgram);
            break;
        case eFxaaQuality::High:
            pass.SetProgram(g_basicRendererContext.fxaaHighProgram);
            break;
        case eFxaaQuality::VeryHigh:
            pass.SetProgram(g_basicRendererContext.fxaaVeryHighProgram);
            break;
        case eFxaaQuality::Ultra:
            pass.SetProgram(g_basicRendererContext.fxaaUltraProgram);
            break;
        default:
            JAM_ASSERT(false, "Invalid FXAA quality");
            break;
    }

    return pass;
}

RenderPass BasicRenderer::CreateHeightFogPass(const HeightFogPass& _desc)
{
    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.fogProgram)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddConstantBuffer(g_basicRendererContext.cameraCbh, eShader::PixelShader, CB_CAMERA_SLOT)
        .AddConstantBuffer(g_basicRendererContext.environmentCbh, eShader::PixelShader, CB_ENVIRONMENT_SLOT)
        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input0Slot)
        .AddTexture(_desc.depthTexture, eShader::PixelShader, t_depthSlot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(FullScreenTriExecutor {});

    return pass;
}

RenderPass BasicRenderer::CreatePassThroughPass(const PassThroughPassDesc& _desc)
{
    RenderPass pass;
    pass.SetProgram(g_basicRendererContext.passThroughProgram)
        .AddSampler(g_basicRendererContext.linearClampSampler, eShader::PixelShader, s_linearClampSlot)
        .AddTexture(_desc.inputTexture, eShader::PixelShader, t_input0Slot)
        .SetFrameBuffer(_desc.frameBuffer)
        .SetViewport(_desc.viewport)
        .SetGfxState(GfxState::k_default)
        .SetBlendState(0, BlendState::k_opaque)
        .SetStencil(StencilState::k_disable, StencilState::k_disable, 0)
        .SetExecutor(FullScreenTriExecutor {});

    return pass;
}

}   // namespace jam