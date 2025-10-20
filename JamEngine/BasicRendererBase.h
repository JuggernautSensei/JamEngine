#pragma once
#include "Environment.h"
#include "GfxBase.h"
#include "VertexLayout.h"

namespace jam
{

struct GBufferParam
{
    TextureHandle albedoAoTex;
    TextureHandle normalRoughnessMetallicTex;
    TextureHandle additionalLightTex;
    TextureHandle depthTex;
};

struct CreateGBufferFrameResult
{
    TextureHandle     albedoAoTex;
    TextureHandle     normalRoughnessMetallicTex;
    TextureHandle     additionalLightTex;
    TextureHandle     depthTex;
    FrameBufferHandle frameBuffer;
};

struct CreatePostProcessFrameResult
{
    TextureHandle     colorTex;
    FrameBufferHandle frameBuffer;
};

struct CreateShadowMappingFrameResult
{
    TextureHandle     shadowMap;
    FrameBufferHandle frameBuffer;
};

struct CreateCascadedShadowMapFrameResult
{
    TextureHandle     shadowMap;
    FrameBufferHandle frameBuffer;
};

struct CreateOmniDirectionalShadowMapFrameResult
{
    TextureHandle     shadowMap;
    FrameBufferHandle frameBuffer;
};

struct CreateSSAOFrameBufferResult
{
    FrameBufferHandle frameBuffer = k_invalidHandle;   
    TextureHandle     ssaoTex     = k_invalidHandle;
};

JAM_NODISCARD CreateGBufferFrameResult                  CreateGBufferFrame(UInt32 _width, UInt32 _height);
JAM_NODISCARD CreatePostProcessFrameResult              CreatePostProcessFrame(UInt32 _width, UInt32 _height, eFormat _format);
JAM_NODISCARD CreateShadowMappingFrameResult            CreateShadowMappingFrame(UInt32 _width, UInt32 _height);
JAM_NODISCARD CreateCascadedShadowMapFrameResult        CreateCascadedShadowMapFrame(UInt32 _width, UInt32 _height, const eCascadeLevel _cascadeLevel);
JAM_NODISCARD CreateOmniDirectionalShadowMapFrameResult CreateOmniDirectionalShadowMapFrame(UInt32 _width, UInt32 _height);
JAM_NODISCARD CreateSSAOFrameBufferResult               CreateSSAOFrameBuffer(UInt32 _width, UInt32 _height);

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