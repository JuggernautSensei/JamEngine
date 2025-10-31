#pragma once
#include "Environment.h"
#include "GraphicsBase.h"
#include "RenderPass.h"
#include "VertexLayout.h"

namespace jam
{

enum class eFxaaQuality
{
    VeryLow = 0,
    Low,
    Medium,
    High,
    VeryHigh,
    Ultra,
};

struct GBufferDesc
{
    TextureHandle albedoAoTexture         = k_invalidHandle;
    TextureHandle normalRoughnessTexture  = k_invalidHandle;
    TextureHandle emissiveMetallicTexture = k_invalidHandle;
    TextureHandle depthTexture            = k_invalidHandle;
};

struct PBRGBufferPassDesc
{
    FrameBufferHandle     frameBuffer = k_invalidHandle;
    Viewport              viewport    = {};
    std::function<void()> executor    = nullptr;
};

struct PBRLightPassDesc
{
    FrameBufferHandle frameBuffer = k_invalidHandle;
    Viewport          viewport    = {};
    GBufferDesc       gBufferDesc = {};

    TextureHandle irradianceMap = k_invalidHandle;   // essential
    TextureHandle specularMap   = k_invalidHandle;   // essential
    TextureHandle brdfLUT       = k_invalidHandle;   // essential
    TextureHandle ssaoTexture   = k_invalidHandle;   // optional
};

struct PBRLightVolumePassDesc
{
    FrameBufferHandle     frameBuffer = k_invalidHandle;
    Viewport              viewport    = {};
    GBufferDesc           gBufferDesc = {};
    std::function<void()> executor    = nullptr;
};

struct PBREnvironmentLightVolumePassDesc
{
    FrameBufferHandle frameBuffer = k_invalidHandle;
    Viewport          viewport    = {};
    GBufferDesc       gBufferDesc = {};
    TextureHandle     shadowMap   = k_invalidHandle;   // optional
};

struct SkyboxPassDesc
{
    FrameBufferHandle frameBuffer   = k_invalidHandle;
    Viewport          viewport      = {};
    TextureHandle     skyboxTexture = k_invalidHandle;   // optional
};

struct ShadowCasterPassDesc
{
    Int32                 lengthOfSide = 0;
    eCascadeLevel         cascadeLevel = eCascadeLevel::level2;
    std::function<void()> executor     = nullptr;
};

struct SSAOPassDesc
{
    Int32       width;
    Int32       height;
    GBufferDesc gBufferDesc = {};
};

struct CreateSSAOPassResult
{
    std::array<RenderPass, 3> passes;        // ssao, ssao blur horizontal, ssao blur vertical
    TextureHandle             ssaoTexture;   // final ssao result texture
};

struct BloomPassDesc
{
    Int32         width;
    Int32         height;
    TextureHandle inputTexture = {};
    Int32         bloomLevel   = 3;
};

struct ToneMappingPassDesc
{
    Int32         width;
    Int32         height;
    TextureHandle inputTexture = k_invalidHandle;
};

struct FXAAPassDesc
{
    Int32         width;
    Int32         height;
    TextureHandle inputTexture = k_invalidHandle;
    eFxaaQuality  quality      = eFxaaQuality::Medium;
};

struct HeightFogPassDesc
{
    Int32         width;
    Int32         height;
    TextureHandle inputTexture = k_invalidHandle;
    TextureHandle depthTexture = k_invalidHandle;
};

struct PassThroughPassDesc
{
    Int32         width;
    Int32         height;
    TextureHandle inputTexture = k_invalidHandle;
};

struct CreateShadowCasterPassResult
{
    RenderPass    pass;
    TextureHandle shadowMap;
};

struct CreatePostProcessPassResult
{
    RenderPass    pass;
    TextureHandle outputTexture;
};

struct CreateBloomPassResult
{
    std::vector<RenderPass> passes;
    TextureHandle           outputTexture;
};

////////////////////////////////
///          VERTEX          ///
////////////////////////////////

struct VertexPosTex2D
{
    Vec2 position;
    Vec2 texCoord;

    inline static VertexLayout s_layout = []
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float2)
            .Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
        return layout;
    }();
};

struct VertexStaticMesh
{
    Vec3 position;
    Vec3 normal;
    Vec3 tangent;
    Vec2 texCoord0;
    Vec2 texCoord1;

    inline static VertexLayout s_layout = []
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3)
            .Add(eVertexAttribute::Normal, eAttributeFormat::Float3)
            .Add(eVertexAttribute::Tangent, eAttributeFormat::Float3)
            .Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2)
            .Add(eVertexAttribute::TexCoord1, eAttributeFormat::Float2);
        return layout;
    }();
};

struct VertexPos3D
{
    Vec3 position;

    inline static VertexLayout s_layout = []
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
        return layout;
    }();
};

///////////////////////////////
///          MISC           ///
///////////////////////////////

enum class eAttachmentType : Int8
{
    RenderTarget,
    DepthStencil
};

}   // namespace jam