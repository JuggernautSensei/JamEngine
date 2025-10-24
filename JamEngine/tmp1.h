#pragma once
#include "Environment.h"
#include "GraphicsBase.h"
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
JAM_NODISCARD CreateCascadedShadowMapFrameResult        CreateCascadedShadowMapFrame(UInt32 _width, UInt32 _height,  eCascadeLevel _cascadeLevel);
JAM_NODISCARD CreateOmniDirectionalShadowMapFrameResult CreateOmniDirectionalShadowMapFrame(UInt32 _width, UInt32 _height);
JAM_NODISCARD CreateSSAOFrameBufferResult               CreateSSAOFrameBuffer(UInt32 _width, UInt32 _height);


}   // namespace jam