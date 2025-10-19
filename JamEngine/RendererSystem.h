#pragma once
#include "Camera.h"
#include "Components.h"
#include "CullingSystem.h"
#include "Environment.h"
#include "RenderSystemBase.h"

namespace jam
{

struct Environment;
struct Transform;
struct DrawItem;
struct Camera;
struct Light;
class Mesh;
class Material;
class ImageTexture;

}   // namespace jam

namespace jam::render_system
{

void Init();
void Shutdown();

void UploadCamera(const Camera& _camera, const Transform& _cameraTrans, const Frustum& _frustumInW);
void UploadEnvironment(const Environment& _env);

JAM_NODISCARD CullingSystem& GetCullingSystem();

namespace pass::gbuffer::pbr
{
    // frame buffer
    // ㄴ rt0: albedo ao
    // ㄴ rt1: normal roughness metallic
    // ㄴ rt2: additional light
    // ㄴ ds: depth stencil
    void SetupPass();
    void Submit(const DrawItem& _item);

    // 유틸리티
    struct CreateGBufferFrameBufferResult
    {
        FrameBufferHandle frameBuffer                = k_invalidHandle;   // 텍스처의 수명을 관리하지 않음
        TextureHandle     albedoAoTex                = k_invalidHandle;
        TextureHandle     normalRoughnessMetallicTex = k_invalidHandle;
        TextureHandle     additionalLightTex         = k_invalidHandle;
        TextureHandle     depthTex                   = k_invalidHandle;
    };
    CreateGBufferFrameBufferResult CreateGBufferFrameBuffer(UInt32 _width, UInt32 _height);

}   // namespace pass::gbuffer::pbr

namespace pass::lighting::pbr
{
    void SetupPass();
    void SetupGBufferTexture(const PBRGbufferParam& _gBufferParam);
    void SetupSSAOTexture(TextureHandle _ssaoTex);
    void SetupEnvironment(const Environment& _env);
    void Submit();

}   // namespace pass::lighting::pbr

namespace pass::lightvolume::pbr
{
    void SetupPass();
    void SetupGBufferTexture(const PBRGbufferParam& _gBufferParam);
    void SubmitPointLight(const Light& _pointLight, const Transform& _lightTrans, TextureHandle _shadowMap);
    void SubmitSpotLight(const Light& _spotLight, const Transform& _lightTrans, TextureHandle _shadowMap);

}   // namespace pass::lightvolume::pbr

namespace pass::environmentlightvolume::pbr
{
    void SetupPass();
    void SetupGBufferTexture(const PBRGbufferParam& _gBufferParam);
    void Submit(TextureHandle _shadowMap);
}   // namespace pass::environmentlightvolume::pbr

namespace pass::skybox
{
    void SetupPass();
    void SetupEnvironment(const Environment& _env);
    void Submit();
}   // namespace pass::skybox

namespace pass::shadow::shadowmapping
{
    void SetupPass();
    void SetLight(const Light& _light, const Transform& _lightTrans);
    void Submit(const DrawItem& _item);

    // 유틸리티
    struct CreateShadowMapFrameBufferResult
    {
        FrameBufferHandle frameBuffer = k_invalidHandle;   // 텍스처의 수명을 관리하지 않음
        TextureHandle     shadowMap   = k_invalidHandle;
    };
    CreateShadowMapFrameBufferResult CreateShadowMapFrameBuffer(UInt32 _width, UInt32 _height);

}   // namespace pass::shadow::shadowmapping

namespace pass::shadow::csm   // Cascaded Shadow Maps
{
    void SetupPass();
    void SetEnvironment(const Environment& _env, const Frustum& _frustumInW);
    void Submit(const DrawItem& _item);

    // 유틸리티
    struct CreateCsmFrameBufferResult
    {
        FrameBufferHandle frameBuffer = k_invalidHandle;   // 텍스처의 수명을 관리하지 않음
        TextureHandle     csmMap      = k_invalidHandle;
    };
    CreateCsmFrameBufferResult CreateCsmFrameBuffer(UInt32 _width, UInt32 _height, eCascadeLevel _level);

}   // namespace pass::shadow::csm

namespace pass::shadow::osm   // Omni-directional Shadow Maps
{
    void SetupPass();
    void SetLight(const Light& _light, const Transform& _lightTrans);
    void Submit(const DrawItem& _item);

    // 유틸리티
    struct CreateOsmFrameBufferResult
    {
        FrameBufferHandle frameBuffer = k_invalidHandle;   // 텍스처의 수명을 관리하지 않음
        TextureHandle     osmMap      = k_invalidHandle;
    };
    CreateOsmFrameBufferResult CreateOsmFrameBuffer(UInt32 _width, UInt32 _height);
}   // namespace pass::shadow::osm

namespace pass::ssao
{
    // Bake 함수들은 Init 과정에서 호출되기에 굳이 수동으로 호출할 필요는 없으나, SSAO 커널관련 환경설정이 바뀌면 BakeKernel은 호출해줘야함
    void BakeKernel(const Environment& _env);   // 내부적으로 커널 생성. 필요할 때만 호출 (ex. 환경설정이 바뀔 때)
    void BakeNoise();                           // 내부적으로 노이즈 생성. 필요할 때만 호출

    void SetupPass();
    void SetupTexture(const TextureHandle& _normalTexture, const TextureHandle& _depthTexture);
    void Submit();

    // 유틸리티
    struct CreateSSAOFrameBufferResult
    {
        FrameBufferHandle frameBuffer = k_invalidHandle;   // 텍스처의 수명을 관리하지 않음
        TextureHandle     ssaoTex     = k_invalidHandle;
    };
    CreateSSAOFrameBufferResult CreateSSAOFrameBuffer(UInt32 _width, UInt32 _height);
}   // namespace pass::ssao

namespace pass::ssaoblur_x
{
    void SetupPass();
    void SetupSSAOTexture(const TextureHandle& _ssaoTexture);
    void Submit();
}   // namespace pass::ssaoblurx

namespace pass::ssaoblur_y
{
    void SetupPass();
    void SetupSSAOTexture(const TextureHandle& _ssaoTexture);
    void Submit();
}   // namespace pass::ssaoblury

namespace postprocess::bloom_x
{
    void SetupPostProcess(const TextureHandle& _inputTexture);
    void Submit();
}   // namespace postprocess::bloom_x

namespace postprocess::bloom_y
{
    void SetupPostProcess(const TextureHandle& _inputTexture);
    void Submit();
}   // namespace postprocess::bloom_y

namespace postprocess::bloom_combine
{
    void SetupPostProcess(const TextureHandle& _baseTexture, const TextureHandle& _bloomTexture);
    void Submit();
}   // namespace postprocess::bloom_combine

namespace postprocess::tone_mapping
{
    void SetupPostProcess(const TextureHandle& _inputTexture);
    void Submit();

}   // namespace postprocess::tone_mapping

namespace postprocess::fog
{
    void SetupPostProcess(const TextureHandle& _inputTexture, const TextureHandle& _depthTexture);
    void Submit();
}   // namespace postprocess::fog

namespace postprocess::passthrough
{
    void SetupPostProcess(const TextureHandle& _inputTexture);
    void Submit();
}   // namespace postprocess::passthrough

namespace postprocess::fxaa
{
    void SetupPostProcess(const Environment& _env, const TextureHandle& _inputTexture);
    void Submit();
}   // namespace postprocess::fxaa

}   // namespace jam::render_system
