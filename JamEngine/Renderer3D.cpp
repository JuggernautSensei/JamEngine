#include "pch.h"

#include "Renderer3D.h"

#include "Camera.h"
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
#include "RendererVertex.h"
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
    void UploadLight(const Environment& _env) const;
    void UploadCamera(const Camera& _camera, const Transform& _transform) const;

    ProgramHandle pbrGBufferProgram;
    ProgramHandle pbrLightingProgram;
    ProgramHandle pbrLightVolumeProgram;
    ProgramHandle csmLevel2CasterProgram;
    ProgramHandle csmLevel3CasterProgram;
    ProgramHandle csmLevel4CasterProgram;
    ProgramHandle osmCasterProgram;
    ProgramHandle shadowCasterProgram;
    ProgramHandle skyboxProgram;

    ConstantBufferHandle cbTransform;
    ConstantBufferHandle cbCamera;
    ConstantBufferHandle cbMaterial;
    ConstantBufferHandle cbLight;
    ConstantBufferHandle cbGlobal;
    ConstantBufferHandle cbShadowCaster;
    ConstantBufferHandle cbCsmCaster;
    ConstantBufferHandle cbOsmCaster;

    Mesh fullScreenQuad;
    Mesh lightVolumeFullScreenQuad;
    Mesh lightVolumeUnitSphere;
    Mesh lightVolumeUnitCone;
    Mesh skyboxCube;

    SamplerState linearWrapSampler;
    SamplerState linearClampSampler;
    SamplerState pointClampSampler;
    SamplerState shadowLinearClampSampler;

    GfxState depthTestState;
    GfxState lightVolumeState;
    GfxState skyboxState;

    Frustum viewFrustumInW = Frustum {};
};
static RendererContext g_rCtx;

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
    cb.cb_bumpMapScaling    = param.bumpMapScaling;

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

    if (param.bumpMap)
    {
        param.bumpMap->Bind(eShader::VertexShader, t_bumpMapSlot);
        cb.cb_textureBindFlags |= JAM_TEXTURE_BIND_FLAGS_BUMP_MAP;
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
    gfx::UploadBuffer(cbLight, MemoryView { cb });
}

void RendererContext::UploadLight(const Environment& _env) const
{
    CB_LIGHT cb          = {};
    cb.cb_lightType      = JAM_LIGHT_TYPE_DIRECTIONAL;
    cb.cb_lightDir       = _env.sunDirection;
    cb.cb_lightRadiance  = _env.sunColor;
    cb.cb_lightIntensity = _env.sunIntensity;
    gfx::UploadBuffer(cbLight, MemoryView { cb });
}

void RendererContext::UploadCamera(const Camera& _camera, const Transform& _transform) const
{
    CB_CAMERA cb      = {};
    cb.cb_eyePos      = _transform.translate;
    cb.cb_view        = _transform.CreateView().Transpose();
    cb.cb_proj        = _camera.CreateProj().Transpose();
    cb.cb_projViewInv = (cb.cb_proj * cb.cb_view).Invert();
    gfx::UploadBuffer(cbCamera, MemoryView { cb });
}

//////////////////////////////////

namespace jam::r3d
{

void Init()
{
    // pbr gbuffer pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::PBRVS_Bytecode, shader::PBRVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: PBRGBufferShader_VS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::PBRGBufferShader_PS_Bytecode, shader::PBRGBufferShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: PBRGBufferShader_PS");

        g_rCtx.pbrGBufferProgram = gfx::CreateProgram(vs, ps, true);
    }

    // pbr lighting pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::FullScreenVS_Bytecode, shader::FullScreenVS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: FullScreenVS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::PBRLightingShader_PS_Bytecode, shader::PBRLightingShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: PBRLightingShader_PS");

        g_rCtx.pbrLightingProgram = gfx::CreateProgram(vs, ps, true);
    }

    // light volume pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::PBRLightVolumeShader_VS_Bytecode, shader::PBRLightVolumeShader_VS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: PBRLightVolumeShader_VS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::PBRLightVolumeShader_PS_Bytecode, shader::PBRLightVolumeShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: PBRLightVolumeShader_PS");

        g_rCtx.pbrLightVolumeProgram = gfx::CreateProgram(vs, ps, true);
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
            g_rCtx.csmLevel2CasterProgram = gfx::CreateProgram(vs, ps, gs, k_invalidHandle, k_invalidHandle, true);
        }

        // level 3
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_VS_Bytecode, shader::CascadeShaderMappingShader_VS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: CascadeShaderMappingShader_VS");

            ShaderHandle gs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_3_GS_Size });
            gfx::SetShaderName(gs, "jam predefined shader: CascadeShaderMappingShader_CASCADE_LEVEL_3_GS");

            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_PS_Bytecode, shader::CascadeShaderMappingShader_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: CascadeShaderMappingShader_PS");

            g_rCtx.csmLevel3CasterProgram = gfx::CreateProgram(vs, ps, gs, k_invalidHandle, k_invalidHandle, true);
        }

        // level 4
        {
            ShaderHandle vs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_VS_Bytecode, shader::CascadeShaderMappingShader_VS_Size });
            gfx::SetShaderName(vs, "jam predefined shader: CascadeShaderMappingShader_VS");

            ShaderHandle gs = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_CASCADE_LEVEL_4_GS_Bytecode, shader::CascadeShaderMappingShader_CASCADE_LEVEL_4_GS_Size });
            gfx::SetShaderName(gs, "jam predefined shader: CascadeShaderMappingShader_CASCADE_LEVEL_4_GS");

            ShaderHandle ps = gfx::CreateShader(MemoryView { shader::CascadeShaderMappingShader_PS_Bytecode, shader::CascadeShaderMappingShader_PS_Size });
            gfx::SetShaderName(ps, "jam predefined shader: CascadeShaderMappingShader_PS");

            g_rCtx.csmLevel4CasterProgram = gfx::CreateProgram(vs, ps, gs, k_invalidHandle, k_invalidHandle, true);
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

        g_rCtx.osmCasterProgram = gfx::CreateProgram(vs, ps, gs, k_invalidHandle, k_invalidHandle, true);
    }

    // shadow caster pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::ShadowMappingShader_VS_Bytecode, shader::ShadowMappingShader_VS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: ShadowMappingShader_VS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::ShadowMappingShader_PS_Bytecode, shader::ShadowMappingShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: ShadowMappingShader_PS");

        g_rCtx.shadowCasterProgram = gfx::CreateProgram(vs, ps, true);
    }

    // skybox pass
    {
        ShaderHandle vs = gfx::CreateShader(MemoryView { shader::SkyboxShader_VS_Bytecode, shader::SkyboxShader_VS_Size });
        gfx::SetShaderName(vs, "jam predefined shader: SkyboxShader_VS");

        ShaderHandle ps = gfx::CreateShader(MemoryView { shader::SkyboxShader_PS_Bytecode, shader::SkyboxShader_PS_Size });
        gfx::SetShaderName(ps, "jam predefined shader: SkyboxShader_PS");

        g_rCtx.skyboxProgram = gfx::CreateProgram(vs, ps, true);
    }

    // constant buffers
    {
        g_rCtx.cbTransform = gfx::CreateConstantBuffer(sizeof(CB_TRANSFORM));
        gfx::SetConstantBufferName(g_rCtx.cbTransform, "jam engien predefined constant buffer: CB_TRANSFORM");

        g_rCtx.cbCamera = gfx::CreateConstantBuffer(sizeof(CB_CAMERA));
        gfx::SetConstantBufferName(g_rCtx.cbCamera, "jam engien predefined constant buffer: CB_CAMERA");

        g_rCtx.cbMaterial = gfx::CreateConstantBuffer(sizeof(CB_MATERIAL));
        gfx::SetConstantBufferName(g_rCtx.cbMaterial, "jam engien predefined constant buffer: CB_MATERIAL");

        g_rCtx.cbLight = gfx::CreateConstantBuffer(sizeof(CB_LIGHT));
        gfx::SetConstantBufferName(g_rCtx.cbLight, "jam engien predefined constant buffer: CB_LIGHT");

        g_rCtx.cbGlobal = gfx::CreateConstantBuffer(sizeof(CB_GLOBAL));
        gfx::SetConstantBufferName(g_rCtx.cbGlobal, "jam engien predefined constant buffer: CB_GLOBAL");

        g_rCtx.cbShadowCaster = gfx::CreateConstantBuffer(sizeof(CB_SHADOW_CASTER));
        gfx::SetConstantBufferName(g_rCtx.cbShadowCaster, "jam engien predefined constant buffer: CB_SHADOW_CASTER");

        g_rCtx.cbCsmCaster = gfx::CreateConstantBuffer(sizeof(CB_CASCADE_SHADOW_CASTER));
        gfx::SetConstantBufferName(g_rCtx.cbCsmCaster, "jam engien predefined constant buffer: CB_CASCADE_SHADOW_CASTER");

        g_rCtx.cbOsmCaster = gfx::CreateConstantBuffer(sizeof(CB_OMNIDIRECTIONAL_SHADOW_CASTER));
        gfx::SetConstantBufferName(g_rCtx.cbOsmCaster, "jam engien predefined constant buffer: CB_OMNIDIRECTIONAL_SHADOW_CASTER");
    }

    // full screen quad
    {
        VertexPosTex2D vertices[] = {
            { Vec2(-1.f, -1.f), Vec2(0.f, 0.f) },
            {  Vec2(1.f, -1.f), Vec2(1.f, 0.f) },
            { Vec2(-1.f,  1.f), Vec2(0.f, 1.f) },
            {  Vec2(1.f,  1.f), Vec2(1.f, 1.f) },
        };

        UInt16 indices[] = { 0, 1, 2, 2, 1, 3 };
        g_rCtx.fullScreenQuad.Load(MemoryView { vertices }, VertexPosTex2D::s_layout, MemoryView { indices });
    }

    // light volume full screen quad
    {
        VertexPos3D vertices[] = {
            { Vec3(-1.f, -1.f, 0.f) },
            { Vec3(1.f, -1.f, 0.f) },
            { Vec3(-1.f, 1.f, 0.f) },
            { Vec3(1.f, 1.f, 0.f) },
        };

        UInt16 indices[] = { 0, 1, 2, 2, 1, 3 };
        g_rCtx.lightVolumeFullScreenQuad.Load(MemoryView { vertices }, VertexPos3D::s_layout, MemoryView { indices });
    }

    // skybox cube
    {
        Vec3 whd          = { 1.f, 1.f, 1.f };
        g_rCtx.skyboxCube = CreateBoxMesh(whd, VertexPos3D::s_layout);
    }

    // unit sphere
    {
        g_rCtx.lightVolumeUnitSphere = CreateSphereMesh(1.f, 2, VertexPos3D::s_layout);
    }

    // unit corn
    {
        g_rCtx.lightVolumeUnitCone = CreateCone(1.f, 1.f, 20, VertexPos3D::s_layout);
    }

    // sampler
    {
        g_rCtx.linearWrapSampler.filter   = eTextureFilter::MinLinear_MagLinear_MipLinear;
        g_rCtx.linearWrapSampler.addressU = eTextureAddress::Wrap;
        g_rCtx.linearWrapSampler.addressV = eTextureAddress::Wrap;
        g_rCtx.linearWrapSampler.addressW = eTextureAddress::Wrap;

        g_rCtx.linearClampSampler.filter   = eTextureFilter::MinLinear_MagLinear_MipLinear;
        g_rCtx.linearClampSampler.addressU = eTextureAddress::Clamp;
        g_rCtx.linearClampSampler.addressV = eTextureAddress::Clamp;
        g_rCtx.linearClampSampler.addressW = eTextureAddress::Clamp;

        g_rCtx.pointClampSampler.filter   = eTextureFilter::MinPoint_MagPoint_MipPoint;
        g_rCtx.pointClampSampler.addressU = eTextureAddress::Clamp;
        g_rCtx.pointClampSampler.addressV = eTextureAddress::Clamp;
        g_rCtx.pointClampSampler.addressW = eTextureAddress::Clamp;

        g_rCtx.shadowLinearClampSampler.filter      = eTextureFilter::MinLinear_MagLinear_MipLinear;
        g_rCtx.shadowLinearClampSampler.addressU    = eTextureAddress::Clamp;
        g_rCtx.shadowLinearClampSampler.addressV    = eTextureAddress::Clamp;
        g_rCtx.shadowLinearClampSampler.addressW    = eTextureAddress::Clamp;
        g_rCtx.shadowLinearClampSampler.compareFunc = eCompareFunc::LessEqual;

        // 후면 컬링, 깊이 테스트 활성화, 깊이 쓰기 활성화
        g_rCtx.depthTestState = GfxState::k_depthTest;

        // 전면 컬링, 깊이 비활성화
        g_rCtx.lightVolumeState = GfxState { ePrimitive::TriangleList, eCullMode::Front, false, false, false, false, false, false, false, false, eCompareFunc::None, false };

        // 전면 컬링, 깊이 테스트 활성화, 깊이 쓰기 비활성화
        g_rCtx.skyboxState = GfxState { ePrimitive::TriangleList, eCullMode::Front, true, false, false, false, false, false, false, false, eCompareFunc::LessEqual, false };
    }
}

void Shutdown()
{
    JAM_DESTROY_PROGRAM(g_rCtx.pbrGBufferProgram);
    JAM_DESTROY_PROGRAM(g_rCtx.pbrLightingProgram);
    JAM_DESTROY_PROGRAM(g_rCtx.pbrLightVolumeProgram);
    JAM_DESTROY_PROGRAM(g_rCtx.csmLevel2CasterProgram);
    JAM_DESTROY_PROGRAM(g_rCtx.osmCasterProgram);
    JAM_DESTROY_PROGRAM(g_rCtx.shadowCasterProgram);
    JAM_DESTROY_PROGRAM(g_rCtx.skyboxProgram);

    JAM_DESTROY_CONSTANT_BUFFER(g_rCtx.cbTransform);
    JAM_DESTROY_CONSTANT_BUFFER(g_rCtx.cbCamera);
    JAM_DESTROY_CONSTANT_BUFFER(g_rCtx.cbMaterial);
    JAM_DESTROY_CONSTANT_BUFFER(g_rCtx.cbLight);
    JAM_DESTROY_CONSTANT_BUFFER(g_rCtx.cbGlobal);
    JAM_DESTROY_CONSTANT_BUFFER(g_rCtx.cbShadowCaster);
    JAM_DESTROY_CONSTANT_BUFFER(g_rCtx.cbCsmCaster);
    JAM_DESTROY_CONSTANT_BUFFER(g_rCtx.cbOsmCaster);

    g_rCtx.fullScreenQuad.Unload();
    g_rCtx.lightVolumeFullScreenQuad.Unload();
    g_rCtx.lightVolumeUnitSphere.Unload();
    g_rCtx.lightVolumeUnitCone.Unload();
    g_rCtx.skyboxCube.Unload();
}

void SetCamera(const Camera& _camera, const Transform& _cameraTrans)
{
    g_rCtx.UploadCamera(_camera, _cameraTrans);
    g_rCtx.viewFrustumInW = _camera.CreateFrustumInWorld(_cameraTrans);
}

void SetCamera(const Camera& _camera, const Transform& _cameraTrans, const Frustum& _viewFrustumInW)
{
    g_rCtx.UploadCamera(_camera, _cameraTrans);
    g_rCtx.viewFrustumInW = _viewFrustumInW;
}

namespace pass::light_volume
{
    void SetupPass(const bool _bSetDefaultState)
    {
        gfx::SetProgram(g_rCtx.pbrLightVolumeProgram);

        gfx::SetSampler(g_rCtx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
        gfx::SetSampler(g_rCtx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetConstantBuffer(g_rCtx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
        gfx::SetConstantBuffer(g_rCtx.cbCamera, eShader::VertexShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_rCtx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_rCtx.cbLight, eShader::VertexShader, CB_LIGHT_SLOT);
        gfx::SetConstantBuffer(g_rCtx.cbLight, eShader::PixelShader, CB_LIGHT_SLOT);

        if (_bSetDefaultState)
        {
            gfx::SetBlendState(0, BlendState::k_additive);
            gfx::SetGfxState(g_rCtx.lightVolumeState);
            gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
        }
    }

    void Submit(const Light& _light, const Transform& _transform)
    {
        if (_light.type == eLightType::Point)
        {
            Sphere sphere;
            sphere.Center = _transform.translate;
            sphere.Radius = _light.fallOffEnd;

            if (!g_rCtx.viewFrustumInW.Intersects(sphere))
            {
                return;   // 뷰 프러스텀 컬링
            }

            Mat4 world = Mat4::CreateScale(_light.fallOffEnd) * Mat4::CreateTranslation(_transform.translate);
            g_rCtx.UploadTransform(world);
            g_rCtx.UploadLight(_light, _transform);
            g_rCtx.lightVolumeUnitSphere.Submit();
        }
        else   // Spot
        {
            Vec3  forward    = _transform.Forward();   // 꼭짓점의 향해야하는 방향
            float coneHeight = _light.fallOffEnd;
            float contRadius = tanf(_light.outerConeHalfRad) * coneHeight;
            Vec3  s          = Vec3 { contRadius, coneHeight, contRadius };            // 스케일
            Quat  r          = Quat::FromToRotation(Vec3::Up, forward);                // 회전
            Vec3  t          = _transform.translate + forward * (coneHeight * 0.5f);   // 위치

            Mat4 world = Mat4::CreateScale(s) * Mat4::CreateFromQuaternion(r) * Mat4::CreateTranslation(t);

            // aabb를 대신 사용
            AABB aabb;
            aabb.Center  = Vec3 { 0.f, 0.f, 0.f };
            aabb.Extents = Vec3 { 2.f, 1.f, 2.f };

            AABB aabbInW;
            aabb.Transform(aabbInW, world);
            if (!g_rCtx.viewFrustumInW.Intersects(aabbInW))
            {
                
            }


            g_rCtx.UploadTransform(world);
            g_rCtx.UploadLight(_light, _transform);
            g_rCtx.lightVolumeUnitCone.Submit();
        }
    }

    void Submit(const Environment& _env)
    {
    }

}   // namespace pass::light_volume

namespace pass::pbr_gbuffer
{

    void SetupPass(const bool _bSetDefaultState)
    {
        gfx::SetProgram(g_rCtx.pbrGBufferProgram);

        gfx::SetSampler(g_rCtx.linearClampSampler, eShader::VertexShader, s_linearClampSlot);
        gfx::SetSampler(g_rCtx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetConstantBuffer(g_rCtx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
        gfx::SetConstantBuffer(g_rCtx.cbCamera, eShader::VertexShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_rCtx.cbMaterial, eShader::VertexShader, CB_MATERIAL_SLOT);
        gfx::SetConstantBuffer(g_rCtx.cbMaterial, eShader::PixelShader, CB_MATERIAL_SLOT);

        if (_bSetDefaultState)
        {
            gfx::SetBlendState(0, BlendState::k_opaque);
            gfx::SetGfxState(GfxState::k_depthTest);
            gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
        }
    }

    void Submit(const DrawItem& _item)
    {
        g_rCtx.UploadMaterial(*_item.pMaterial);
        g_rCtx.UploadTransform(_item.world);
        _item.pMesh->Submit();
    }

}   // namespace pass::pbr_gbuffer

namespace pass::pbr_lighting
{
    void SetupPass(const bool _bSetDefaultState)
    {
        gfx::SetProgram(g_rCtx.pbrLightingProgram);

        gfx::SetSampler(g_rCtx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
        gfx::SetSampler(g_rCtx.linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
        gfx::SetSampler(g_rCtx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetConstantBuffer(g_rCtx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
        gfx::SetConstantBuffer(g_rCtx.cbGlobal, eShader::PixelShader, CB_GLOBAL_SLOT);

        if (_bSetDefaultState)
        {
            gfx::SetGfxState(GfxState::k_depthTest);
            gfx::SetBlendState(0, BlendState::k_opaque);
            gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
        }
    }

    void SetEnvironment(const Environment& _env)
    {
        _env.irradianceMap->Bind(eShader::PixelShader, t_irradianceSlot);
        _env.specularMap->Bind(eShader::PixelShader, t_specularSlot);
        _env.brdfLUT->Bind(eShader::PixelShader, t_brdfSlot);
    }

    void Submit()
    {
        g_rCtx.fullScreenQuad.Submit();
    }
}   // namespace pass::pbr_lighting

namespace pass::skybox
{
    void SetupPass(const bool _bSetDefaultState)
    {
        gfx::SetProgram(g_rCtx.pbrLightingProgram);

        gfx::SetSampler(g_rCtx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);

        gfx::SetConstantBuffer(g_rCtx.cbCamera, eShader::VertexShader, CB_CAMERA_SLOT);

        if (_bSetDefaultState)
        {
            gfx::SetGfxState(g_rCtx.skyboxState);
            gfx::SetBlendState(0, BlendState::k_opaque);
            gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
        }
    }

    void SetEnvironment(const Environment& _env)
    {
        _env.skyboxTexture->Bind(eShader::PixelShader, t_skyboxSlot);
    }

    void Submit()
    {
        g_rCtx.skyboxCube.Submit();
    }
}   // namespace pass::skybox
}   // namespace jam::r3d

// namespace pass::pbr_light_volume
//{
//    void Start(const bool _bSetDefaultState)
//    {
//        gfx::SetProgram(g_rCtx.pbrLightVolumeProgram);
//
//        gfx::SetSampler(g_rCtx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
//        gfx::SetSampler(g_rCtx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//
//        gfx::SetConstantBuffer(g_rCtx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
//        gfx::SetConstantBuffer(g_rCtx.cbCamera, eShader::VertexShader, CB_CAMERA_SLOT);
//        gfx::SetConstantBuffer(g_rCtx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
//        gfx::SetConstantBuffer(g_rCtx.cbLight, eShader::VertexShader, CB_LIGHT_SLOT);
//        gfx::SetConstantBuffer(g_rCtx.cbLight, eShader::PixelShader, CB_LIGHT_SLOT);
//
//        if (_bSetDefaultState)
//        {
//            constexpr GfxState k_lightVolumeState {
//                ePrimitive::TriangleList,
//                eCullMode::Front,   // 전면 컬링
//                false,
//                false,
//                false,
//                false,
//                false,
//                false,
//                false,
//                false,
//                eCompareFunc::None,   // 깊이 테스트 비활성화 (빛은 가산)
//                false
//            };
//
//            gfx::SetBlendState(0, BlendState::k_additive);
//            gfx::SetGfxState(k_lightVolumeState);
//            gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
//        }
//    }
//
//    void SetGBuffer(
//        const TextureHandle _gBufferAlbedoAO,
//        const TextureHandle _gBufferNormalRoughnessMetallic,
//        const TextureHandle _gBufferAdditionalLight,
//        const TextureHandle _depth)
//    {
//        gfx::SetTexture(_gBufferAlbedoAO, eShader::PixelShader, t_gBufferAlbedoAOSlot);
//        gfx::SetTexture(_gBufferNormalRoughnessMetallic, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot);
//        gfx::SetTexture(_gBufferAdditionalLight, eShader::PixelShader, t_gBufferAdditionalLightSlot);
//        gfx::SetTexture(_depth, eShader::PixelShader, t_depthSlot);
//    }
//
//    void Submit(const Light& _light, const Transform& _transform)
//    {
//        if (_light.type == eLightType::Point)
//        {
//            Mat4 world = Mat4::CreateScale(_light.fallOffEnd) * Mat4::CreateTranslation(_transform.translate);
//
//            g_rCtx.SetTransform(world);
//            g_rCtx.SetLight(_light, _transform);
//            g_rCtx.lightVolumeUnitSphere.Submit();
//        }
//        else if (_light.type == eLightType::Spot)
//        {
//            // 원뿔은 꼭짓점이 +y를 향하고 있고, 꼭짓점 좌표는 (0, 0.5, 0) 임
//            Vec3 forward = _transform.Forward();   // 꼭짓점의 향해야하는 방향
//
//            // 원뿔의 스케일링
//            float coneHeight = _light.fallOffEnd;
//            float contRadius = tanf(_light.outerConeHalfRad) * coneHeight;
//            Vec3  scale      = Vec3 { contRadius, coneHeight, contRadius };
//
//            // 원뿔의 회전
//            Quat rot = Quat::FromToRotation(Vec3::Up, forward);
//
//            // 원뿔의 위치
//            Vec3 pos = _transform.translate + forward * (coneHeight * 0.5f);
//
//            Mat4 world = Mat4::CreateScale(scale) * Mat4::CreateFromQuaternion(rot) * Mat4::CreateTranslation(pos);
//            g_rCtx.SetTransform(world);
//            g_rCtx.SetLight(_light, _transform);
//            g_rCtx.lightVolumeUnitCone.Submit();
//        }
//    }
//
//    void Submit(const Environment& _environment)
//    {
//        g_rCtx.SetLight(_environment);
//        g_rCtx.lightVolumeFullScreenQuad.Submit();
//    }
//}   // namespace pass::pbr_light_volume
//
// namespace pass::pbr_lighting
//{
//    void Start(const bool _bSetDefaultState)
//    {
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//    }
//
//    void SetGBuffer(const TextureHandle _gBufferAlbedoAO, const TextureHandle _gBufferNormalRoughnessMetallic, const TextureHandle _gBufferAdditionalLight, const TextureHandle _depth)
//    {
//        gfx::SetTexture(_gBufferAlbedoAO, eShader::PixelShader, t_gBufferAlbedoAOSlot);
//        gfx::SetTexture(_gBufferNormalRoughnessMetallic, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot);
//        gfx::SetTexture(_gBufferAdditionalLight, eShader::PixelShader, t_gBufferAdditionalLightSlot);
//        gfx::SetTexture(_depth, eShader::PixelShader, t_depthSlot);
//    }
//
//    void Submit()
//    {
//        g_rCtx.fullScreenQuad.Submit();
//    }
//
//}   // namespace pass::pbr_lighting
//
// namespace pass::shadow_caster
//{
//    void Start(const bool _bSetDefaultState)
//    {
//        gfx::SetProgram(g_rCtx.shadowCasterProgram);
//
//        gfx::SetConstantBuffer(g_rCtx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
//        gfx::SetConstantBuffer(g_rCtx.cbShadowCaster, eShader::VertexShader, CB_SHADOW_CASTER_SLOT);
//
//        if (_bSetDefaultState)
//        {
//            gfx::SetGfxState(GfxState::k_depthTest);
//            gfx::SetBlendState(0, BlendState::k_opaque);
//            gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
//        }
//    }
//
//    void SetLight(const Light& _light, const Transform& _transform)
//    {
//        JAM_ASSERT(_light.type == eLightType::Spot, "Only spot light is supported for standard shadow map caster.");
//        JAM_ASSERT(_light.bCastShadow == true, "Light must have shadow casting enabled.");
//
//        float fov       = _light.outerConeHalfRad * 2.f;
//        Mat4  lightProj = CreatePerspectiveFov(fov, 1.f, 0.1f, _light.fallOffEnd);
//
//        CB_SHADOW_CASTER cb;
//        cb.cb_shaderMappingProjView = (lightProj * _transform.CreateView()).Transpose();
//        gfx::UploadBuffer(g_rCtx.cbShadowCaster, MemoryView { cb });
//    }
//
//    void Submit(const Mesh& _mesh, const Transform& _transform)
//    {
//        g_rCtx.SetTransform(_transform);
//        _mesh.Submit();
//    }
//}   // namespace pass::shadow_caster
//
// namespace pass::osm_caster
//{
//    void Start(const bool _bSetDefaultState)
//    {
//        gfx::SetProgram(g_rCtx.osmCasterProgram);
//
//        gfx::SetConstantBuffer(g_rCtx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
//        gfx::SetConstantBuffer(g_rCtx.cbOsmCaster, eShader::GeometryShader, CB_OMNIDIRECTIONAL_SHADOW_CASTER_SLOT);
//
//        if (_bSetDefaultState)
//        {
//            gfx::SetGfxState(g_rCtx.depthTestState);
//            gfx::SetBlendState(0, BlendState::k_opaque);
//            gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
//        }
//    }
//
//    void SetLight(const Light& _light, const Transform& _transform)
//    {
//        JAM_ASSERT(_light.type == eLightType::Point, "Only point light is supported for omni-directional shadow map caster.");
//        JAM_ASSERT(_light.bCastShadow == true, "Light must have shadow casting enabled.");
//
//        Mat4 lightProj = CreatePerspectiveFov(ToRad(90.f), 1.f, 0.1f, _light.fallOffEnd);
//
//        CB_OMNIDIRECTIONAL_SHADOW_CASTER cb             = {};
//        cb.cb_ODSLightPos                               = _transform.translate;
//        cb.cb_ODSLightRange                             = _light.fallOffEnd;
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Right)]  = lightProj * CreateViewLookTo(_transform.translate, Vec3::Right);
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Left)]   = lightProj * CreateViewLookTo(_transform.translate, Vec3::Left);
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Top)]    = lightProj * CreateViewLookTo(_transform.translate, Vec3::Up);
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Bottom)] = lightProj * CreateViewLookTo(_transform.translate, Vec3::Down);
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Front)]  = lightProj * CreateViewLookTo(_transform.translate, Vec3::Forward);
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Back)]   = lightProj * CreateViewLookTo(_transform.translate, Vec3::Backward);
//        gfx::UploadBuffer(g_rCtx.cbOsmCaster, MemoryView { cb });
//    }
//
//    void Submit(const Mesh& _mesh, const Transform& _transform)
//    {
//        g_rCtx.SetTransform(_transform);
//        _mesh.Submit();
//    }
//
//}   // namespace pass::osm_caster
//
// namespace pass::csm_caster
//{
//    void Start(const bool _bSetDefaultState)
//    {
//        gfx::SetConstantBuffer(g_rCtx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
//        gfx::SetConstantBuffer(g_rCtx.cbCsmCaster, eShader::GeometryShader, CB_CASCADE_SHADOW_CASTER_SLOT);
//
//        if (_bSetDefaultState)
//        {
//            gfx::SetGfxState(g_rCtx.depthTestState);
//            gfx::SetBlendState(0, BlendState::k_opaque);
//            gfx::SetStencil(StencilState::k_disable, StencilState::k_disable);
//        }
//    }
//
//    void SetLight(const Camera& _camera, const Frustum& _viewFrustumInWorld, const Environment& _env)
//    {
//        Int32 level;
//        if (_env.shadowCascadeLevel == eCascadeLevel::level2)
//        {
//            gfx::SetProgram(g_rCtx.csmLevel2CasterProgram);
//            level = 2;
//        }
//        else if (_env.shadowCascadeLevel == eCascadeLevel::level3)
//        {
//            gfx::SetProgram(g_rCtx.csmLevel3CasterProgram);
//            level = 3;
//        }
//        else
//        {
//            gfx::SetProgram(g_rCtx.csmLevel4CasterProgram);
//            level = 4;
//        }
//
//        Vec3 cornerInWorld[8];   // in world
//        _viewFrustumInWorld.GetCorners(cornerInWorld);
//
//        // calc center
//        Vec3 nearZCenter, farZCenter;
//        for (Int32 i = 0; i < 4; ++i)
//        {
//            nearZCenter += cornerInWorld[i] * 0.25f;
//            farZCenter += cornerInWorld[i + 4] * 0.25f;
//        }
//
//        // calc lerp factor
//        float cascadeBoundT[k_maxShadowCascadeLevel];
//        float cascadeBoundStart = _camera.nearZ;
//        for (Int32 i = 0; i < level; ++i)
//        {
//            float cascadeEnd  = Max(cascadeBoundStart, _env.shadowCascadeRanges[i]);
//            cascadeBoundT[i]  = (cascadeEnd - _camera.nearZ) / (_camera.farZ - _camera.nearZ);
//            cascadeBoundStart = cascadeEnd;
//        }
//
//        CB_CASCADE_SHADOW_CASTER cb = {};
//        float                    t0 = 0.f;
//        for (Int32 i = 0; i < level; ++i)
//        {
//            float t1 = cascadeBoundT[i];
//            float t2 = (t0 + t1) * 0.5f;   // center
//
//            Vec3 cascadeCenter = Vec3::Lerp(nearZCenter, farZCenter, t2);
//            Vec3 lightPos      = cascadeCenter - _env.sunDirection * _env.zMult;
//            Mat4 lightView     = CreateViewLookTo(lightPos, _env.sunDirection);
//
//            float maxX, maxY, maxZ;
//            maxX = maxY = maxZ = -FloatLimits::lowest();
//            float minX, minY, minZ;
//            minX = minY = minZ = FloatLimits::max();
//            for (Int32 j = 0; j < 8; ++j)
//            {
//                Vec3 cornerInLight = Vec3::Transform(cornerInWorld[j], lightView);
//                maxX               = Max(maxX, cornerInLight.x);
//                maxY               = Max(maxY, cornerInLight.y);
//                maxZ               = Max(maxZ, cornerInLight.z);
//                minX               = Min(minX, cornerInLight.x);
//                minY               = Min(minY, cornerInLight.y);
//                minZ               = Min(minZ, cornerInLight.z);
//            }
//
//            // 프러스텀 외부의 물체도 포함시키기 위함
//            minZ = (minZ < 0) ? minZ * _env.zMult
//                              : minZ / _env.zMult;
//
//            maxZ = (maxZ < 0) ? maxZ / _env.zMult
//                              : maxZ * _env.zMult;
//
//            Mat4 ortho = CreateOrthographic(minX, maxX, minY, maxY, minZ, maxZ);
//
//            t0 = t1;
//        }
//    }
//
//    void Submit(const Mesh& _mesh, const Transform& _transform)
//    {
//    }
//}   // namespace pass::csm_caster
