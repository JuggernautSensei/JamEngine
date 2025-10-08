#include "pch.h"

#include "PostProcess.h"

#include "BaseUtilities.h"
#include "EmbeddedShader_base.h"
#include "Gfx.h"

namespace jam
{

PostProcess::PostProcess()
{
    m_postProcessCbufferHandle = gfx::CreateConstantBuffer(CB_GLOBAL 
}

PostProcess::PostProcess(PostProcess&&) noexcept
{
}

PostProcess& PostProcess::operator=(PostProcess&&) noexcept
{
}

void PostProcess::AddFilter(
    const eFilterType _type,
    const Int32       _width,
    const Int32       _height,
    const eFormat     _format)
{
    Filter filter;
    filter.type               = _type;
    filter.viewport           = Viewport { 0, 0, NarrowCast<UInt16>(_width), NarrowCast<UInt16>(_height) };
    filter.frameBufferHandle  = gfx::CreateFrameBuffer(_width, _height, _format);
    filter.textureHandleCache = gfx::GetRenderTargetTexture(filter.frameBufferHandle);
    // filter.programHandle = ...
}

void PostProcess::Submit(TextureHandle _inputTexture, FrameBufferHandle _outputFrameBuffer)
{
}

void PostProcess::Reset()
{
}

}   // namespace jam