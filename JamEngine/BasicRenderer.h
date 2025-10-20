#pragma once
#include "BasicRendererBase.h"
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

/// [read me]
/// basic_renderer 네임스페이스는 jam engine 의 내장 렌더러 기능들을 구현합니다.
/// 사용하지 않아도 좋으나, jam engine 의 렌더링 파이프라인을 파악하는데 도움이 될 수 있습니다.
/// 사용할 시에 어플리케이션 시작과 끝에 각각 Init / Shutdown 을 호출해야 합니다.
/// 반드시 Begin / End 쌍으로 감싸서 사용해야 합니다.
/// 기본적인 사용법은 다음과 같습니다.
/// SetUp*** : 각 패스에 필요한 리소스들을 설정합니다. 매 프레임 한 번만 호출하면 됩니다.
/// Submit : 각 패스에 렌더링할 오브젝트를 제출합니다. 드로우 콜을 발생시킵니다.
/// Bake*** : Pass에 필요한 데이터를 미리 계산합니다. 대부분의 경우 따로 호출할 필요가 없습니다. 환경이 바뀔경우 Begin 가 내부적으로 호출합니다.
/// Create*** : 해당 Pass가 원하는 형태의 리소스를 생성합니다. 리소스의 수명관리는 호출자가 책임집니다. 이는 단순 헬퍼함수입니다.
/// GfxState, BlendState, StencilState, SamplerState 등은 각 Pass 내부에서 설정되기에 따로 설정할 필요는 없습니다.
/// 다른 형태의 상태가 필요하면, SetUpPass 이후 별도로 호출하여 덮어쓰면 됩니다. 이는 큰 오버헤드를 발생시키지 않습니다.
/// 단, FrameBuffer와 Viewport는 Pass 외부에서 설정해줘야 합니다.

namespace jam::basic_renderer
{

void Init();
void Shutdown();

void Begin(const Camera& _camera, const Transform& _cameraTrans, const Environment& _env);
void Begin(const Camera& _camera, const Transform& _cameraTrans, const Frustum& _viewFrustumInW, const Environment& _env);   // 계산된 Frustum을 직접 넘겨주는 마이크로 최적화 버전 (기존 함수는 내부적으로 Frustum 계산)
void End();

JAM_NODISCARD const Timer&   GetRendererCPUProfileTimer();   // end 이후에 호출해야 올바른 값이 나옴
JAM_NODISCARD CullingSystem& GetCullingSystem();

namespace pass::gbuffer::pbr
{
    // frame buffer
    // ㄴ rt0: albedo ao
    // ㄴ rt1: normal roughness metallic
    // ㄴ rt2: additional light
    // ㄴ ds: depth
    void SetupPass();
    void Submit(const DrawItem& _item);

}   // namespace pass::gbuffer::pbr

namespace pass::lighting::pbr
{
    void SetupPass(const GBufferParam& _gBufferParam);
    void SetupSSAOTexture(TextureHandle _ssaoTex);
    void Submit();

}   // namespace pass::lighting::pbr

namespace pass::lightvolume::pbr
{
    void SetupPass(const GBufferParam& _gBufferParam);
    void Submit(const PointLight& _light, const Transform& _lightTrans);
    void Submit(const SpotLight& _light, const Transform& _lightTrans);
    void Submit(const PointLight& _light, const Transform& _lightTrans, TextureHandle _shadowMap);
    void Submit(const SpotLight& _light, const Transform& _lightTrans, TextureHandle _shadowMap);

}   // namespace pass::lightvolume::pbr

namespace pass::environmentlightvolume::pbr
{
    void SetupPass(const GBufferParam& _gBufferParam);
    void Submit();
    void Submit(TextureHandle _shadowMap);
}   // namespace pass::environmentlightvolume::pbr

namespace pass::skybox
{
    void SetupPass();
    void Submit();
}   // namespace pass::skybox

namespace pass::shadow::shadowmapping
{
    void SetupPass();
    void SetupLight(const SpotLight& _light, const Transform& _lightTrans);
    void Submit(const DrawItem& _item);
}   // namespace pass::shadow::shadowmapping

namespace pass::shadow::csm   // Cascaded Shadow Maps
{
    void SetupPass();
    void Submit(const DrawItem& _item);
}   // namespace pass::shadow::csm

namespace pass::shadow::osm   // Omni-directional Shadow Maps
{
    void SetupPass();
    void SetupLight(const PointLight& _light, const Transform& _lightTrans);
    void Submit(const DrawItem& _item);
}   // namespace pass::shadow::osm

namespace pass::ssao
{
    // Bake 함수들은 Init 과정에서 호출되기에 굳이 수동으로 호출할 필요는 없으나, SSAO 커널관련 환경설정이 바뀌면 BakeKernel은 호출해줘야함
    void BakeKernel();   // 내부적으로 커널 생성.
    void BakeNoise();    // 내부적으로 노이즈 생성. 

    void SetupPass(const GBufferParam& _param);
    void Submit();
}   // namespace pass::ssao

namespace pass::ssaoblur_x
{
    void SetupPass(const TextureHandle& _ssaoTexture);
    void Submit();
}   // namespace pass::ssaoblur_x

namespace pass::ssaoblur_y
{
    void SetupPass(const TextureHandle& _ssaoTexture);
    void Submit();
}   // namespace pass::ssaoblur_y

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

}   // namespace jam::basic_renderer
