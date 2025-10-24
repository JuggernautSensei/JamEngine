#include "pch.h"

#include "tmp2.h"

#include "tmp1.h"
#include "Camera.h"
#include "CullingSystem.h"
#include "DrawItem.h"
#include "EmbeddedShaderGlue.h"
#include "Environment.h"
#include "Graphics.h"
#include "GraphicsBase.h"
#include "Light.h"
#include "Material.h"
#include "Memory.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include "Profile.h"
#include "Shaders/PrecompiledShaders/PrecompiledJamShader.h"
#include "Transform.h"

//namespace pass::shadow::osm
//{
//
//    void SetupPass()
//    {
//        Graphics::SetProgram(g_ctx.osmCasterProgram);
//        Graphics::SetConstantBuffer(g_ctx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbOsmCaster, eShader::GeometryShader, CB_OMNIDIRECTIONAL_SHADOW_CASTER_SLOT);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetGfxState(g_ctx.depthTestState);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//    }
//
//    void SetupLight(const PointLight& _light, const Transform& _lightTrans)
//    {
//        Mat4 lightProj = CreatePerspectiveFov(ToRad(90.f), 1.f, 0.1f, _light.fallOffEnd);
//
//        CB_OMNIDIRECTIONAL_SHADOW_CASTER cb             = {};
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Right)]  = (CreateViewLookTo(_lightTrans.translate, Vec3::Right) * lightProj).Transpose();
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Left)]   = (CreateViewLookTo(_lightTrans.translate, Vec3::Left) * lightProj).Transpose();
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Top)]    = (CreateViewLookTo(_lightTrans.translate, Vec3::Up) * lightProj).Transpose();
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Bottom)] = (CreateViewLookTo(_lightTrans.translate, Vec3::Down) * lightProj).Transpose();
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Front)]  = (CreateViewLookTo(_lightTrans.translate, Vec3::Forward) * lightProj).Transpose();
//        cb.cb_ODSProjView[EnumToInt(eCubeFace::Back)]   = (CreateViewLookTo(_lightTrans.translate, Vec3::Backward) * lightProj).Transpose();
//        Graphics::UploadBuffer(g_ctx.cbOsmCaster, MemoryView { cb });
//    }
//
//    void Submit(const DrawItem& _item)
//    {
//        g_ctx.UploadTransform(_item.world);
//        _item.pMesh->Submit();
//    }
//
//}   // namespace pass::shadow::osm
//
//namespace pass::shadow::csm
//{
//
//    void SetupPass()
//    {
//        Graphics::SetConstantBuffer(g_ctx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbCsmCaster, eShader::GeometryShader, CB_CASCADE_SHADOW_CASTER_SLOT);
//
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetGfxState(g_ctx.depthTestState);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//
//        {
//            Int32 cascadeLevel = 0;
//            if (g_ctx.pCurEnv->shadowCascadeLevel == eCascadeLevel::level2)
//            {
//                cascadeLevel = 2;
//                Graphics::SetProgram(g_ctx.csmLevel2CasterProgram);
//            }
//            else if (g_ctx.pCurEnv->shadowCascadeLevel == eCascadeLevel::level3)
//            {
//                cascadeLevel = 3;
//                Graphics::SetProgram(g_ctx.csmLevel3CasterProgram);
//            }
//            else if (g_ctx.pCurEnv->shadowCascadeLevel == eCascadeLevel::level4)
//            {
//                cascadeLevel = 4;
//                Graphics::SetProgram(g_ctx.csmLevel4CasterProgram);
//            }
//
//            CB_CASCADE_SHADOW_CASTER cb;
//            const float              nearZ    = g_ctx.viewFrustumInW.GetNear();
//            const float              farZ     = g_ctx.viewFrustumInW.GetFar();
//            const float              invRange = 1.f / (farZ - nearZ);
//
//            Frustum cascadeFrustums[4] = {};
//            float   startScale         = 0.f;
//            for (Int32 i = 0; i < cascadeLevel; ++i)
//            {
//                // 각 캐스케이드 프러스텀 생성
//                const float scale  = Clamp(g_ctx.pCurEnv->shadowCascadeRanges[i] * invRange, startScale, 1.f);
//                cascadeFrustums[i] = Frustum::CreateSubFrustum(g_ctx.viewFrustumInW, startScale, scale);
//                startScale         = scale;
//
//                // 프러스텀의 코너, 중심
//                FrustumCorners corners = cascadeFrustums[i].GetCorners();
//                Vec3           center  = Vec3::Zero;
//                for (const Vec3& corner: corners.points)
//                    center += corner;
//                center /= 8.f;
//
//                // 라이트 뷰 행렬
//                Mat4 lightV = CreateViewLookTo(center, g_ctx.pCurEnv->sunDirection);
//
//                // 프러스텀의 8개 꼭짓점을 라이트 뷰 공간으로 변환 후 AABB 생성
//                AABB lightAABB = {};
//                for (Int32 j = 0; j < 8; ++j)
//                {
//                    Vec3 p    = Vec3::Transform(corners.points[j], lightV);
//                    lightAABB = AABB::Merge(lightAABB, p);
//                }
//
//                // z축 범위 약간 확장
//                if (lightAABB.min.z < 0.f)
//                    lightAABB.min.z *= g_ctx.pCurEnv->shadowCascadeExtendZScale;
//                else
//                    lightAABB.min.z /= g_ctx.pCurEnv->shadowCascadeExtendZScale;
//
//                if (lightAABB.max.z < 0.f)
//                    lightAABB.max.z /= g_ctx.pCurEnv->shadowCascadeExtendZScale;
//                else
//                    lightAABB.max.z *= g_ctx.pCurEnv->shadowCascadeExtendZScale;
//
//                // 라이트 투영 행렬
//                Mat4 lightP          = CreateOrthographic(lightAABB.Width(), lightAABB.Height(), lightAABB.min.z, lightAABB.max.z);
//                cb.cb_CSMProjView[i] = (lightV * lightP).Transpose();
//            }
//
//            Graphics::UploadBuffer(g_ctx.cbCsmCaster, MemoryView { cb });
//        }
//    }
//
//    void Submit(const DrawItem& _item)
//    {
//        g_ctx.UploadTransform(_item.world);
//        _item.pMesh->Submit();
//    }
//
//}   // namespace pass::shadow::csm
//
//namespace pass::shadow::shadowmapping
//{
//    void SetupPass()
//    {
//        Graphics::SetProgram(g_ctx.shadowCasterProgram);
//        Graphics::SetConstantBuffer(g_ctx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbShadowCaster, eShader::VertexShader, CB_SHADOW_CASTER_SLOT);
//
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetGfxState(g_ctx.depthTestState);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//    }
//
//    void SetupLight(const SpotLight& _light, const Transform& _lightTrans)
//    {
//        Mat4 lightView = _lightTrans.CreateView();
//        Mat4 lightProj = CreatePerspectiveFov(_light.outerConeHalfRad * 2.f, 1.f, _light.fallOffStart, _light.fallOffEnd);
//
//        CB_SHADOW_CASTER cb;
//        cb.cb_shaderMappingProjView = (lightView * lightProj).Transpose();
//        Graphics::UploadBuffer(g_ctx.cbShadowCaster, MemoryView { cb });
//    }
//
//    void Submit(const DrawItem& _item)
//    {
//        g_ctx.UploadTransform(_item.world);
//        _item.pMesh->Submit();
//    }
//
//}   // namespace pass::shadow::shadowmapping
//
//namespace pass::lightvolume::pbr
//{
//    void SetupPass(const GBufferParam& _gBufferParam)
//    {
//        Graphics::SetProgram(g_ctx.pbrLightVolumeProgram);
//
//        Graphics::SetSampler(g_ctx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//
//        Graphics::SetConstantBuffer(g_ctx.cbTransform, eShader::VertexShader, CB_TRANSFORM_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbCamera, eShader::VertexShader, CB_CAMERA_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbLight, eShader::PixelShader, CB_LIGHT_SLOT);
//
//        Graphics::SetBlendState(0, BlendState::k_additive);
//        Graphics::SetGfxState(g_ctx.lightVolumeState);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//
//        g_ctx.SetupGBufferTexture(_gBufferParam);
//    }
//
//    void Submit(const PointLight& _light, const Transform& _lightTrans)
//    {
//        Submit(_light, _lightTrans, k_invalidHandle);
//    }
//
//    void Submit(const SpotLight& _light, const Transform& _lightTrans)
//    {
//        Submit(_light, _lightTrans, k_invalidHandle);
//    }
//
//    void Submit(const PointLight& _light, const Transform& _lightTrans, const TextureHandle _shadowMap)
//    {
//        if (JAM_IS_VALID_HANDLE(_shadowMap))
//            Graphics::SetTexture(_shadowMap, eShader::PixelShader, t_omniDirectionalShadowSlot);
//
//        Sphere sphere;
//        sphere.center = _lightTrans.translate;
//        sphere.radius = _light.fallOffEnd;
//        if (!g_ctx.viewFrustumInW.Intersects(sphere))
//            return;   // 뷰 프러스텀 컬링
//
//        Mat4 world = Mat4::CreateScale(_light.fallOffEnd) * Mat4::CreateTranslation(_lightTrans.translate);
//        g_ctx.UploadTransform(world);
//        g_ctx.UploadPointLight(_light, _lightTrans);
//        g_ctx.lightVolumeUnitSphere.Submit();
//    }
//
//    void Submit(const SpotLight& _light, const Transform& _lightTrans, const TextureHandle _shadowMap)
//    {
//        if (JAM_IS_VALID_HANDLE(_shadowMap))
//            Graphics::SetTexture(_shadowMap, eShader::PixelShader, t_shadowSlot);
//
//        Vec3  forward    = _lightTrans.Forward();   // 꼭짓점의 향해야하는 방향
//        float coneHeight = _light.fallOffEnd;
//        float contRadius = tanf(_light.outerConeHalfRad) * coneHeight;
//        Vec3  s          = Vec3 { contRadius, coneHeight, contRadius };             // 스케일
//        Quat  r          = Quat::FromToRotation(Vec3::Up, forward);                 // 회전
//        Vec3  t          = _lightTrans.translate + forward * (coneHeight * 0.5f);   // 위치
//        Mat4  world      = Mat4::CreateScale(s) * Mat4::CreateFromQuaternion(r) * Mat4::CreateTranslation(t);
//
//        // aabb를 대신 사용
//        // unit cone은 원점을 기준으로 높이 1. 반지름 1.
//        AABB aabb    = AABB::FromCenterExtents(Vec3::Zero, Vec3 { 1.f, 0.5f, 1.f });
//        AABB aabbInW = AABB::Transform(aabb, world);
//        if (g_ctx.viewFrustumInW.Intersects(aabbInW) == ePlaneIntersectType::Outside)
//            return;   // 뷰 프러스텀 컬링
//
//        g_ctx.UploadTransform(world);
//        g_ctx.UploadSpotLight(_light, _lightTrans);
//        g_ctx.lightVolumeUnitCone.Submit();
//    }
//
//}   // namespace pass::lightvolume::pbr
//
//namespace pass::environmentlightvolume::pbr
//{
//    void SetupPass(const GBufferParam& _gBufferParam)
//    {
//        Graphics::SetProgram(g_ctx.pbrEnvironmentLightVolumeProgram);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        Graphics::SetConstantBuffer(g_ctx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbEnvironment, eShader::PixelShader, CB_ENVIRONMENT_SLOT);
//        Graphics::SetBlendState(0, BlendState::k_additive);
//        Graphics::SetGfxState(g_ctx.lightVolumeState);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//
//        g_ctx.SetupGBufferTexture(_gBufferParam);
//    }
//
//    void Submit()
//    {
//        Submit(k_invalidHandle);
//    }
//
//    void Submit(const TextureHandle _shadowMap)
//    {
//        if (JAM_IS_VALID_HANDLE(_shadowMap))
//            Graphics::SetTexture(_shadowMap, eShader::PixelShader, t_shadowSlot);
//        g_ctx.lightVolumeFullScreenTri.Submit();
//    }
//
//}   // namespace pass::environmentlightvolume::pbr

//namespace pass::lighting::pbr
//{
//    void SetupPass(const GBufferParam& _gBufferParam)
//    {
//        Graphics::SetProgram(g_ctx.pbrLightingProgram);
//
//        Graphics::SetSampler(g_ctx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
//        Graphics::SetSampler(g_ctx.linearWrapSampler, eShader::PixelShader, s_linearWrapSlot);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//
//        Graphics::SetConstantBuffer(g_ctx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbEnvironment, eShader::PixelShader, CB_ENVIRONMENT_SLOT);
//
//        Graphics::SetGfxState(GfxState::k_depthTest);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//
//        g_ctx.pCurEnv->irradianceMap->Bind(eShader::PixelShader, t_irradianceSlot);
//        g_ctx.pCurEnv->specularMap->Bind(eShader::PixelShader, t_specularSlot);
//        g_ctx.pCurEnv->brdfLUT->Bind(eShader::PixelShader, t_brdfSlot);
//
//        g_ctx.SetupGBufferTexture(_gBufferParam);
//    }
//
//    void SetupSSAOTexture(const TextureHandle _ssaoTex)
//    {
//        Graphics::SetTexture(_ssaoTex, eShader::PixelShader, t_ssaoSlot);
//    }
//
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace pass::lighting::pbr
//
//namespace pass::skybox
//{
//    void SetupPass()
//    {
//        Graphics::SetProgram(g_ctx.skyboxProgram);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        Graphics::SetConstantBuffer(g_ctx.cbCamera, eShader::VertexShader, CB_CAMERA_SLOT);
//
//        Graphics::SetGfxState(g_ctx.skyboxState);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//
//        g_ctx.pCurEnv->skyboxTexture->Bind(eShader::PixelShader, t_skyboxSlot);
//    }
//
//    void Submit()
//    {
//        g_ctx.skyboxCube.Submit();
//    }
//}   // namespace pass::skybox
//
//namespace pass::ssao
//{
//    void BakeKernel()
//    {
//        g_ctx.BakeSSAOKernel();
//    }
//
//    void BakeNoise()
//    {
//        g_ctx.BakeSSAONoise();
//    }
//
//    void SetupPass(const GBufferParam& _param)
//    {
//        Graphics::SetProgram(g_ctx.ssaoProgram);
//
//        Graphics::SetConstantBuffer(g_ctx.cbCamera, eShader::PixelShader, CB_CAMERA_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbSSAO, eShader::PixelShader, CB_SSAO_KERNEL_SLOT);
//        Graphics::SetConstantBuffer(g_ctx.cbEnvironment, eShader::PixelShader, CB_ENVIRONMENT_SLOT);
//
//        Graphics::SetSampler(g_ctx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
//        Graphics::SetSampler(g_ctx.pointWrapSampler, eShader::PixelShader, s_pointWrapSlot);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//
//        Graphics::SetTexture(_param.normalRoughnessMetallicTex, eShader::PixelShader, t_gBufferNormalRoughnessMetallicSlot);
//        Graphics::SetTexture(_param.depthTex, eShader::PixelShader, t_depthSlot);
//        Graphics::SetTexture(g_ctx.ssaoNoiseTexture, eShader::PixelShader, t_ssaoNoiseSlot);
//    }
//
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace pass::ssao
//
//namespace pass::ssaoblur_x
//{
//    void SetupPass(const TextureHandle& _ssaoTexture)
//    {
//        Graphics::SetProgram(g_ctx.ssaoBlurXProgram);
//
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//
//        Graphics::SetTexture(_ssaoTexture, eShader::PixelShader, t_ssaoSlot);
//    }
//
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace pass::ssaoblur_x
//
//namespace pass::ssaoblur_y
//{
//    void SetupPass(const TextureHandle& _ssaoTexture)
//    {
//        Graphics::SetProgram(g_ctx.ssaoBlurYProgram);
//
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//
//        Graphics::SetTexture(_ssaoTexture, eShader::PixelShader, t_ssaoSlot);
//    }
//
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace pass::ssaoblur_y
//
//namespace postprocess::bloom_x
//{
//    void SetupPostProcess(const TextureHandle& _inputTexture)
//    {
//        Graphics::SetProgram(g_ctx.bloomXProgram);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        Graphics::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//    }
//
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace postprocess::bloom_x
//
//namespace postprocess::bloom_y
//{
//    void SetupPostProcess(const TextureHandle& _inputTexture)
//    {
//        Graphics::SetProgram(g_ctx.bloomYProgram);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        Graphics::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//    }
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace postprocess::bloom_y
//
//namespace postprocess::bloom_combine
//{
//    void SetupPostProcess(const TextureHandle& _baseTexture, const TextureHandle& _bloomTexture)
//    {
//        Graphics::SetProgram(g_ctx.bloomCombineProgram);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        Graphics::SetTexture(_baseTexture, eShader::PixelShader, t_input0Slot);
//        Graphics::SetTexture(_bloomTexture, eShader::PixelShader, t_input1Slot);
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//    }
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace postprocess::bloom_combine
//
//namespace tone_mapping
//{
//    void SetupPostProcess(const Environment& _env, const TextureHandle& _inputTexture)
//    {
//        switch (_env.toneMappingType)
//        {
//            case eToneMappingType::Linear:
//                Graphics::SetProgram(g_ctx.toneMappingLinearProgram);
//                break;
//            case eToneMappingType::Reinhard:
//                Graphics::SetProgram(g_ctx.toneMappingReinhardProgram);
//                break;
//            case eToneMappingType::LumaBasedReinhard:
//                Graphics::SetProgram(g_ctx.toneMappingLumaBasedReinhardProgram);
//                break;
//            case eToneMappingType::WhitePreservingReinhard:
//                Graphics::SetProgram(g_ctx.toneMappingWhitePreservingReinhardProgram);
//                break;
//            case eToneMappingType::RomBinDaHouse:
//                Graphics::SetProgram(g_ctx.toneMappingRombDaHouseProgram);
//                break;
//            case eToneMappingType::Uncharted2:
//                Graphics::SetProgram(g_ctx.toneMappingUncharted2Program);
//                break;
//            case eToneMappingType::Filmic:
//                Graphics::SetProgram(g_ctx.toneMappingFilmicProgram);
//                break;
//            default:
//                JAM_ASSERT(0, "Unknown tone mapping type.");
//        }
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        Graphics::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//    }
//
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace tone_mapping
//
//namespace postprocess::fog
//{
//
//    void SetupPostProcess(const TextureHandle& _inputTexture, const TextureHandle& _depthTexture)
//    {
//        Graphics::SetProgram(g_ctx.fogProgram);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        Graphics::SetSampler(g_ctx.pointClampSampler, eShader::PixelShader, s_pointClampSlot);
//        Graphics::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
//        Graphics::SetTexture(_depthTexture, eShader::PixelShader, t_depthSlot);
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//    }
//
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace postprocess::fog
//
//namespace postprocess::passthrough
//{
//    void SetupPostProcess(const TextureHandle& _inputTexture)
//    {
//        Graphics::SetProgram(g_ctx.passThroughProgram);
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        Graphics::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//    }
//
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace postprocess::passthrough
//
//namespace postprocess::fxaa
//{
//    void SetupPostProcess(const Environment& _env, const TextureHandle& _inputTexture)
//    {
//        switch (_env.fxaaQuality)
//        {
//            case eFxaaQuality::VeryLow:
//                Graphics::SetProgram(g_ctx.fxaaVeryLowProgram);
//                break;
//            case eFxaaQuality::Low:
//                Graphics::SetProgram(g_ctx.fxaaLowProgram);
//                break;
//            case eFxaaQuality::Medium:
//                Graphics::SetProgram(g_ctx.fxaaMediumProgram);
//                break;
//            case eFxaaQuality::High:
//                Graphics::SetProgram(g_ctx.fxaaHighProgram);
//                break;
//            case eFxaaQuality::VeryHigh:
//                Graphics::SetProgram(g_ctx.fxaaVeryHighProgram);
//                break;
//            case eFxaaQuality::Ultra:
//                Graphics::SetProgram(g_ctx.fxaaUltraProgram);
//                break;
//            default:
//                JAM_ASSERT(0, "Unknown FXAA quality.");
//        }
//        Graphics::SetSampler(g_ctx.linearClampSampler, eShader::PixelShader, s_linearClampSlot);
//        Graphics::SetTexture(_inputTexture, eShader::PixelShader, t_input0Slot);
//        Graphics::SetGfxState(GfxState::k_default);
//        Graphics::SetBlendState(0, BlendState::k_opaque);
//        Graphics::SetStencil(StencilState::k_disable, StencilState::k_disable);
//    }
//
//    void Submit()
//    {
//        g_ctx.fullScreenTri.Submit();
//    }
//}   // namespace postprocess::fxaa
//}   // namespace jam::basic_renderer
