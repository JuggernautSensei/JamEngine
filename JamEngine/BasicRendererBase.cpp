#include "pch.h"

#include "BasicRendererBase.h"

#include "Gfx.h"

namespace jam
{

CreateGBufferFrameResult CreateGBufferFrame(const UInt32 _width, const UInt32 _height)
{
    CreateGBufferFrameResult result;

    result.albedoAoTex                = gfx::CreateTexture2D(_width, _height, eFormat::RGBA8_UNorm, 1, eTextureCreateFlags_RenderTarget);
    result.normalRoughnessMetallicTex = gfx::CreateTexture2D(_width, _height, eFormat::RGBA16_Float, 1, eTextureCreateFlags_RenderTarget);
    result.additionalLightTex         = gfx::CreateTexture2D(_width, _height, eFormat::RGBA16_Float, 1, eTextureCreateFlags_RenderTarget);
    result.depthTex                   = gfx::CreateTexture2D(_width, _height, eFormat::D24_UNorm_S8_UInt, 1, eTextureCreateFlags_DepthStencil);

    TextureHandle attachments[] = {
        result.albedoAoTex,
        result.normalRoughnessMetallicTex,
        result.additionalLightTex,
        result.depthTex,
    };

    result.frameBuffer = gfx::CreateFrameBuffer(attachments, false);
    return result;
}

CreatePostProcessFrameResult CreatePostProcessFrame(const UInt32 _width, const UInt32 _height, const eFormat _format)
{
    CreatePostProcessFrameResult result;

    result.colorTex    = gfx::CreateTexture2D(_width, _height, _format, 1, eTextureCreateFlags_RenderTarget);
    result.frameBuffer = gfx::CreateFrameBuffer(result.colorTex, false);
    return result;
}

CreateShadowMappingFrameResult CreateShadowMappingFrame(const UInt32 _width, const UInt32 _height)
{
    CreateShadowMappingFrameResult result;
    result.shadowMap   = gfx::CreateTexture2D(_width, _height, eFormat::D32_Float, 1, eTextureCreateFlags_DepthStencil);
    result.frameBuffer = gfx::CreateFrameBuffer(result.shadowMap, false);
    return result;
}

CreateCascadedShadowMapFrameResult CreateCascadedShadowMapFrame(const UInt32 _width, const UInt32 _height, const eCascadeLevel _cascadeLevel)
{
    Int32 cascadeLevel = 0;
    if (_cascadeLevel == eCascadeLevel::level2)
        cascadeLevel = 2;
    else if (_cascadeLevel == eCascadeLevel::level3)
        cascadeLevel = 3;
    else if (_cascadeLevel == eCascadeLevel::level4)
        cascadeLevel = 4;

    CreateCascadedShadowMapFrameResult result;
    result.shadowMap   = gfx::CreateTexture2D(_width, _height, eFormat::D32_Float, cascadeLevel, eTextureCreateFlags_DepthStencil);
    result.frameBuffer = gfx::CreateFrameBuffer(result.shadowMap, false);
    return result;
}

CreateOmniDirectionalShadowMapFrameResult CreateOmniDirectionalShadowMapFrame(const UInt32 _width, const UInt32 _height)
{
    CreateOmniDirectionalShadowMapFrameResult result;
    result.shadowMap   = gfx::CreateTextureCube(_width, _height, eFormat::D32_Float, eTextureCreateFlags_DepthStencil);
    result.frameBuffer = gfx::CreateFrameBuffer(result.shadowMap, false);
    return result;
}

CreateSSAOFrameBufferResult CreateSSAOFrameBuffer(const UInt32 _width, const UInt32 _height)
{
    CreateSSAOFrameBufferResult result;
    result.ssaoTex     = gfx::CreateTexture2D(_width, _height, eFormat::R8_UNorm, 1, eTextureCreateFlags_RenderTarget);
    result.frameBuffer = gfx::CreateFrameBuffer(result.ssaoTex, false);
    return result;
}

}   // namespace jam
