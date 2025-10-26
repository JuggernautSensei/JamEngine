#pragma once
#include "BasicRendererBase.h"
#include "RenderPass.h"

namespace jam
{

struct Transform;
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

    static void Render(const Vec3& _eyePos, const Mat4& _view, const Mat4& _proj, const Environment& _env);

    JAM_NODISCARD static const Timer& GetRenderCPUProfileTimer();

    // utility
    static void UploadCascadeShadowCasterData(const Frustum& _viewFrustumInW);           // _executor 에서 호출 필요
    static void UploadOmniDirectionalShadowCasterData(const Vec3& _pos, float _range);   // _executor 에서 호출 필요

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
    static void AddPass(const RenderPass& _pass);

    // resource factory
    JAM_NODISCARD static TextureHandle CreateTexture2D(Int32 _width, Int32 _height, eFormat _format, bool _bRTVAttachable, bool _bDSVAttachable);
    JAM_NODISCARD static TextureHandle CreateTextureCube(Int32 _width, Int32 _height, eFormat _format, bool _bRTVAttachable, bool _bDSVAttachable);
    JAM_NODISCARD static TextureHandle CreateTextureArray(Int32 _width, Int32 _height, Int32 _arraySize, eFormat _format, bool _bRTVAttachable, bool _bDSVAttachable);

    JAM_NODISCARD static GBufferDesc   CreateGBuffer(Int32 _width, Int32 _height);
    JAM_NODISCARD static TextureHandle CreateShadowMap(Int32 _sideLength);
    JAM_NODISCARD static TextureHandle CreateShadowCubeMap(Int32 _sideLength);
    JAM_NODISCARD static TextureHandle CreateShadowCascadeMap(Int32 _sideLength, Int32 _cascadeCount);
    JAM_NODISCARD static TextureHandle CreateSSAOTexture(Int32 _width, Int32 _height);
    JAM_NODISCARD static TextureHandle CreatePostProcessTexture(Int32 _width, Int32 _height, bool _bHdr);   // RGBA16_Float or RGBA8_UNorm

    // pass creators
    JAM_NODISCARD static RenderPass CreatePBRGBufferPass(const PBRGBufferPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreatePBRLightPass(const PBRLightPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreatePBRLightVolumePass(const PBRLightVolumePassDesc& _desc);
    JAM_NODISCARD static RenderPass CreatePBREnvironmentLightVolumePass(const PBREnvironmentLightVolumePassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateSkyboxPass(const SkyboxPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateShadowCasterPass(const ShadowCasterPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateOmniDirectionalShadowCasterPass(const ShadowCasterPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateCascadeShadowCasterPass(const ShadowCasterPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateSSAOPass(const SSAOPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateSSAOBBlurPass(const SSAOBlurPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateBlurPass(const BlurPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateCombinePass(const CombinePassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateToneMappingPass(const ToneMappingPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateFXAAPass(const FXAAPassDesc& _desc);
    JAM_NODISCARD static RenderPass CreateHeightFogPass(const HeightFogPass& _desc);
    JAM_NODISCARD static RenderPass CreatePassThroughPass(const PassThroughPassDesc& _desc);
};

}   // namespace jam
