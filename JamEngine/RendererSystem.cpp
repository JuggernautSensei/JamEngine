#include "pch.h"

#include "RendererSystem.h"

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
#include "RenderSystemBase.h"
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
    void UploadLight(const Light& _light, const Transform& _transform) const;
    void UploadCamera(const Vec3& _eyePos, const Mat4& _view, const Mat4& _proj) const;
    void UploadEnvironment(const Environment& _env) const;

    void SetupGBufferTexture(const PBRGbufferParam& _gBufferParam) const;

    void BakeSSAOKernel(eSSAOKernel _kernel) const;

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
};
static RendererContext g_ctx;

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

void RendererContext::UploadLight(const Light& _light, const Transform& _transform) const
{
    constexpr JAM_LIGHT_TYPE k_lightTypeTable[EnumCount<eLightType>()] = {
        JAM_LIGHT_TYPE_POINT,
        JAM_LIGHT_TYPE_SPOT,
    };

    CB_LIGHT cb                 = {};
    cb.cb_lightType             = k_lightTypeTable[EnumToInt(_light.type)];
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
    cb.cb_projViewInv = (_proj * _view).Invert().Transpose();
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

void RendererContext::SetupGBufferTexture(const PBRGbufferParam& _gBufferParam) const
{
    gfx::SetTexture(_gBufferParam.albedoAoTex, eShader::PixelShader, t_gBufferAlbedoAOSlot);
    gfx::SetTexture(_gBufferParam.normalRoughnessMetallicTex, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot);
    gfx::SetTexture(_gBufferParam.additionalLightTex, eShader::PixelShader, t_gBufferAdditionalLightSlot);
    gfx::SetTexture(_gBufferParam.depthTex, eShader::PixelShader, t_depthSlot);
}

void RendererContext::BakeSSAOKernel(const eSSAOKernel _kernel) const
{
    Int32 kernelSize = 0;
    if (_kernel == eSSAOKernel::Kernel4x4)
        kernelSize = 16;
    else if (_kernel == eSSAOKernel::Kernel6x6)
        kernelSize = 36;
    else if (_kernel == eSSAOKernel::Kernel8x8)
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
    gfx::UploadBuffer(g_ctx.cbSSAO, MemoryView { cb });
}

//////////////////////////////////

namespace jam::render_system
{

void Init()
{
    JAM_PROFILE_SCOPE_MS("RendererSystem::Init");

    // pbr gbuffer pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::PBRVS_Bytecode, shader::PBRVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: PBRGBufferShader_VS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::PBRGBufferShader_PS_Bytecode, shader::PBRGBufferShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: PBRGBufferShader_PS");

        g_ctx.pbrGBufferProgram = gfx::CreateProgram(vs, ps, true);
    }

    // pbr lighting pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::PBRLightingShader_PS_Bytecode, shader::PBRLightingShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: PBRLightingShader_PS");

        g_ctx.pbrLightingProgram = gfx::CreateProgram(vs, ps, true);
    }

    // light volume pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::PBRLightVolumeShader_VS_Bytecode, shader::PBRLightVolumeShader_VS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: PBRLightVolumeShader_VS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::PBRLightVolumeShader_PS_Bytecode, shader::PBRLightVolumeShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: PBRLightVolumeShader_PS");

        g_ctx.pbrLightVolumeProgram = gfx::CreateProgram(vs, ps, true);
    }

    // environment light volume pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::PBRLightVolumeShaderForDirectional_VS_Bytecode, shader::PBRLightVolumeShaderForDirectional_VS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: PBREnvironmentLightVolumeShader_VS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::PBRLightVolumeShaderForDirectional_PS_Bytecode, shader::PBRLightVolumeShaderForDirectional_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: PBREnvironmentLightVolumeShader_PS");

        g_ctx.pbrEnvironmentLightVolumeProgram = gfx::CreateProgram(vs, ps, true);
    }

    // csm caster pass
    {
        // level 2
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_VS_Bytecode, shader::CascadeShaderMappingShader_VS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: CascadeShaderMappingShader_VS");

            ShaderHandle gs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_2_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_2_GS_Size });
            gfx::SetShaderName(gs, "jam predefined shader: CascadeShaderMappingShader_CASCADE_LEVEL_2_GS");

            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_PS_Bytecode, shader::CascadeShaderMappingShader_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: CascadeShaderMappingShader_PS");
            g_ctx.csmLevel2CasterProgram = gfx::CreateProgram(vs, ps, gs, k_invalidHandle, k_invalidHandle, true);
        }

        // level 3
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_VS_Bytecode, shader::CascadeShaderMappingShader_VS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: CascadeShaderMappingShader_VS");

            ShaderHandle gs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Size });
            gfx::SetShaderName(gs, "jam predefined shader: CascadeShaderMappingShader_CASCADE_LEVEL_3_GS");

            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_PS_Bytecode, shader::CascadeShaderMappingShader_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: CascadeShaderMappingShader_PS");

            g_ctx.csmLevel3CasterProgram = gfx::CreateProgram(vs, ps, gs, k_invalidHandle, k_invalidHandle, true);
        }

        // level 4
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_VS_Bytecode, shader::CascadeShaderMappingShader_VS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: CascadeShaderMappingShader_VS");

            ShaderHandle gs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_4_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_4_GS_Size });
            gfx::SetShaderName(gs, "jam predefined shader: CascadeShaderMappingShader_CASCADE_LEVEL_4_GS");

            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_PS_Bytecode, shader::CascadeShaderMappingShader_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: CascadeShaderMappingShader_PS");

            g_ctx.csmLevel4CasterProgram = gfx::CreateProgram(vs, ps, gs, k_invalidHandle, k_invalidHandle, true);
        }
    }

    // osm cast pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::OmniDirectionalShaderMappingShader_VS_Bytecode, shader::OmniDirectionalShaderMappingShader_VS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: OmniDirectionalShaderMappingShader_VS");

        ShaderHandle gs = gfx::CreateShader(MemoryView { shader::OmniDirectionalShaderMappingShader_GS_Bytecode, shader::OmniDirectionalShaderMappingShader_GS_Size });
        gfx::SetShaderName(gs, "jam predefined shader: OmniDirectionalShaderMappingShader_GS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::OmniDirectionalShaderMappingShader_PS_Bytecode, shader::OmniDirectionalShaderMappingShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: OmniDirectionalShaderMappingShader_PS");

        g_ctx.osmCasterProgram = gfx::CreateProgram(vs, ps, gs, k_invalidHandle, k_invalidHandle, true);
    }

    // shadow caster pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::ShadowMappingShader_VS_Bytecode, shader::ShadowMappingShader_VS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: ShadowMappingShader_VS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::ShadowMappingShader_PS_Bytecode, shader::ShadowMappingShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: ShadowMappingShader_PS");

        g_ctx.shadowCasterProgram = gfx::CreateProgram(vs, ps, true);
    }

    // skybox pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::SkyboxShader_VS_Bytecode, shader::SkyboxShader_VS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: SkyboxShader_VS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::SkyboxShader_PS_Bytecode, shader::SkyboxShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: SkyboxShader_PS");

        g_ctx.skyboxProgram = gfx::CreateProgram(vs, ps, true);
    }

    // ssao pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::SSAOPS_Bytecode, shader::SSAOPS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: SSAOShader_PS");

        g_ctx.ssaoProgram = gfx::CreateProgram(vs, ps, true);
    }

    // ssao blur x pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");

        ShaderHandle psX = gfx::CreateShader(MemoryView { shader::SSAOBlurXPS_Bytecode, shader::SSAOBlurXPS_Size });
        gfx::SetShaderName(psX, "jam predefined shader: SSAOBlurX_PS");

        g_ctx.ssaoBlurXProgram = gfx::CreateProgram(vs, psX, true);
    }

    // ssao blur y pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");

        ShaderHandle psY = gfx::CreateShader(MemoryView { shader::SSAOBlurYPS_Bytecode, shader::SSAOBlurYPS_Size });
        gfx::SetShaderName(psY, "jam predefined shader: SSAOBlurY_PS");

        g_ctx.ssaoBlurYProgram = gfx::CreateProgram(vs, psY, true);
    }

    // bloom x
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::BloomXPS_Bytecode, shader::BloomXPS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: BloomX_PS");

        g_ctx.bloomXProgram = gfx::CreateProgram(vs, ps, true);
    }

    // bloom y
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::BloomYPS_Bytecode, shader::BloomYPS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: BloomY_PS");

        g_ctx.bloomYProgram = gfx::CreateProgram(vs, ps, true);
    }

    // bloom combine
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::BloomCombinePS_Bytecode, shader::BloomCombinePS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: BloomCombine_PS");

        g_ctx.bloomCombineProgram = gfx::CreateProgram(vs, ps, true);
    }

    // tone mapping passes
    {
        // uncharted2
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::ToneMappingPS_UNCHARTED2_Bytecode, shader::ToneMappingPS_UNCHARTED2_Size });
            gfx::SetShaderName(ps, "jam predefined shader: ToneMappingUncharted2_PS");
            g_ctx.toneMappingUncharted2Program = gfx::CreateProgram(vs, ps, true);
        }

        // linear
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::ToneMappingPS_LINEAR_Bytecode, shader::ToneMappingPS_LINEAR_Size });
            gfx::SetShaderName(ps, "jam predefined shader: ToneMappingLinear_PS");
            g_ctx.toneMappingLinearProgram = gfx::CreateProgram(vs, ps, true);
        }

        // reinhard
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::ToneMappingPS_REINHARD_Bytecode, shader::ToneMappingPS_REINHARD_Size });
            gfx::SetShaderName(ps, "jam predefined shader: ToneMappingReinhard_PS");
            g_ctx.toneMappingReinhardProgram = gfx::CreateProgram(vs, ps, true);
        }

        // luma based reinhard
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::ToneMappingPS_LUMA_BASED_REINHARD_Bytecode, shader::ToneMappingPS_LUMA_BASED_REINHARD_Size });
            gfx::SetShaderName(ps, "jam predefined shader: ToneMappingLumaBasedReinhard_PS");
            g_ctx.toneMappingLumaBasedReinhardProgram = gfx::CreateProgram(vs, ps, true);
        }

        // white preserving reinhard
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::ToneMappingPS_WHITE_PRESERVING_REINHARD_Bytecode, shader::ToneMappingPS_WHITE_PRESERVING_REINHARD_Size });
            gfx::SetShaderName(ps, "jam predefined shader: ToneMappingWhitePreservingReinhard_PS");
            g_ctx.toneMappingWhitePreservingReinhardProgram = gfx::CreateProgram(vs, ps, true);
        }

        // romb da house
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::ToneMappingPS_ROMB_DA_HOUSE_Bytecode, shader::ToneMappingPS_ROMB_DA_HOUSE_Size });
            gfx::SetShaderName(ps, "jam predefined shader: ToneMappingRombDaHouse_PS");
            g_ctx.toneMappingRombDaHouseProgram = gfx::CreateProgram(vs, ps, true);
        }

        // filmic
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::ToneMappingPS_FILMIC_Bytecode, shader::ToneMappingPS_FILMIC_Size });
            gfx::SetShaderName(ps, "jam predefined shader: ToneMappingFilmic_PS");
            g_ctx.toneMappingFilmicProgram = gfx::CreateProgram(vs, ps, true);
        }
    }

    // fxaa
    {
        // very low
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::FXAA_VeryLow_PS_Bytecode, shader::FXAA_VeryLow_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: FXAA_VeryLow_PS");
            g_ctx.fxaaVeryLowProgram = gfx::CreateProgram(vs, ps, true);
        }
        // low
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::FXAA_Low_PS_Bytecode, shader::FXAA_Low_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: FXAA_Low_PS");
            g_ctx.fxaaLowProgram = gfx::CreateProgram(vs, ps, true);
        }
        // medium
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::FXAA_Medium_PS_Bytecode, shader::FXAA_Medium_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: FXAA_Medium_PS");
            g_ctx.fxaaMediumProgram = gfx::CreateProgram(vs, ps, true);
        }

        // high
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::FXAA_High_PS_Bytecode, shader::FXAA_High_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: FXAA_High_PS");
            g_ctx.fxaaHighProgram = gfx::CreateProgram(vs, ps, true);
        }
        // very high
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::FXAA_VeryHigh_PS_Bytecode, shader::FXAA_VeryHigh_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: FXAA_VeryHigh_PS");
            g_ctx.fxaaVeryHighProgram = gfx::CreateProgram(vs, ps, true);
        }

        // ultra
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::FXAA_Ultra_PS_Bytecode, shader::FXAA_Ultra_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: FXAA_Ultra_PS");
            g_ctx.fxaaUltraProgram = gfx::CreateProgram(vs, ps, true);
        }
    }

    // fog pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::FogPS_Bytecode, shader::FogPS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: Fog_PS");
        g_ctx.fogProgram = gfx::CreateProgram(vs, ps, true);
    }

    // pass through pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");
        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::PassThroughPS_Bytecode, shader::PassThroughPS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: PassThrough_PS");
        g_ctx.passThroughProgram = gfx::CreateProgram(vs, ps, true);
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

    // ssao 기본 커널 업로드
    {
        g_ctx.BakeSSAOKernel(eSSAOKernel::Kernel8x8);
        pass::ssao::BakeNoise();
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

void UploadCamera(const Camera& _camera, const Transform& _cameraTrans, const Frustum& _frustumInW)
{
    Mat4 view = _cameraTrans.CreateView();
    Mat4 proj = _camera.CreateProj();
    g_ctx.UploadCamera(_cameraTrans.translate, view, proj);
    g_ctx.viewFrustumInW = _frustumInW;
}

void UploadEnvironment(const Environment& _env)
{
    g_ctx.UploadEnvironment(_env);
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

    void SetLight(const Light& _light, const Transform& _lightTrans)
    {
        JAM_ASSERT(_light.type == eLightType::Point, "Only point light is supported for omni-directional shadow mapping pass.");

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

    CreateOsmFrameBufferResult CreateOsmFrameBuffer(const UInt32 _width, const UInt32 _height)
    {
        CreateOsmFrameBufferResult result;
        result.osmMap      = gfx::CreateTextureCube(_width, _height, eFormat::D32_Float, eTextureCreateFlags_DepthStencil);
        result.frameBuffer = gfx::CreateFrameBuffer(result.osmMap, false);
        return result;
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
    }

    void SetEnvironment(const Environment& _env, const Frustum& _frustumInW)
    {
        Int32 cascadeLevel = 0;
        if (_env.shadowCascadeLevel == eCascadeLevel::level2)
        {
            cascadeLevel = 2;
            gfx::SetProgram(g_ctx.csmLevel2CasterProgram);
        }
        else if (_env.shadowCascadeLevel == eCascadeLevel::level3)
        {
            cascadeLevel = 3;
            gfx::SetProgram(g_ctx.csmLevel3CasterProgram);
        }
        else if (_env.shadowCascadeLevel == eCascadeLevel::level4)
        {
            cascadeLevel = 4;
            gfx::SetProgram(g_ctx.csmLevel4CasterProgram);
        }

        CB_CASCADE_SHADOW_CASTER cb;

        const float nearZ    = _frustumInW.GetNear();
        const float farZ     = _frustumInW.GetFar();
        const float invRange = 1.f / (farZ - nearZ);

        Frustum cascadeFrustums[4] = {};
        float   startScale         = 0.f;
        for (Int32 i = 0; i < cascadeLevel; ++i)
        {
            // 각 캐스케이드 프러스텀 생성
            const float scale  = Clamp(_env.shadowCascadeRanges[i] * invRange, startScale, 1.f);
            cascadeFrustums[i] = Frustum::CreateSubFrustum(_frustumInW, startScale, scale);
            startScale         = scale;

            // 프러스텀의 코너, 중심
            FrustumCorners corners = cascadeFrustums[i].GetCorners();
            Vec3           center  = Vec3::Zero;
            for (const Vec3& corner: corners.points)
                center += corner;
            center /= 8.f;

            // 라이트 뷰 행렬
            Mat4 lightV = CreateViewLookTo(center, _env.sunDirection);

            // 프러스텀의 8개 꼭짓점을 라이트 뷰 공간으로 변환 후 AABB 생성
            AABB lightAABB = {};
            for (Int32 j = 0; j < 8; ++j)
            {
                Vec3 p    = Vec3::Transform(corners.points[j], lightV);
                lightAABB = AABB::Merge(lightAABB, p);
            }

            // z축 범위 약간 확장
            if (lightAABB.min.z < 0.f)
                lightAABB.min.z *= _env.shadowCascadeExtendZScale;
            else
                lightAABB.min.z /= _env.shadowCascadeExtendZScale;

            if (lightAABB.max.z < 0.f)
                lightAABB.max.z /= _env.shadowCascadeExtendZScale;
            else
                lightAABB.max.z *= _env.shadowCascadeExtendZScale;

            // 라이트 투영 행렬
            Mat4 lightP          = CreateOrthographic(lightAABB.Width(), lightAABB.Height(), lightAABB.min.z, lightAABB.max.z);
            cb.cb_CSMProjView[i] = (lightV * lightP).Transpose();
        }

        gfx::UploadBuffer(g_ctx.cbCsmCaster, MemoryView { cb });
    }

    void Submit(const DrawItem& _item)
    {
        g_ctx.UploadTransform(_item.world);
        _item.pMesh->Submit();
    }

    CreateCsmFrameBufferResult CreateCsmFrameBuffer(const UInt32 _width, const UInt32 _height, const eCascadeLevel _level)
    {
        Int32 cascadeLevel = 0;
        if (_level == eCascadeLevel::level2)
            cascadeLevel = 2;
        else if (_level == eCascadeLevel::level3)
            cascadeLevel = 3;
        else if (_level == eCascadeLevel::level4)
            cascadeLevel = 4;

        CreateCsmFrameBufferResult result;
        result.csmMap      = gfx::CreateTexture2D(_width, _height, eFormat::D32_Float, cascadeLevel, eTextureCreateFlags_DepthStencil);
        result.frameBuffer = gfx::CreateFrameBuffer(result.csmMap, false);
        return result;
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

    void SetLight(const Light& _light, const Transform& _lightTrans)
    {
        JAM_ASSERT(_light.type == eLightType::Spot, "Only spot light is supported for shadow mapping pass.");

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

    CreateShadowMapFrameBufferResult CreateShadowMapFrameBuffer(const UInt32 _width, const UInt32 _height)
    {
        CreateShadowMapFrameBufferResult result;
        result.shadowMap   = gfx::CreateTexture2D(_width, _height, eFormat::D32_Float, 1, eTextureCreateFlags_DepthStencil);
        result.frameBuffer = gfx::CreateFrameBuffer(result.shadowMap, false);
        return result;
    }
}   // namespace pass::shadow::shadowmapping

namespace pass::lightvolume::pbr
{
    void SetupPass()
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
    }

    void SetupGBufferTexture(const PBRGbufferParam& _gBufferParam)
    {
        g_ctx.SetupGBufferTexture(_gBufferParam);
    }

    void SubmitPointLight(const Light& _pointLight, const Transform& _lightTrans, const TextureHandle _shadowMap)
    {
        JAM_ASSERT(_pointLight.type == eLightType::Point, "Only point light is supported in SubmitPointLight.");

        if (JAM_IS_VALID_HANDLE(_shadowMap))
        {
            gfx::SetTexture(_shadowMap, eShader::PixelShader, t_omniDirectionalShadowSlot);
        }

        Sphere sphere;
        sphere.center = _lightTrans.translate;
        sphere.radius = _pointLight.fallOffEnd;

        if (!g_ctx.viewFrustumInW.Intersects(sphere))
            return;   // 뷰 프러스텀 컬링

        Mat4 world = Mat4::CreateScale(_pointLight.fallOffEnd) * Mat4::CreateTranslation(_lightTrans.translate);
        g_ctx.UploadTransform(world);
        g_ctx.UploadLight(_pointLight, _lightTrans);
        g_ctx.lightVolumeUnitSphere.Submit();
    }

    void SubmitSpotLight(const Light& _spotLight, const Transform& _lightTrans, const TextureHandle _shadowMap)
    {
        JAM_ASSERT(_spotLight.type == eLightType::Spot, "Only spot light is supported in SubmitSpotLight.");

        if (JAM_IS_VALID_HANDLE(_shadowMap))
        {
            gfx::SetTexture(_shadowMap, eShader::PixelShader, t_shadowSlot);
        }

        Vec3  forward    = _lightTrans.Forward();   // 꼭짓점의 향해야하는 방향
        float coneHeight = _spotLight.fallOffEnd;
        float contRadius = tanf(_spotLight.outerConeHalfRad) * coneHeight;
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
        g_ctx.UploadLight(_spotLight, _lightTrans);
        g_ctx.lightVolumeUnitCone.Submit();
    }

}   // namespace pass::lightvolume::pbr

namespace pass::environmentlightvolume::pbr
{
    void SetupPass()
    {
        gfx::SetProgram(g_ctx.pbrEnvironmentLightVolumeProgram);
        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
        gfx::SetConstantBuffer(g_ctx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_ctx.cbEnvironment, eShader::PixelShader, CB_ENVIRONMENT_SLOT);
        gfx::SetBlendState(0, BlendState::k_additive);
        gfx::SetGfxState(g_ctx.lightVolumeState);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void SetupGBufferTexture(const PBRGbufferParam& _gBufferParam)
    {
        g_ctx.SetupGBufferTexture(_gBufferParam);
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

    CreateGBufferFrameBufferResult CreateGBufferFrameBuffer(const UInt32 _width, const UInt32 _height)
    {
        CreateGBufferFrameBufferResult result;

        result.albedoAoTex                = gfx::CreateTexture2D(_width, _height, eFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget);
        result.normalRoughnessMetallicTex = gfx::CreateTexture2D(_width, _height, eFormat::RGBA16_Float, 1, eTextureCreateFlags_RenderTarget);
        result.additionalLightTex         = gfx::CreateTexture2D(_width, _height, eFormat::RGBA16_Float, 1, eTextureCreateFlags_RenderTarget);
        result.depthTex                   = gfx::CreateTexture2D(_width, _height, eFormat::D24_UNorm_S8_UInt, 1, eTextureCreateFlags_DepthStencil);

        TextureHandle attachments[] = {
            result.albedoAoTex,
            result.normalRoughnessMetallicTex,
            result.additionalLightTex,
            result.depthTex,
        };

        result.frameBuffer = gfx::CreateFrameBuffer(attachments, false);
        return result;
    }

}   // namespace pass::gbuffer::pbr

namespace pass::lighting::pbr
{
    void SetupPass()
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
    }

    void SetupGBufferTexture(const PBRGbufferParam& _gBufferParam)
    {
        g_ctx.SetupGBufferTexture(_gBufferParam);
    }

    void SetupSSAOTexture(const TextureHandle _ssaoTex)
    {
        gfx::SetTexture(_ssaoTex, eShader::PixelShader, t_ssaoSlot);
    }

    void SetupEnvironment(const Environment& _env)
    {
        _env.irradianceMap->Bind(eShader::PixelShader, t_irradianceSlot);
        _env.specularMap->Bind(eShader::PixelShader, t_specularSlot);
        _env.brdfLUT->Bind(eShader::PixelShader, t_brdfSlot);
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
    }

    void SetupEnvironment(const Environment& _env)
    {
        _env.skyboxTexture->Bind(eShader::PixelShader, t_skyboxSlot);
    }

    void Submit()
    {
        g_ctx.skyboxCube.Submit();
    }
}   // namespace pass::skybox

namespace pass::ssao
{
    void BakeKernel(const Environment& _env)
    {
        g_ctx.BakeSSAOKernel(_env.ssaoKernelSize);
    }

    void BakeNoise()
    {
        JAM_DESTROY_TEXTURE(g_ctx.ssaoNoiseTexture);

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
        g_ctx.ssaoNoiseTexture = gfx::CreateTexture2D(noiseSize, noiseSize, eFormat::RGBA16_Float, 1, eTextureCreateFlags_None, mem);
    }

    void SetupPass()
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
    }

    void SetupTexture(const TextureHandle& _normalTexture, const TextureHandle& _depthTexture)
    {
        gfx::SetTexture(_normalTexture, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot);
        gfx::SetTexture(_depthTexture, eShader::PixelShader, t_depthSlot);
        gfx::SetTexture(g_ctx.ssaoNoiseTexture, eShader::PixelShader, t_ssaoNoiseSlot);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }

    CreateSSAOFrameBufferResult CreateSSAOFrameBuffer(const UInt32 _width, const UInt32 _height)
    {
        CreateSSAOFrameBufferResult result;
        result.ssaoTex     = gfx::CreateTexture2D(_width, _height, eFormat::R8_UNorm, 1, eTextureCreateFlags_RenderTarget);
        result.frameBuffer = gfx::CreateFrameBuffer(result.ssaoTex, false);
        return result;
    }

}   // namespace pass::ssao

namespace pass::ssaoblur_x
{
    void SetupPass()
    {
        gfx::SetProgram(g_ctx.ssaoBlurXProgram);

        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void SetupSSAOTexture(const TextureHandle& _ssaoTexture)
    {
        gfx::SetTexture(_ssaoTexture, eShader::PixelShader, t_ssaoSlot);
    }

    void Submit()
    {
        g_ctx.fullScreenTri.Submit();
    }
}   // namespace pass::ssaoblur_x

namespace pass::ssaoblur_y
{
    void SetupPass()
    {
        gfx::SetProgram(g_ctx.ssaoBlurYProgram);

        gfx::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetGfxState(GfxState::k_default);
        gfx::SetBlendState(0, BlendState::k_opaque);
        gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
    }

    void SetupSSAOTexture(const TextureHandle& _ssaoTexture)
    {
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
}   // namespace jam::render_system
