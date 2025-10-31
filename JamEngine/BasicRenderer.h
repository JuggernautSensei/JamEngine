#pragma once
#pragma once
#include "BasicRendererBase.h"
#include "RenderPass.h"

namespace jam
{

struct RenderView;
class Transform;
class Material;
class Mesh;
class Timer;
struct Frustum;
class RenderGraph;
struct Environment;

class BasicRenderer
{
public:
    static void Init();
    static void Shutdown();

    static void Render(const Vec3& _eyePos, const Mat4& _view, const Mat4& _proj);
    static void Render(const RenderView& _view);

    // constant buffer uploader
    static void UploadEnvironmentData(const Environment& _env);                                           // _env 가 변경될 때마다 호출 필요
    static void UploadCascadeShadowCasterData(const Frustum& _viewFrustumInW, const Environment& _env);   // _executor 에서 호출 필요
    static void UploadOmniDirectionalShadowCasterData(const Vec3& _pos, float _range);                    // _executor 에서 호출 필요

    static void BakeSSAONoiseTexture();   // 내부적으로 관리. 필요시에만 호출
    static void BakeSSAOKernel();         // 내부적으로 관리. 필요시에만 호출

    // draw call helpers
    static void Submit(const Mesh& _mesh, const Material& _material, const Transform& _transform);
    static void Submit(const Mesh& _mesh, const Transform& _transform);
    static void SubmitPointLight(const Vec3& _radiance, float _lightIntensity, const Vec3& _pos, float _fallOffStart, float _fallOffEnd);
    static void SubmitPointLightWithShadow(const Vec3& _radiance, float _lightIntensity, const Vec3& _pos, float _fallOffStart, float _fallOffEnd, TextureHandle _shadowMap);
    static void SubmitSpotLight(const Vec3& _radiance, float _lightIntensity, const Vec3& _pos, const Vec3& _dir, float _fallOffStart, float _fallOffEnd, float _innerConeAngle, float _outerConeAngle);
    static void SubmitSpotLightWithShadow(const Vec3& _radiance, float _lightIntensity, const Vec3& _pos, const Vec3& _dir, float _fallOffStart, float _fallOffEnd, float _innerConeAngle, float _outerConeAngle, TextureHandle _shadowMap);

    // render graph
    static void CompileRenderGraph();
    static void ClearRenderGraph();
    static void AddPass(RenderPass&& _pass);

    // resource factory
    JAM_NODISCARD static TextureHandle CreateTexture2D(Int32 _width, Int32 _height, eFormat _format, eAttachmentType _type, bool _bWrireOnly = false);
    JAM_NODISCARD static TextureHandle CreateTextureCube(Int32 _width, Int32 _height, eFormat _format, eAttachmentType _type, bool _bWrireOnly = false);
    JAM_NODISCARD static TextureHandle CreateTextureArray(Int32 _width, Int32 _height, Int32 _arraySize, eFormat _format, eAttachmentType _type, bool _bWrireOnly = false);
    JAM_NODISCARD static GBufferDesc   CreateGBuffer(Int32 _width, Int32 _height);

    // pass creators
    JAM_NODISCARD static RenderPass CreatePBRGBufferPass(const PBRGBufferPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreatePBRLightPass(const PBRLightPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreatePBRLightVolumePass(const PBRLightVolumePassDesc& _desc);
    JAM_NODISCARD static RenderPass CreatePBREnvironmentLightVolumePass(const PBREnvironmentLightVolumePassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateSkyboxPass(const SkyboxPassDesc& _desc);

    // shadow caster passes
    JAM_NODISCARD static CreateShadowCasterPassResult CreateShadowCasterPass(const ShadowCasterPassDesc& _desc);
    JAM_NODISCARD static CreateShadowCasterPassResult CreateOmniDirectionalShadowCasterPass(const ShadowCasterPassDesc& _desc);
    JAM_NODISCARD static CreateShadowCasterPassResult CreateCascadeShadowCasterPass(const ShadowCasterPassDesc& _desc);

    // ssao
    JAM_NODISCARD static CreateSSAOPassResult CreateSSAOPass(const SSAOPassDesc& _desc);

    // post process
    JAM_NODISCARD static CreatePostProcessPassResult CreatePostProcessToneMappingPass(const ToneMappingPassDesc& _desc);
    JAM_NODISCARD static CreatePostProcessPassResult CreatePostProcessFXAAPass(const FXAAPassDesc& _desc);
    JAM_NODISCARD static CreatePostProcessPassResult CreatePostProcessHeightFogPass(const HeightFogPassDesc& _desc);
    JAM_NODISCARD static CreatePostProcessPassResult CreatePostProcessPassThroughPass(const PassThroughPassDesc& _desc);
    JAM_NODISCARD static CreateBloomPassResult       CreatePostProcessBloomPass(const BloomPassDesc& _desc);
};

}   // namespace jam
