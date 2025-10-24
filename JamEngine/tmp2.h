#pragma once
#include "tmp1.h"
#include "Camera.h"
#include "Components.h"
#include "CullingSystem.h"
#include "Environment.h"

namespace jam
{

struct SpotLight;
struct PointLight;
struct Environment;
struct Transform;
struct DrawItem;
struct Camera;
class Timer;
class Mesh;
class Material;
class ImageTexture;

}   // namespace jam

namespace jam
{

class SimpleRenderer
{
public:
    static void Init();
    static void Shutdown();

    static void Begin(const Camera& _camera, const Transform& _cameraTrans, const Environment& _env);
    static void Begin(const Camera& _camera, const Transform& _cameraTrans, const Frustum& _viewFrustumInW, const Environment& _env);   // 계산된 Frustum을 직접 넘겨주는 마이크로 최적화 버전 (기존 함수는 내부적으로 Frustum 계산)
    static void End();

    JAM_NODISCARD static const Timer&   GetRendererCPUProfileTimer();
    JAM_NODISCARD static CullingSystem& GetCullingSystem();

    // simple factory


};

}

////
////    //namespace pass::gbuffer::pbr
////    //{
////    //    // frame buffer
////    //    // ㄴ rt0: albedo ao
////    //    // ㄴ rt1: normal roughness metallic
////    //    // ㄴ rt2: additional light
////    //    // ㄴ ds: depth
////    //    void SetupPass();
////    //    void Submit(const DrawItem& _item);
////    //
////    //}   // namespace pass::gbuffer::pbr
////
////    namespace pass::lighting::pbr
////    {
////        void SetupPass(const GBufferParam& _gBufferParam);
////        void SetupSSAOTexture(TextureHandle _ssaoTex);
////        void Submit();
////
////    }   // namespace pass::lighting::pbr
////
////    namespace pass::lightvolume::pbr
////    {
////        void SetupPass(const GBufferParam& _gBufferParam);
////        void Submit(const PointLight& _light, const Transform& _lightTrans);
////        void Submit(const SpotLight& _light, const Transform& _lightTrans);
////        void Submit(const PointLight& _light, const Transform& _lightTrans, TextureHandle _shadowMap);
////        void Submit(const SpotLight& _light, const Transform& _lightTrans, TextureHandle _shadowMap);
////
////    }   // namespace pass::lightvolume::pbr
////
////    namespace pass::environmentlightvolume::pbr
////    {
////        void SetupPass(const GBufferParam& _gBufferParam);
////        void Submit();
////        void Submit(TextureHandle _shadowMap);
////    }   // namespace pass::environmentlightvolume::pbr
////
////    namespace pass::skybox
////    {
////        void SetupPass();
////        void Submit();
////    }   // namespace pass::skybox
////
////    namespace pass::shadow::shadowmapping
////    {
////        void SetupPass();
////        void SetupLight(const SpotLight& _light, const Transform& _lightTrans);
////        void Submit(const DrawItem& _item);
////    }   // namespace pass::shadow::shadowmapping
////
////    namespace pass::shadow::csm   // Cascaded Shadow Maps
////    {
////        void SetupPass();
////        void Submit(const DrawItem& _item);
////    }   // namespace pass::shadow::csm
////
////    namespace pass::shadow::osm   // Omni-directional Shadow Maps
////    {
////        void SetupPass();
////        void SetupLight(const PointLight& _light, const Transform& _lightTrans);
////        void Submit(const DrawItem& _item);
////    }   // namespace pass::shadow::osm
////
////    namespace pass::ssao
////    {
////        // Bake 함수들은 Init 과정에서 호출되기에 굳이 수동으로 호출할 필요는 없으나, SSAO 커널관련 환경설정이 바뀌면 BakeKernel은 호출해줘야함
////        void BakeKernel();   // 내부적으로 커널 생성.
////        void BakeNoise();    // 내부적으로 노이즈 생성.
////
////        void SetupPass(const GBufferParam& _param);
////        void Submit();
////    }   // namespace pass::ssao
////
////    namespace pass::ssaoblur_x
////    {
////        void SetupPass(const TextureHandle& _ssaoTexture);
////        void Submit();
////    }   // namespace pass::ssaoblur_x
////
////    namespace pass::ssaoblur_y
////    {
////        void SetupPass(const TextureHandle& _ssaoTexture);
////        void Submit();
////    }   // namespace pass::ssaoblur_y
////
////    namespace postprocess::bloom_x
////    {
////        void SetupPostProcess(const TextureHandle& _inputTexture);
////        void Submit();
////    }   // namespace postprocess::bloom_x
////
////    namespace postprocess::bloom_y
////    {
////        void SetupPostProcess(const TextureHandle& _inputTexture);
////        void Submit();
////    }   // namespace postprocess::bloom_y
////
////    namespace postprocess::bloom_combine
////    {
////        void SetupPostProcess(const TextureHandle& _baseTexture, const TextureHandle& _bloomTexture);
////        void Submit();
////    }   // namespace postprocess::bloom_combine
////
////    namespace postprocess::tone_mapping
////    {
////        void SetupPostProcess(const TextureHandle& _inputTexture);
////        void Submit();
////
////    }   // namespace postprocess::tone_mapping
////
////    namespace postprocess::fog
////    {
////        void SetupPostProcess(const TextureHandle& _inputTexture, const TextureHandle& _depthTexture);
////        void Submit();
////    }   // namespace postprocess::fog
////
////    namespace postprocess::passthrough
////    {
////        void SetupPostProcess(const TextureHandle& _inputTexture);
////        void Submit();
////    }   // namespace postprocess::passthrough
////
////    namespace postprocess::fxaa
////    {
////        void SetupPostProcess(const Environment& _env, const TextureHandle& _inputTexture);
////        void Submit();
////    }   // namespace postprocess::fxaa
////}
////}   // namespace jam::basic_renderer
