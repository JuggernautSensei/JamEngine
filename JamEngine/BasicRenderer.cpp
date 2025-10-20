#include "pch.h"

#include "BasicRenderer.h"

#include "BasicRendererBase.h"
#include "Camera.h"
#include "CullingSystem.h"
#include "DrawItem.h"
#include "EmbeddedShaderGlue.h"
#include "Environment.h"
#include "Gfx.h"
#include "GfxBase.h"
#include "Light.h"
#include "Material.h"
#include "Memory.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include "Profile.h"
#include "Shaders/PrecompiledShaders/PrecompiledJamShader.h"
#include "Transform.h"
using namespace jam;

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

struct RendererContext
{
    void UploadTransform(const Mat4& _world) const;
    void UploadTransform(const Transform& _transform) const;
    void UploadMaterial(const Material& _material) const;
    void UploadPointLight(const PointLight& _light, const Transform& _transform) const;
    void UploadSpotLight(const SpotLight& _light, const Transform& _transform) const;
    void UploadCamera(const Vec3& _eyePos, const Mat4& _view, const Mat4& _proj) const;
    void UploadEnvironment(const Environment& _env) const;

    void SetupGBufferTexture(const GBufferParam& _gBufferParam) const;

    void BakeSSAOKernel() const;
    void BakeSSAONoise();

    void Begin(const Vec3& _eyePos, const Mat4& _view, const Mat4& _proj, const Frustum& _frustumInW, const Environment& _env);

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

    ConstantBufferHandle cbTransform;
    ConstantBufferHandle cbCamera;
    ConstantBufferHandle cbMaterial;
    ConstantBufferHandle cbLight;
    ConstantBufferHandle cbEnvironment;
    ConstantBufferHandle cbShadowCaster;
    ConstantBufferHandle cbCsmCaster;
    ConstantBufferHandle cbOsmCaster;
    ConstantBufferHandle cbSSAO;

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

    Frustum       viewFrustumInW;
    CullingSystem cullingSystem;
    Timer         timer;
    bool          bBeginCalled = false;

    const Environment* pCurEnv               = nullptr;
    eSSAOKernel        curSSAOKernelSize     = eSSAOKernel::Kernel4x4;
    bool               bIsSSAOKernelUploaded = false;
} g_ctx;

void RendererContext::BakeSSAOKernel() const
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

    gfx::UploadBuffer(cbSSAO, MemoryView { cb });
}

void RendererContext::BakeSSAONoise()
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

    ssaoNoiseTexture = gfx::CreateTexture2D(noiseSize, noiseSize, eFormat::RGBA16_Float, 1, eTextureCreateFlags_None, mem);
}

void RendererContext::UploadTransform(const Mat4& _world) const
{
    CB_TRANSFORM cb;
    cb.cb_world         = _world.Transpose();
    cb.cb_worldInvTrans = _world.Invert();
    gfx::UploadBuffer(cbTransform, MemoryView { cb });
}

void RendererContext::UploadTransform(const Transform& _transform) const
{
    Mat4 world = _transform.CreateWorld();

    CB_TRANSFORM cb;
    cb.cb_world         = world.Transpose();
    cb.cb_worldInvTrans = world.Invert();
    gfx::UploadBuffer(cbTransform, MemoryView { cb });
}

void RendererContext::UploadMaterial(const Material& _material) const
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

    gfx::UploadBuffer(cbMaterial, MemoryView { cb });
}

void RendererContext::UploadPointLight(const PointLight& _light, const Transform& _transform) const
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
    gfx::UploadBuffer(cbLight, MemoryView { cb });
}

void RendererContext::UploadSpotLight(const SpotLight& _light, const Transform& _transform) const
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
    gfx::UploadBuffer(cbLight, MemoryView { cb });
}

void RendererContext::UploadCamera(const Vec3& _eyePos, const Mat4& _view, const Mat4& _proj) const
{
    CB_CAMERA cb      = {};
    cb.cb_eyePos      = _eyePos;
    cb.cb_view        = _view.Transpose();
    cb.cb_proj        = _proj.Transpose();
    cb.cb_projViewInv = (_view * _proj).Invert().Transpose();
    gfx::UploadBuffer(cbCamera, MemoryView { cb });
}

void RendererContext::UploadEnvironment(const Environment& _env) const
{
    CB_ENVIRONMENT cb;
    cb.cb_envFlags = JAM_ENVIRONMENT_FLAGS_NONE;

    cb.cb_sunDirection = _env.sunDirection;
    cb.cb_sunIntensity = _env.sunIntensity;
    cb.cb_sunColor     = _env.sunColor;

    if (_env.shadowCascadeLevel == eCascadeLevel::level2)
    {
        cb.cb_shadowCascadeLevel = 2;
    }
    else if (_env.shadowCascadeLevel == eCascadeLevel::level3)
    {
        cb.cb_shadowCascadeLevel = 3;
    }
    else if (_env.shadowCascadeLevel == eCascadeLevel::level4)
    {
        cb.cb_shadowCascadeLevel = 4;
    }

    for (Int32 i = 0; i < 4; ++i)
    {
        cb.cb_shadowCascadeRages[i] = _env.shadowCascadeRanges[i];
    }

    cb.cb_shadowBias = _env.shadowBias;

    cb.cb_irradianceMapIntensity = _env.irradianceMapIntensity;
    cb.cb_specularMapIntensity   = _env.specularMapIntensity;
    cb.cb_skyBoxColor            = _env.skyboxColor;

    if (_env.ssaoKernelSize == eSSAOKernel::Kernel4x4)
    {
        cb.cb_ssaoKernelSize = 16;
    }
    else if (_env.ssaoKernelSize == eSSAOKernel::Kernel6x6)
    {
        cb.cb_ssaoKernelSize = 36;
    }
    else if (_env.ssaoKernelSize == eSSAOKernel::Kernel8x8)
    {
        cb.cb_ssaoKernelSize = 64;
    }
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
    {
        cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SUN_SHADOW;
    }
    if (_env.bSSAOEnabled)
    {
        cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SSAO;
    }

    if (_env.skyboxType == eSkyboxType::Cubemap)
    {
        cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SKYBOX_CUBEMAP;
    }
    else if (_env.skyboxType == eSkyboxType::SolidColor)
    {
        cb.cb_envFlags |= JAM_ENVIRONMENT_FLAGS_SKYBOX_SOLID_COLOR;
    }

    gfx::UploadBuffer(cbEnvironment, MemoryView { cb });
}

void RendererContext::SetupGBufferTexture(const GBufferParam& _gBufferParam) const
{
    gfx::SetTexture(_gBufferParam.albedoAoTex, eShader::PixelShader, t_gBufferAlbedoAOSlot);
    gfx::SetTexture(_gBufferParam.normalRoughnessMetallicTex, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot);
    gfx::SetTexture(_gBufferParam.additionalLightTex, eShader::PixelShader, t_gBufferAdditionalLightSlot);
    gfx::SetTexture(_gBufferParam.depthTex, eShader::PixelShader, t_depthSlot);
}

void RendererContext::Begin(const Vec3& _eyePos, const Mat4& _view, const Mat4& _proj, const Frustum& _frustumInW, const Environment& _env)
{
    JAM_ASSERT(bBeginCalled == false, "Begin() called without calling End() first.");

    timer.Reset();
    timer.Start();

    // 카메라 정보 업로드
    viewFrustumInW = _frustumInW;
    UploadCamera(_eyePos, _view, _proj);

    // 환경 정보 업로드
    g_ctx.pCurEnv = &_env;

    // SSAO 커널 크기 변경 감지
    if (_env.bSSAOEnabled && (!bIsSSAOKernelUploaded || g_ctx.curSSAOKernelSize != _env.ssaoKernelSize))
    {
        BakeSSAOKernel();
        g_ctx.curSSAOKernelSize     = _env.ssaoKernelSize;
        g_ctx.bIsSSAOKernelUploaded = true;
    }

    UploadEnvironment(_env);
    bBeginCalled = true;
}

//////////////////////////////////

namespace jam::basic_renderer
{

void Init()
{
    JAM_PROFILE_SCOPE_MS("RendererSystem::Init");

    // vertex shaders
    {
        g_ctx.pbrVS = gfx::CreateShader(MemoryView { shader::PBRVS_Bytecode, shader::PBRVS_Size });
        gfx::SetShaderName(g_ctx.pbrVS, "jam engine predefined shader: pbrVS");

        g_ctx.fullscreenVS = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(g_ctx.fullscreenVS, "jam engine predefined shader: fullscreenVS");

        g_ctx.pbrLightVolumeVS = gfx::CreateShader(MemoryView { shader::PBRLightVolumeShader_VS_Bytecode, shader::PBRLightVolumeShader_VS_Size });
        gfx::SetShaderName(g_ctx.pbrLightVolumeVS, "jam engine predefined shader: pbrLightVolumeVS");

        g_ctx.pbrEnvironmentLIghtVolumeVS = gfx::CreateShader(MemoryView { shader::PBRLightVolumeShaderForDirectional_VS_Bytecode, shader::PBRLightVolumeShaderForDirectional_VS_Size });
        gfx::SetShaderName(g_ctx.pbrEnvironmentLIghtVolumeVS, "jam engine predefined shader: pbrEnvironmentLightVolumeVS");

        g_ctx.csmVS = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_VS_Bytecode, shader::CascadeShaderMappingShader_VS_Size });
        gfx::SetShaderName(g_ctx.csmVS, "jam engine predefined shader: csmVS");

        g_ctx.osmVS = gfx::CreateShader(MemoryView { shader::OmniDirectionalShaderMappingShader_VS_Bytecode, shader::OmniDirectionalShaderMappingShader_VS_Size });
        gfx::SetShaderName(g_ctx.osmVS, "jam engine predefined shader: osmVS");

        g_ctx.shadowMappingVS = gfx::CreateShader(MemoryView { shader::ShadowMappingShader_VS_Bytecode, shader::ShadowMappingShader_VS_Size });
        gfx::SetShaderName(g_ctx.shadowMappingVS, "jam engine predefined shader: shadowMappingVS");

        g_ctx.skyboxVS = gfx::CreateShader(MemoryView { shader::SkyboxShader_VS_Bytecode, shader::SkyboxShader_VS_Size });
        gfx::SetShaderName(g_ctx.skyboxVS, "jam engine predefined shader: skyboxVS");
    }

    // geometry shaders
    {
        g_ctx.csmLevel2GS = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_2_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_2_GS_Size });
        gfx::SetShaderName(g_ctx.csmLevel2GS, "jam engine predefined shader: csmLevel2GS");

        g_ctx.csmLevel3GS = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Size });
        gfx::SetShaderName(g_ctx.csmLevel3GS, "jam engine predefined shader: csmLevel3GS");

        g_ctx.csmLevel4GS = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_4_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_4_GS_Size });
        gfx::SetShaderName(g_ctx.csmLevel4GS, "jam engine predefined shader: csmLevel4GS");

        g_ctx.osmGS = gfx::CreateShader(MemoryView { shader::OmniDirectionalShaderMappingShader_GS_Bytecode, shader::OmniDirectionalShaderMappingShader_GS_Size });
        gfx::SetShaderName(g_ctx.osmGS, "jam engine predefined shader: osmGS");
    }

    // pixel shaders
    {
        g_ctx.pbrGBufferPS = gfx::CreateShader(MemoryView { shader::PBRGBufferShader_PS_Bytecode, shader::PBRGBufferShader_PS_Size });
        gfx::SetShaderName(g_ctx.pbrGBufferPS, "jam engine predefined shader: pbrGBufferPS");

        g_ctx.pbrLightingPS = gfx::CreateShader(MemoryView { shader::PBRLightingShader_PS_Bytecode, shader::PBRLightingShader_PS_Size });
        gfx::SetShaderName(g_ctx.pbrLightingPS, "jam engine predefined shader: pbrLightingPS");

        g_ctx.pbrLightVolumePS = gfx::CreateShader(MemoryView { shader::PBRLightVolumeShader_PS_Bytecode, shader::PBRLightVolumeShader_PS_Size });
        gfx::SetShaderName(g_ctx.pbrLightVolumePS, "jam engine predefined shader: pbrLightVolumePS");

        g_ctx.pbrEnvironmentLightVolumePS = gfx::CreateShader(MemoryView { shader::PBRLightingShader_PS_Bytecode, shader::PBRLightingShader_PS_Size });
        gfx::SetShaderName(g_ctx.pbrEnvironmentLightVolumePS, "jam engine predefined shader: pbrEnvironmentLightVolumePS");

        g_ctx.csmPS = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_PS_Bytecode, shader::CascadeShaderMappingShader_PS_Size });
        gfx::SetShaderName(g_ctx.csmPS, "jam engine predefined shader: csmPS");

        g_ctx.osmPS = gfx::CreateShader(MemoryView { shader::OmniDirectionalShaderMappingShader_PS_Bytecode, shader::OmniDirectionalShaderMappingShader_PS_Size });
        gfx::SetShaderName(g_ctx.osmPS, "jam engine predefined shader: osmPS");

        g_ctx.shadowMappingPS = gfx::CreateShader(MemoryView { shader::ShadowMappingShader_PS_Bytecode, shader::ShadowMappingShader_PS_Size });
        gfx::SetShaderName(g_ctx.shadowMappingPS, "jam engine predefined shader: shadowMappingPS");

        g_ctx.skyboxPS = gfx::CreateShader(MemoryView { shader::SkyboxShader_PS_Bytecode, shader::SkyboxShader_PS_Size });
        gfx::SetShaderName(g_ctx.skyboxPS, "jam engine predefined shader: skyboxPS");

        g_ctx.ssaoPS = gfx::CreateShader(MemoryView { shader::SSAOPS_Bytecode, shader::SSAOPS_Size });
        gfx::SetShaderName(g_ctx.ssaoPS, "jam engine predefined shader: ssaoPS");

        g_ctx.ssaoBlurXPS = gfx::CreateShader(MemoryView { shader::SSAOBlurXPS_Bytecode, shader::SSAOBlurXPS_Size });
        gfx::SetShaderName(g_ctx.ssaoBlurXPS, "jam engine predefined shader: ssaoBlurXPS");

        g_ctx.ssaoBlurYPS = gfx::CreateShader(MemoryView { shader::SSAOBlurYPS_Bytecode, shader::SSAOBlurYPS_Size });
        gfx::SetShaderName(g_ctx.ssaoBlurYPS, "jam engine predefined shader: ssaoBlurYPS");

        g_ctx.bloomXPS = gfx::CreateShader(MemoryView { shader::BloomXPS_Bytecode, shader::BloomXPS_Size });
        gfx::SetShaderName(g_ctx.bloomXPS, "jam engine predefined shader: bloomXPS");

        g_ctx.bloomYPS = gfx::CreateShader(MemoryView { shader::BloomYPS_Bytecode, shader::BloomYPS_Size });
        gfx::SetShaderName(g_ctx.bloomYPS, "jam engine predefined shader: bloomYPS");

        g_ctx.bloomCombinePS = gfx::CreateShader(MemoryView { shader::BloomCombinePS_Bytecode, shader::BloomCombinePS_Size });
        gfx::SetShaderName(g_ctx.bloomCombinePS, "jam engine predefined shader: bloomCombinePS");

        g_ctx.toneMappingUncharted2PS = gfx::CreateShader(MemoryView { shader::ToneMappingPS_UNCHARTED2_Bytecode, shader::ToneMappingPS_UNCHARTED2_Size });
        gfx::SetShaderName(g_ctx.toneMappingUncharted2PS, "jam engine predefined shader: toneMappingUncharted2PS");

        g_ctx.toneMappingLinearPS = gfx::CreateShader(MemoryView { shader::ToneMappingPS_LINEAR_Bytecode, shader::ToneMappingPS_LINEAR_Size });
        gfx::SetShaderName(g_ctx.toneMappingLinearPS, "jam engine predefined shader: toneMappingLinearPS");

        g_ctx.toneMappingReinhardPS = gfx::CreateShader(MemoryView { shader::ToneMappingPS_REINHARD_Bytecode, shader::ToneMappingPS_REINHARD_Size });
        gfx::SetShaderName(g_ctx.toneMappingReinhardPS, "jam engine predefined shader: toneMappingReinhardPS");

        g_ctx.toneMappingLumaBasedReinhardPS = gfx::CreateShader(MemoryView { shader::ToneMappingPS_LUMA_BASED_REINHARD_Bytecode, shader::ToneMappingPS_LUMA_BASED_REINHARD_Size });
        gfx::SetShaderName(g_ctx.toneMappingLumaBasedReinhardPS, "jam engine predefined shader: toneMappingLumaBasedReinhardPS");

        g_ctx.toneMappingWhitePreservingReinhardPS = gfx::CreateShader(MemoryView { shader::ToneMappingPS_WHITE_PRESERVING_REINHARD_Bytecode, shader::ToneMappingPS_WHITE_PRESERVING_REINHARD_Size });
        gfx::SetShaderName(g_ctx.toneMappingWhitePreservingReinhardPS, "jam engine predefined shader: toneMappingWhitePreservingReinhardPS");

        g_ctx.toneMappingRombDaHousePS = gfx::CreateShader(MemoryView { shader::ToneMappingPS_ROMB_DA_HOUSE_Bytecode, shader::ToneMappingPS_ROMB_DA_HOUSE_Size });
        gfx::SetShaderName(g_ctx.toneMappingRombDaHousePS, "jam engine predefined shader: toneMappingRombDaHousePS");

        g_ctx.toneMappingFilmicPS = gfx::CreateShader(MemoryView { shader::ToneMappingPS_FILMIC_Bytecode, shader::ToneMappingPS_FILMIC_Size });
        gfx::SetShaderName(g_ctx.toneMappingFilmicPS, "jam engine predefined shader: toneMappingFilmicPS");

        g_ctx.fxaaVeryLowPS = gfx::CreateShader(MemoryView { shader::FXAA_VeryLow_PS_Bytecode, shader::FXAA_VeryLow_PS_Size });
        gfx::SetShaderName(g_ctx.fxaaVeryLowPS, "jam engine predefined shader: fxaaVeryLowPS");

        g_ctx.fxaaLowPS = gfx::CreateShader(MemoryView { shader::FXAA_VeryLow_PS_Bytecode, shader::FXAA_VeryLow_PS_Size });
        gfx::SetShaderName(g_ctx.fxaaLowPS, "jam engine predefined shader: fxaaLowPS");

        g_ctx.fxaaMediumPS = gfx::CreateShader(MemoryView { shader::FXAA_Medium_PS_Bytecode, shader::FXAA_Medium_PS_Size });
        gfx::SetShaderName(g_ctx.fxaaMediumPS, "jam engine predefined shader: fxaaMediumPS");

        g_ctx.fxaaHighPS = gfx::CreateShader(MemoryView { shader::FXAA_High_PS_Bytecode, shader::FXAA_High_PS_Size });
        gfx::SetShaderName(g_ctx.fxaaHighPS, "jam engine predefined shader: fxaaHighPS");

        g_ctx.fxaaVeryHighPS = gfx::CreateShader(MemoryView { shader::FXAA_VeryHigh_PS_Bytecode, shader::FXAA_VeryHigh_PS_Size });
        gfx::SetShaderName(g_ctx.fxaaVeryHighPS, "jam engine predefined shader: fxaaVeryHighPS");

        g_ctx.fxaaUltraPS = gfx::CreateShader(MemoryView { shader::FXAA_Ultra_PS_Bytecode, shader::FXAA_Ultra_PS_Size });
        gfx::SetShaderName(g_ctx.fxaaUltraPS, "jam engine predefined shader: fxaaUltraPS");

        g_ctx.fogPS = gfx::CreateShader(MemoryView { shader::FogPS_Bytecode, shader::FogPS_Size });
        gfx::SetShaderName(g_ctx.fogPS, "jam engine predefined shader: fogPS");

        g_ctx.passThroughPS = gfx::CreateShader(MemoryView { shader::PassThroughPS_Bytecode, shader::PassThroughPS_Size });
        gfx::SetShaderName(g_ctx.passThroughPS, "jam engine predefined shader: passThroughPS");
    }

    // programs
    {
        g_ctx.pbrGBufferProgram                         = gfx::CreateProgram(g_ctx.pbrVS, g_ctx.pbrGBufferPS);
        g_ctx.pbrLightingProgram                        = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.pbrLightingPS);
        g_ctx.pbrLightVolumeProgram                     = gfx::CreateProgram(g_ctx.pbrLightVolumeVS, g_ctx.pbrLightVolumePS);
        g_ctx.pbrEnvironmentLightVolumeProgram          = gfx::CreateProgram(g_ctx.pbrEnvironmentLIghtVolumeVS, g_ctx.pbrEnvironmentLightVolumePS);
        g_ctx.csmLevel2CasterProgram                    = gfx::CreateProgram(g_ctx.csmVS, g_ctx.csmPS, g_ctx.csmLevel2GS, k_invalidHandle, k_invalidHandle);
        g_ctx.csmLevel3CasterProgram                    = gfx::CreateProgram(g_ctx.csmVS, g_ctx.csmPS, g_ctx.csmLevel3GS, k_invalidHandle, k_invalidHandle);
        g_ctx.csmLevel4CasterProgram                    = gfx::CreateProgram(g_ctx.csmVS, g_ctx.csmPS, g_ctx.csmLevel4GS, k_invalidHandle, k_invalidHandle);
        g_ctx.osmCasterProgram                          = gfx::CreateProgram(g_ctx.osmVS, g_ctx.osmPS, g_ctx.osmGS, k_invalidHandle, k_invalidHandle);
        g_ctx.shadowCasterProgram                       = gfx::CreateProgram(g_ctx.shadowMappingVS, g_ctx.shadowMappingPS);
        g_ctx.skyboxProgram                             = gfx::CreateProgram(g_ctx.skyboxVS, g_ctx.skyboxPS);
        g_ctx.ssaoProgram                               = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.ssaoPS);
        g_ctx.ssaoBlurXProgram                          = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.ssaoBlurXPS);
        g_ctx.ssaoBlurYProgram                          = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.ssaoBlurYPS);
        g_ctx.bloomXProgram                             = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.bloomXPS);
        g_ctx.bloomYProgram                             = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.bloomYPS);
        g_ctx.bloomCombineProgram                       = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.bloomCombinePS);
        g_ctx.toneMappingUncharted2Program              = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.toneMappingUncharted2PS);
        g_ctx.toneMappingLinearProgram                  = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.toneMappingLinearPS);
        g_ctx.toneMappingReinhardProgram                = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.toneMappingReinhardPS);
        g_ctx.toneMappingLumaBasedReinhardProgram       = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.toneMappingLumaBasedReinhardPS);
        g_ctx.toneMappingWhitePreservingReinhardProgram = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.toneMappingWhitePreservingReinhardPS);
        g_ctx.toneMappingRombDaHouseProgram             = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.toneMappingRombDaHousePS);
        g_ctx.toneMappingFilmicProgram                  = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.toneMappingFilmicPS);
        g_ctx.fxaaVeryLowProgram                        = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.fxaaVeryLowPS);
        g_ctx.fxaaLowProgram                            = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.fxaaLowPS);
        g_ctx.fxaaMediumProgram                         = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.fxaaMediumPS);
        g_ctx.fxaaHighProgram                           = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.fxaaHighPS);
        g_ctx.fxaaVeryHighProgram                       = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.fxaaVeryHighPS);
        g_ctx.fxaaUltraProgram                          = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.fxaaUltraPS);
        g_ctx.fogProgram                                = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.fogPS);
        g_ctx.passThroughProgram                        = gfx::CreateProgram(g_ctx.fullscreenVS, g_ctx.passThroughPS);
    }

    // constant buffers
    {
        g_ctx.cbTransform = gfx::CreateConstantBuffer(sizeof(CB_TRANSFORM));
        gfx::SetConstantBufferName(g_ctx.cbTransform, "jam engine predefined constant buffer: CB_TRANSFORM");

        g_ctx.cbCamera = gfx::CreateConstantBuffer(sizeof(CB_CAMERA));
        gfx::SetConstantBufferName(g_ctx.cbCamera, "jam engine predefined constant buffer: CB_CAMERA");

        g_ctx.cbMaterial = gfx::CreateConstantBuffer(sizeof(CB_MATERIAL));
        gfx::SetConstantBufferName(g_ctx.cbMaterial, "jam engine predefined constant buffer: CB_MATERIAL");

        g_ctx.cbLight = gfx::CreateConstantBuffer(sizeof(CB_LIGHT));
        gfx::SetConstantBufferName(g_ctx.cbLight, "jam engine predefined constant buffer: CB_LIGHT");

        g_ctx.cbEnvironment = gfx::CreateConstantBuffer(sizeof(CB_ENVIRONMENT));
        gfx::SetConstantBufferName(g_ctx.cbEnvironment, "jam engine predefined constant buffer: CB_GLOBAL");

        g_ctx.cbShadowCaster = gfx::CreateConstantBuffer(sizeof(CB_SHADOW_CASTER));
        gfx::SetConstantBufferName(g_ctx.cbShadowCaster, "jam engine predefined constant buffer: CB_SHADOW_CASTER");

        g_ctx.cbCsmCaster = gfx::CreateConstantBuffer(sizeof(CB_CASCADE_SHADOW_CASTER));
        gfx::SetConstantBufferName(g_ctx.cbCsmCaster, "jam engine predefined constant buffer: CB_CASCADE_SHADOW_CASTER");

        g_ctx.cbOsmCaster = gfx::CreateConstantBuffer(sizeof(CB_OMNIDIRECTIONAL_SHADOW_CASTER));
        gfx::SetConstantBufferName(g_ctx.cbOsmCaster, "jam engine predefined constant buffer: CB_OMNIDIRECTIONAL_SHADOW_CASTER");

        g_ctx.cbSSAO = gfx::CreateConstantBuffer(sizeof(CB_SSAO_KERNEL));
        gfx::SetConstantBufferName(g_ctx.cbSSAO, "jam engine predefined constant buffer: CB_SSAO_KERNEL");
    }

    // full screen tri
    {
        VertexPosTex2D vertices[] = {
            { Vec2(-1.f, -1.f), Vec2(0.f,  1.f) },
            { Vec2(-1.f,  3.f), Vec2(0.f, -1.f) },
            {  Vec2(3.f, -1.f), Vec2(2.f,  1.f) }
        };
        UInt16 indices[] = { 0, 1, 2 };
        g_ctx.fullScreenTri.Load(MemoryView { vertices }, VertexPosTex2D::s_layout, MemoryView { indices });
    }

    // light volume full tri
    {
        VertexPos3D vertices[] = {
            { Vec3(-1.f, -1.f, 0.f) },
            { Vec3(-1.f, 3.f, 0.f) },
            { Vec3(3.f, -1.f, 0.f) }
        };
        UInt16 indices[] = { 0, 1, 2 };
        g_ctx.lightVolumeFullScreenTri.Load(MemoryView { vertices }, VertexPos3D::s_layout, MemoryView { indices });
    }

    // skybox cube
    {
        Vec3 whd         = { 1.f, 1.f, 1.f };
        g_ctx.skyboxCube = CreateBoxMesh(whd, VertexPos3D::s_layout);
    }

    // unit sphere
    {
        g_ctx.lightVolumeUnitSphere = CreateSphereMesh(1.f, 2, VertexPos3D::s_layout);
    }

    // unit corn
    {
        g_ctx.lightVolumeUnitCone = CreateCone(1.f, 1.f, 20, VertexPos3D::s_layout);
    }

    // sampler
    {
        g_ctx.linearWrapSampler.filter   = eTextureFilter::MinLinear_MagLinear_MipLinear;
        g_ctx.linearWrapSampler.addressU = eTextureAddress::Wrap;
        g_ctx.linearWrapSampler.addressV = eTextureAddress::Wrap;
        g_ctx.linearWrapSampler.addressW = eTextureAddress::Wrap;

        g_ctx.linearClampSampler.filter   = eTextureFilter::MinLinear_MagLinear_MipLinear;
        g_ctx.linearClampSampler.addressU = eTextureAddress::Clamp;
        g_ctx.linearClampSampler.addressV = eTextureAddress::Clamp;
        g_ctx.linearClampSampler.addressW = eTextureAddress::Clamp;

        g_ctx.pointWrapSampler.filter   = eTextureFilter::MinPoint_MagPoint_MipPoint;
        g_ctx.pointWrapSampler.addressU = eTextureAddress::Wrap;
        g_ctx.pointWrapSampler.addressV = eTextureAddress::Wrap;
        g_ctx.pointWrapSampler.addressW = eTextureAddress::Wrap;

        g_ctx.pointClampSampler.filter   = eTextureFilter::MinPoint_MagPoint_MipPoint;
        g_ctx.pointClampSampler.addressU = eTextureAddress::Clamp;
        g_ctx.pointClampSampler.addressV = eTextureAddress::Clamp;
        g_ctx.pointClampSampler.addressW = eTextureAddress::Clamp;

        g_ctx.shadowLinearClampSampler.filter      = eTextureFilter::MinLinear_MagLinear_MipLinear;
        g_ctx.shadowLinearClampSampler.addressU    = eTextureAddress::Clamp;
        g_ctx.shadowLinearClampSampler.addressV    = eTextureAddress::Clamp;
        g_ctx.shadowLinearClampSampler.addressW    = eTextureAddress::Clamp;
        g_ctx.shadowLinearClampSampler.compareFunc = eCompareFunc::LessEqual;

        // 후면 컬링, 깊이 테스트 활성화, 깊이 쓰기 활성화
        g_ctx.depthTestState = GfxState::k_depthTest;

        // 전면 컬링, 깊이 비활성화
        g_ctx.lightVolumeState = GfxState { ePrimitive::TriangleList, eCullMode::Front, false, false, false, false, false, false, false, false, eCompareFunc::LessEqual, false };

        // 전면 컬링, 깊이 테스트 활성화, 깊이 쓰기 비활성화
        g_ctx.skyboxState = GfxState { ePrimitive::TriangleList, eCullMode::Front, true, false, false, false, false, false, false, false, eCompareFunc::LessEqual, false };
    }
}

void Shutdown()
{
    JAM_DESTROY_SHADER(g_ctx.pbrVS);
    JAM_DESTROY_SHADER(g_ctx.fullscreenVS);
    JAM_DESTROY_SHADER(g_ctx.pbrLightVolumeVS);
    JAM_DESTROY_SHADER(g_ctx.pbrEnvironmentLIghtVolumeVS);
    JAM_DESTROY_SHADER(g_ctx.csmVS);
    JAM_DESTROY_SHADER(g_ctx.osmVS);
    JAM_DESTROY_SHADER(g_ctx.shadowMappingVS);
    JAM_DESTROY_SHADER(g_ctx.skyboxVS);

    JAM_DESTROY_SHADER(g_ctx.csmLevel2GS);
    JAM_DESTROY_SHADER(g_ctx.csmLevel3GS);
    JAM_DESTROY_SHADER(g_ctx.csmLevel4GS);
    JAM_DESTROY_SHADER(g_ctx.osmGS);

    JAM_DESTROY_SHADER(g_ctx.pbrGBufferPS);
    JAM_DESTROY_SHADER(g_ctx.pbrLightingPS);
    JAM_DESTROY_SHADER(g_ctx.pbrLightVolumePS);
    JAM_DESTROY_SHADER(g_ctx.pbrEnvironmentLightVolumePS);
    JAM_DESTROY_SHADER(g_ctx.csmPS);
    JAM_DESTROY_SHADER(g_ctx.osmPS);
    JAM_DESTROY_SHADER(g_ctx.shadowMappingPS);
    JAM_DESTROY_SHADER(g_ctx.skyboxPS);
    JAM_DESTROY_SHADER(g_ctx.ssaoPS);
    JAM_DESTROY_SHADER(g_ctx.ssaoBlurXPS);
    JAM_DESTROY_SHADER(g_ctx.ssaoBlurYPS);
    JAM_DESTROY_SHADER(g_ctx.bloomXPS);
    JAM_DESTROY_SHADER(g_ctx.bloomYPS);
    JAM_DESTROY_SHADER(g_ctx.bloomCombinePS);
    JAM_DESTROY_SHADER(g_ctx.toneMappingUncharted2PS);
    JAM_DESTROY_SHADER(g_ctx.toneMappingLinearPS);
    JAM_DESTROY_SHADER(g_ctx.toneMappingReinhardPS);
    JAM_DESTROY_SHADER(g_ctx.toneMappingLumaBasedReinhardPS);
    JAM_DESTROY_SHADER(g_ctx.toneMappingWhitePreservingReinhardPS);
    JAM_DESTROY_SHADER(g_ctx.toneMappingRombDaHousePS);
    JAM_DESTROY_SHADER(g_ctx.toneMappingFilmicPS);
    JAM_DESTROY_SHADER(g_ctx.fxaaVeryLowPS);
    JAM_DESTROY_SHADER(g_ctx.fxaaLowPS);
    JAM_DESTROY_SHADER(g_ctx.fxaaMediumPS);
    JAM_DESTROY_SHADER(g_ctx.fxaaHighPS);
    JAM_DESTROY_SHADER(g_ctx.fxaaVeryHighPS);
    JAM_DESTROY_SHADER(g_ctx.fxaaUltraPS);
    JAM_DESTROY_SHADER(g_ctx.fogPS);
    JAM_DESTROY_SHADER(g_ctx.passThroughPS);

    JAM_DESTROY_PROGRAM(g_ctx.pbrGBufferProgram);
    JAM_DESTROY_PROGRAM(g_ctx.pbrLightingProgram);
    JAM_DESTROY_PROGRAM(g_ctx.pbrLightVolumeProgram);
    JAM_DESTROY_PROGRAM(g_ctx.pbrEnvironmentLightVolumeProgram);
    JAM_DESTROY_PROGRAM(g_ctx.csmLevel2CasterProgram);
    JAM_DESTROY_PROGRAM(g_ctx.csmLevel3CasterProgram);
    JAM_DESTROY_PROGRAM(g_ctx.csmLevel4CasterProgram);
    JAM_DESTROY_PROGRAM(g_ctx.osmCasterProgram);
    JAM_DESTROY_PROGRAM(g_ctx.shadowCasterProgram);
    JAM_DESTROY_PROGRAM(g_ctx.skyboxProgram);
    JAM_DESTROY_PROGRAM(g_ctx.ssaoProgram);
    JAM_DESTROY_PROGRAM(g_ctx.ssaoBlurXProgram);
    JAM_DESTROY_PROGRAM(g_ctx.ssaoBlurYProgram);

    JAM_DESTROY_PROGRAM(g_ctx.bloomXProgram);
    JAM_DESTROY_PROGRAM(g_ctx.bloomYProgram);
    JAM_DESTROY_PROGRAM(g_ctx.bloomCombineProgram);
    JAM_DESTROY_PROGRAM(g_ctx.toneMappingUncharted2Program);
    JAM_DESTROY_PROGRAM(g_ctx.toneMappingLinearProgram);
    JAM_DESTROY_PROGRAM(g_ctx.toneMappingReinhardProgram);
    JAM_DESTROY_PROGRAM(g_ctx.toneMappingLumaBasedReinhardProgram);
    JAM_DESTROY_PROGRAM(g_ctx.toneMappingWhitePreservingReinhardProgram);
    JAM_DESTROY_PROGRAM(g_ctx.toneMappingRombDaHouseProgram);
    JAM_DESTROY_PROGRAM(g_ctx.toneMappingFilmicProgram);
    JAM_DESTROY_PROGRAM(g_ctx.fxaaVeryLowProgram);
    JAM_DESTROY_PROGRAM(g_ctx.fxaaLowProgram);
    JAM_DESTROY_PROGRAM(g_ctx.fxaaMediumProgram);
    JAM_DESTROY_PROGRAM(g_ctx.fxaaHighProgram);
    JAM_DESTROY_PROGRAM(g_ctx.fxaaVeryHighProgram);
    JAM_DESTROY_PROGRAM(g_ctx.fxaaUltraProgram);
    JAM_DESTROY_PROGRAM(g_ctx.fogProgram);
    JAM_DESTROY_PROGRAM(g_ctx.passThroughProgram);

    JAM_DESTROY_CONSTANT_BUFFER(g_ctx.cbTransform);
    JAM_DESTROY_CONSTANT_BUFFER(g_ctx.cbCamera);
    JAM_DESTROY_CONSTANT_BUFFER(g_ctx.cbMaterial);
    JAM_DESTROY_CONSTANT_BUFFER(g_ctx.cbLight);
    JAM_DESTROY_CONSTANT_BUFFER(g_ctx.cbEnvironment);
    JAM_DESTROY_CONSTANT_BUFFER(g_ctx.cbShadowCaster);
    JAM_DESTROY_CONSTANT_BUFFER(g_ctx.cbCsmCaster);
    JAM_DESTROY_CONSTANT_BUFFER(g_ctx.cbOsmCaster);
    JAM_DESTROY_CONSTANT_BUFFER(g_ctx.cbSSAO);

    JAM_DESTROY_TEXTURE(g_ctx.ssaoNoiseTexture);

    g_ctx.fullScreenTri.Unload();
    g_ctx.lightVolumeFullScreenTri.Unload();
    g_ctx.lightVolumeUnitSphere.Unload();
    g_ctx.lightVolumeUnitCone.Unload();
    g_ctx.skyboxCube.Unload();
}

void Begin(const Camera& _camera, const Transform& _cameraTrans, const Environment& _env)
{
    Mat4    view           = _cameraTrans.CreateView();
    Mat4    proj           = _camera.CreateProj();
    Frustum viewFrustumInW = Frustum::CreateFromMatrix(view * proj);
    g_ctx.Begin(_cameraTrans.translate, view, proj, viewFrustumInW, _env);
}

void Begin(const Camera& _camera, const Transform& _cameraTrans, const Frustum& _viewFrustumInW, const Environment& _env)
{
    Mat4 view = _cameraTrans.CreateView();
    Mat4 proj = _camera.CreateProj();
    g_ctx.Begin(_cameraTrans.translate, view, proj, _viewFrustumInW, _env);
}

void End()
{
    JAM_ASSERT(g_ctx.bBeginCalled == true, "End() called without calling Begin() first.");
    g_ctx.timer.Stop();
}

const Timer& GetRendererCPUProfileTimer()
{
    JAM_ASSERT(g_ctx.bBeginCalled == false, "GetRendererCPUProfileTimer() called between Begin() and End().");
    return g_ctx.timer;
}

CullingSystem& GetCullingSystem()
{
    return g_ctx.cullingSystem;
}

namespace pass::shadow::osm
{

    void SetupPass()
    {
        gfx::SetProgram(g_ctx.osmCasterProgram);
        gfx::SetConstantBuffer(g_ctx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbOsmCaster, eShader::GeometryShader, CB_OMNIDIRECTIONAL_SHADOW_CASTER_SLOT);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetGfxState(g_ctx.depthTestState);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void SetupLight(const PointLight& _light, const Transform& _lightTrans)
    {
        Mat4 lightProj = CreatePerspectiveFov(ToRad(90.f), 1.f, 0.1f, _light.fallOffEnd);

        CB_OMNIDIRECTIONAL_SHADOW_CASTER cb             = {};
        cb.cb_ODSProjView[EnumToInt(eCubeFace::Right)]  = (CreateViewLookTo(_lightTrans.translate, Vec3::Right) * lightProj).Transpose();
        cb.cb_ODSProjView[EnumToInt(eCubeFace::Left)]   = (CreateViewLookTo(_lightTrans.translate, Vec3::Left) * lightProj).Transpose();
        cb.cb_ODSProjView[EnumToInt(eCubeFace::Top)]    = (CreateViewLookTo(_lightTrans.translate, Vec3::Up) * lightProj).Transpose();
        cb.cb_ODSProjView[EnumToInt(eCubeFace::Bottom)] = (CreateViewLookTo(_lightTrans.translate, Vec3::Down) * lightProj).Transpose();
        cb.cb_ODSProjView[EnumToInt(eCubeFace::Front)]  = (CreateViewLookTo(_lightTrans.translate, Vec3::Forward) * lightProj).Transpose();
        cb.cb_ODSProjView[EnumToInt(eCubeFace::Back)]   = (CreateViewLookTo(_lightTrans.translate, Vec3::Backward) * lightProj).Transpose();
        gfx::UploadBuffer(g_ctx.cbOsmCaster, MemoryView { cb });
    }

    void Submit(const DrawItem& _item)
    {
        g_ctx.UploadTransform(_item.world);
        _item.pMesh->Submit();
    }

}   // namespace pass::shadow::osm

namespace pass::shadow::csm
{

    void SetupPass()
    {
        gfx::SetConstantBuffer(g_ctx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbCsmCaster, eShader::GeometryShader, CB_CASCADE_SHADOW_CASTER_SLOT);

        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetGfxState(g_ctx.depthTestState);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);

        {
            Int32 cascadeLevel = 0;
            if (g_ctx.pCurEnv->shadowCascadeLevel == eCascadeLevel::level2)
            {
                cascadeLevel = 2;
                gfx::SetProgram(g_ctx.csmLevel2CasterProgram);
            }
            else if (g_ctx.pCurEnv->shadowCascadeLevel == eCascadeLevel::level3)
            {
                cascadeLevel = 3;
                gfx::SetProgram(g_ctx.csmLevel3CasterProgram);
            }
            else if (g_ctx.pCurEnv->shadowCascadeLevel == eCascadeLevel::level4)
            {
                cascadeLevel = 4;
                gfx::SetProgram(g_ctx.csmLevel4CasterProgram);
            }

            CB_CASCADE_SHADOW_CASTER cb;
            const float              nearZ    = g_ctx.viewFrustumInW.GetNear();
            const float              farZ     = g_ctx.viewFrustumInW.GetFar();
            const float              invRange = 1.f / (farZ - nearZ);

            Frustum cascadeFrustums[4] = {};
            float   startScale         = 0.f;
            for (Int32 i = 0; i < cascadeLevel; ++i)
            {
                // 각 캐스케이드 프러스텀 생성
                const float scale  = Clamp(g_ctx.pCurEnv->shadowCascadeRanges[i] * invRange, startScale, 1.f);
                cascadeFrustums[i] = Frustum::CreateSubFrustum(g_ctx.viewFrustumInW, startScale, scale);
                startScale         = scale;

                // 프러스텀의 코너, 중심
                FrustumCorners corners = cascadeFrustums[i].GetCorners();
                Vec3           center  = Vec3::Zero;
                for (const Vec3& corner: corners.points)
                    center += corner;
                center /= 8.f;

                // 라이트 뷰 행렬
                Mat4 lightV = CreateViewLookTo(center, g_ctx.pCurEnv->sunDirection);

                // 프러스텀의 8개 꼭짓점을 라이트 뷰 공간으로 변환 후 AABB 생성
                AABB lightAABB = {};
                for (Int32 j = 0; j < 8; ++j)
                {
                    Vec3 p    = Vec3::Transform(corners.points[j], lightV);
                    lightAABB = AABB::Merge(lightAABB, p);
                }

                // z축 범위 약간 확장
                if (lightAABB.min.z < 0.f)
                    lightAABB.min.z *= g_ctx.pCurEnv->shadowCascadeExtendZScale;
                else
                    lightAABB.min.z /= g_ctx.pCurEnv->shadowCascadeExtendZScale;

                if (lightAABB.max.z < 0.f)
                    lightAABB.max.z /= g_ctx.pCurEnv->shadowCascadeExtendZScale;
                else
                    lightAABB.max.z *= g_ctx.pCurEnv->shadowCascadeExtendZScale;

                // 라이트 투영 행렬
                Mat4 lightP          = CreateOrthographic(lightAABB.Width(), lightAABB.Height(), lightAABB.min.z, lightAABB.max.z);
                cb.cb_CSMProjView[i] = (lightV * lightP).Transpose();
            }

            gfx::UploadBuffer(g_ctx.cbCsmCaster, MemoryView { cb });
        }
    }

    void Submit(const DrawItem& _item)
    {
        g_ctx.UploadTransform(_item.world);
        _item.pMesh->Submit();
    }

}   // namespace pass::shadow::csm

namespace pass::shadow::shadowmapping
{
    void SetupPass()
    {
        gfx::SetProgram(g_ctx.shadowCasterProgram);
        gfx::SetConstantBuffer(g_ctx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbShadowCaster, eShader::VertexShader, CB_SHADOW_CASTER_SLOT);

        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetGfxState(g_ctx.depthTestState);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void SetupLight(const SpotLight& _light, const Transform& _lightTrans)
    {
        Mat4 lightView = _lightTrans.CreateView();
        Mat4 lightProj = CreatePerspectiveFov(_light.outerConeHalfRad * 2.f, 1.f, _light.fallOffStart, _light.fallOffEnd);

        CB_SHADOW_CASTER cb;
        cb.cb_shaderMappingProjView = (lightView * lightProj).Transpose();
        gfx::UploadBuffer(g_ctx.cbShadowCaster, MemoryView { cb });
    }

    void Submit(const DrawItem& _item)
    {
        g_ctx.UploadTransform(_item.world);
        _item.pMesh->Submit();
    }

}   // namespace pass::shadow::shadowmapping

namespace pass::lightvolume::pbr
{
    void SetupPass(const GBufferParam& _gBufferParam)
    {
        gfx::SetProgram(g_ctx.pbrLightVolumeProgram);

        gfx::SetSampler(g_ctx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetConstantBuffer(g_ctx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbCamera, eShader::VertexShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbLight, eShader::PixelShader, CB_LIGHT_SLOT);

        gfx::SetBlendState(0, BlendState::k_additive);
        gfx::SetGfxState(g_ctx.lightVolumeState);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);

        g_ctx.SetupGBufferTexture(_gBufferParam);
    }

    void Submit(const PointLight& _light, const Transform& _lightTrans)
    {
        Submit(_light, _lightTrans, k_invalidHandle);
    }

    void Submit(const SpotLight& _light, const Transform& _lightTrans)
    {
        Submit(_light, _lightTrans, k_invalidHandle);
    }

    void Submit(const PointLight& _light, const Transform& _lightTrans, const TextureHandle _shadowMap)
    {
        if (JAM_IS_VALID_HANDLE(_shadowMap))
            gfx::SetTexture(_shadowMap, eShader::PixelShader, t_omniDirectionalShadowSlot);

        Sphere sphere;
        sphere.center = _lightTrans.translate;
        sphere.radius = _light.fallOffEnd;
        if (!g_ctx.viewFrustumInW.Intersects(sphere))
            return;   // 뷰 프러스텀 컬링

        Mat4 world = Mat4::CreateScale(_light.fallOffEnd) * Mat4::CreateTranslation(_lightTrans.translate);
        g_ctx.UploadTransform(world);
        g_ctx.UploadPointLight(_light, _lightTrans);
        g_ctx.lightVolumeUnitSphere.Submit();
    }

    void Submit(const SpotLight& _light, const Transform& _lightTrans, const TextureHandle _shadowMap)
    {
        if (JAM_IS_VALID_HANDLE(_shadowMap))
            gfx::SetTexture(_shadowMap, eShader::PixelShader, t_shadowSlot);

        Vec3  forward    = _lightTrans.Forward();   // 꼭짓점의 향해야하는 방향
        float coneHeight = _light.fallOffEnd;
        float contRadius = tanf(_light.outerConeHalfRad) * coneHeight;
        Vec3  s          = Vec3 { contRadius, coneHeight, contRadius };             // 스케일
        Quat  r          = Quat::FromToRotation(Vec3::Up, forward);                 // 회전
        Vec3  t          = _lightTrans.translate + forward * (coneHeight * 0.5f);   // 위치
        Mat4  world      = Mat4::CreateScale(s) * Mat4::CreateFromQuaternion(r) * Mat4::CreateTranslation(t);

        // aabb를 대신 사용
        // unit cone은 원점을 기준으로 높이 1. 반지름 1.
        AABB aabb    = AABB::FromCenterExtents(Vec3::Zero, Vec3 { 1.f, 0.5f, 1.f });
        AABB aabbInW = AABB::Transform(aabb, world);
        if (g_ctx.viewFrustumInW.Intersects(aabbInW) == ePlaneIntersectType::Outside)
            return;   // 뷰 프러스텀 컬링

        g_ctx.UploadTransform(world);
        g_ctx.UploadSpotLight(_light, _lightTrans);
        g_ctx.lightVolumeUnitCone.Submit();
    }

}   // namespace pass::lightvolume::pbr

namespace pass::environmentlightvolume::pbr
{
    void SetupPass(const GBufferParam& _gBufferParam)
    {
        gfx::SetProgram(g_ctx.pbrEnvironmentLightVolumeProgram);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetConstantBuffer(g_ctx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbEnvironment, eShader::PixelShader, CB_ENVIRONMENT_SLOT);
        gfx::SetBlendState(0, BlendState::k_additive);
        gfx::SetGfxState(g_ctx.lightVolumeState);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);

        g_ctx.SetupGBufferTexture(_gBufferParam);
    }

    void Submit()
    {
        Submit(k_invalidHandle);
    }

    void Submit(const TextureHandle _shadowMap)
    {
        if (JAM_IS_VALID_HANDLE(_shadowMap))
            gfx::SetTexture(_shadowMap, eShader::PixelShader, t_shadowSlot);
        g_ctx.lightVolumeFullScreenTri.Submit();
    }

}   // namespace pass::environmentlightvolume::pbr

namespace pass::gbuffer::pbr
{
    void SetupPass()
    {
        gfx::SetProgram(g_ctx.pbrGBufferProgram);

        gfx::SetSampler(g_ctx.linearClampSampler, eShader::VertexShader, s_linearClampSlot);

        gfx::SetConstantBuffer(g_ctx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbCamera, eShader::VertexShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbMaterial, eShader::VertexShader, CB_MATERIAL_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbMaterial, eShader::PixelShader, CB_MATERIAL_SLOT);

        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetGfxState(GfxState::k_depthTest);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void Submit(const DrawItem& _item)
    {
        g_ctx.UploadTransform(_item.world);
        g_ctx.UploadMaterial(*_item.pMaterial);
        _item.pMesh->Submit();
    }

}   // namespace pass::gbuffer::pbr

namespace pass::lighting::pbr
{
    void SetupPass(const GBufferParam& _gBufferParam)
    {
        gfx::SetProgram(g_ctx.pbrLightingProgram);

        gfx::SetSampler(g_ctx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
        gfx::SetSampler(g_ctx.linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetConstantBuffer(g_ctx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbEnvironment, eShader::PixelShader, CB_ENVIRONMENT_SLOT);

        gfx::SetGfxState(GfxState::k_depthTest);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);

        g_ctx.pCurEnv->irradianceMap->Bind(eShader::PixelShader, t_irradianceSlot);
        g_ctx.pCurEnv->specularMap->Bind(eShader::PixelShader, t_specularSlot);
        g_ctx.pCurEnv->brdfLUT->Bind(eShader::PixelShader, t_brdfSlot);

        g_ctx.SetupGBufferTexture(_gBufferParam);
    }

    void SetupSSAOTexture(const TextureHandle _ssaoTex)
    {
        gfx::SetTexture(_ssaoTex, eShader::PixelShader, t_ssaoSlot);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace pass::lighting::pbr

namespace pass::skybox
{
    void SetupPass()
    {
        gfx::SetProgram(g_ctx.skyboxProgram);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetConstantBuffer(g_ctx.cbCamera, eShader::VertexShader, CB_CAMERA_SLOT);

        gfx::SetGfxState(g_ctx.skyboxState);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);

        g_ctx.pCurEnv->skyboxTexture->Bind(eShader::PixelShader, t_skyboxSlot);
    }

    void Submit()
    {
        g_ctx.skyboxCube.Submit();
    }
}   // namespace pass::skybox

namespace pass::ssao
{
    void BakeKernel()
    {
        g_ctx.BakeSSAOKernel();
    }

    void BakeNoise()
    {
        g_ctx.BakeSSAONoise();
    }

    void SetupPass(const GBufferParam& _param)
    {
        gfx::SetProgram(g_ctx.ssaoProgram);

        gfx::SetConstantBuffer(g_ctx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbSSAO, eShader::PixelShader, CB_SSAO_KERNEL_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbEnvironment, eShader::PixelShader, CB_ENVIRONMENT_SLOT);

        gfx::SetSampler(g_ctx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
        gfx::SetSampler(g_ctx.pointWrapSampler, eShader::PixelShader, s_pointWrapSlot);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);

        gfx::SetTexture(_param.normalRoughnessMetallicTex, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot);
        gfx::SetTexture(_param.depthTex, eShader::PixelShader, t_depthSlot);
        gfx::SetTexture(g_ctx.ssaoNoiseTexture, eShader::PixelShader, t_ssaoNoiseSlot);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace pass::ssao

namespace pass::ssaoblur_x
{
    void SetupPass(const TextureHandle& _ssaoTexture)
    {
        gfx::SetProgram(g_ctx.ssaoBlurXProgram);

        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);

        gfx::SetTexture(_ssaoTexture, eShader::PixelShader, t_ssaoSlot);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace pass::ssaoblur_x

namespace pass::ssaoblur_y
{
    void SetupPass(const TextureHandle& _ssaoTexture)
    {
        gfx::SetProgram(g_ctx.ssaoBlurYProgram);

        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);

        gfx::SetTexture(_ssaoTexture, eShader::PixelShader, t_ssaoSlot);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace pass::ssaoblur_y

namespace postprocess::bloom_x
{
    void SetupPostProcess(const TextureHandle& _inputTexture)
    {
        gfx::SetProgram(g_ctx.bloomXProgram);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace postprocess::bloom_x

namespace postprocess::bloom_y
{
    void SetupPostProcess(const TextureHandle& _inputTexture)
    {
        gfx::SetProgram(g_ctx.bloomYProgram);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }
    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace postprocess::bloom_y

namespace postprocess::bloom_combine
{
    void SetupPostProcess(const TextureHandle& _baseTexture, const TextureHandle& _bloomTexture)
    {
        gfx::SetProgram(g_ctx.bloomCombineProgram);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetTexture(_baseTexture, eShader::PixelShader, t_input0Slot);
        gfx::SetTexture(_bloomTexture, eShader::PixelShader, t_input1Slot);
        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }
    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace postprocess::bloom_combine

namespace tone_mapping
{
    void SetupPostProcess(const Environment& _env, const TextureHandle& _inputTexture)
    {
        switch (_env.toneMappingType)
        {
            case eToneMappingType::Linear:
                gfx::SetProgram(g_ctx.toneMappingLinearProgram);
                break;
            case eToneMappingType::Reinhard:
                gfx::SetProgram(g_ctx.toneMappingReinhardProgram);
                break;
            case eToneMappingType::LumaBasedReinhard:
                gfx::SetProgram(g_ctx.toneMappingLumaBasedReinhardProgram);
                break;
            case eToneMappingType::WhitePreservingReinhard:
                gfx::SetProgram(g_ctx.toneMappingWhitePreservingReinhardProgram);
                break;
            case eToneMappingType::RomBinDaHouse:
                gfx::SetProgram(g_ctx.toneMappingRombDaHouseProgram);
                break;
            case eToneMappingType::Uncharted2:
                gfx::SetProgram(g_ctx.toneMappingUncharted2Program);
                break;
            case eToneMappingType::Filmic:
                gfx::SetProgram(g_ctx.toneMappingFilmicProgram);
                break;
            default:
                JAM_ASSERT(0, "Unknown tone mapping type.");
        }
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace tone_mapping

namespace postprocess::fog
{

    void SetupPostProcess(const TextureHandle& _inputTexture, const TextureHandle& _depthTexture)
    {
        gfx::SetProgram(g_ctx.fogProgram);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetSampler(g_ctx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
        gfx::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
        gfx::SetTexture(_depthTexture, eShader::PixelShader, t_depthSlot);
        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace postprocess::fog

namespace postprocess::passthrough
{
    void SetupPostProcess(const TextureHandle& _inputTexture)
    {
        gfx::SetProgram(g_ctx.passThroughProgram);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace postprocess::passthrough

namespace postprocess::fxaa
{
    void SetupPostProcess(const Environment& _env, const TextureHandle& _inputTexture)
    {
        switch (_env.fxaaQuality)
        {
            case eFxaaQuality::VeryLow:
                gfx::SetProgram(g_ctx.fxaaVeryLowProgram);
                break;
            case eFxaaQuality::Low:
                gfx::SetProgram(g_ctx.fxaaLowProgram);
                break;
            case eFxaaQuality::Medium:
                gfx::SetProgram(g_ctx.fxaaMediumProgram);
                break;
            case eFxaaQuality::High:
                gfx::SetProgram(g_ctx.fxaaHighProgram);
                break;
            case eFxaaQuality::VeryHigh:
                gfx::SetProgram(g_ctx.fxaaVeryHighProgram);
                break;
            case eFxaaQuality::Ultra:
                gfx::SetProgram(g_ctx.fxaaUltraProgram);
                break;
            default:
                JAM_ASSERT(0, "Unknown FXAA quality.");
        }
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace postprocess::fxaa
}   // namespace jam::basic_renderer
