#pragma once
#include "GraphicsBase.h"
#include "VertexLayout.h"

namespace jam
{

struct GBufferDesc
{
    TextureHandle albedoAOTexture                = k_invalidHandle;
    TextureHandle normalRoughnessMetallicTexture = k_invalidHandle;
    TextureHandle additionalLightTexture         = k_invalidHandle;
    TextureHandle depthTexture                   = k_invalidHandle;
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
    TextureHandle     ssaoTexture = k_invalidHandle;   // optional
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
    TextureHandle     shadowMap   = k_invalidHandle;
};

struct SkyboxPassDesc
{
    FrameBufferHandle frameBuffer = k_invalidHandle;
    Viewport          viewport    = {};
};

struct ShadowCasterPassDesc
{
    FrameBufferHandle     frameBuffer = k_invalidHandle;
    Viewport              viewport    = {};
    std::function<void()> executor    = nullptr;
};

struct SSAOPassDesc
{
    FrameBufferHandle frameBuffer  = k_invalidHandle;
    Viewport          viewport     = {};
    GBufferDesc       gBufferDesc  = {};
};

struct SSAOBlurPassDesc
{
    FrameBufferHandle frameBuffer = k_invalidHandle;
    Viewport          viewport    = {};
    TextureHandle     ssaoTexture = k_invalidHandle;
    bool              bHorizontal = true;   // true: horizontal blur, false: vertical blur
};

struct BlurPassDesc
{
    FrameBufferHandle frameBuffer  = k_invalidHandle;
    Viewport          viewport     = {};
    TextureHandle     inputTexture = k_invalidHandle;
    bool              bHorizontal  = true;   // true: horizontal blur, false: vertical blur
};

struct CombinePassDesc
{
    FrameBufferHandle frameBuffer   = k_invalidHandle;
    Viewport          viewport      = {};
    TextureHandle     inputTexture0 = k_invalidHandle;
    TextureHandle     inputTexture1 = k_invalidHandle;
};

struct ToneMappingPassDesc
{
    FrameBufferHandle frameBuffer  = k_invalidHandle;
    Viewport          viewport     = {};
    TextureHandle     inputTexture = k_invalidHandle;
};

struct FXAAPassDesc
{
    FrameBufferHandle frameBuffer  = k_invalidHandle;
    Viewport          viewport     = {};
    TextureHandle     inputTexture = k_invalidHandle;
};

struct HeightFogPass
{
    FrameBufferHandle frameBuffer  = k_invalidHandle;
    Viewport          viewport     = {};
    TextureHandle     inputTexture = k_invalidHandle;
    TextureHandle     depthTexture = k_invalidHandle;
};

struct PassThroughPassDesc
{
    FrameBufferHandle frameBuffer  = k_invalidHandle;
    Viewport          viewport     = {};
    TextureHandle     inputTexture = k_invalidHandle;
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

}   // namespace jam