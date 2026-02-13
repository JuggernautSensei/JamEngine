#include "pch.h"

#include "GraphicsBase.h"

namespace Jam
{

RGBA RGBA::k_white       = RGBA(255, 255, 255, 255);
RGBA RGBA::k_black       = RGBA(0, 0, 0, 255);
RGBA RGBA::k_transparent = RGBA(0, 0, 0, 0);

RenderState RenderState::k_default   = {};
RenderState RenderState::k_depthTest = { ePrimitive::TriangleList, eCullMode::Back, false, false, false, false, false, false, false, false, eCompareFunc::LessEqual, true };   // namespace Jam

BlendState BlendState::k_opaque             = {};
BlendState BlendState::k_alphaBlend         = { true, eBlend::SrcAlpha, eBlend::InvSrcAlpha, eBlendOp::Add, eBlend::One, eBlend::InvSrcAlpha, eBlendOp::Add, eWriteFlags::All };
BlendState BlendState::k_additive           = { true, eBlend::One, eBlend::One, eBlendOp::Add, eBlend::One, eBlend::One, eBlendOp::Add, eWriteFlags::All };
BlendState BlendState::k_premultipliedAlpha = { true, eBlend::One, eBlend::InvSrcAlpha, eBlendOp::Add, eBlend::One, eBlend::InvSrcAlpha, eBlendOp::Add, eWriteFlags::All };
BlendState BlendState::k_subtractive        = { true, eBlend::InvDstColor, eBlend::Zero, eBlendOp::Add, eBlend::InvDstAlpha, eBlend::Zero, eBlendOp::Add, eWriteFlags::All };
BlendState BlendState::k_multiply           = { true, eBlend::DstColor, eBlend::Zero, eBlendOp::Add, eBlend::DstAlpha, eBlend::Zero, eBlendOp::Add, eWriteFlags::All };
BlendState BlendState::k_screen             = { true, eBlend::One, eBlend::InvSrcColor, eBlendOp::Add, eBlend::One, eBlend::InvSrcAlpha, eBlendOp::Add, eWriteFlags::All };
BlendState BlendState::k_overlay            = { true, eBlend::DstColor, eBlend::InvSrcColor, eBlendOp::Add, eBlend::One, eBlend::Zero, eBlendOp::Add, eWriteFlags::All };

StencilState StencilState::k_disable = {};

SamplerState SamplerState::k_pointClamp  = { eTextureFilter::MinPoint_MagPoint_MipPoint, eTextureAddressMode::Clamp, eTextureAddressMode::Clamp, eTextureAddressMode::Clamp, eCompareFunc::None };
SamplerState SamplerState::k_linearClamp = { eTextureFilter::MinLinear_MagLinear_MipLinear, eTextureAddressMode::Clamp, eTextureAddressMode::Clamp, eTextureAddressMode::Clamp, eCompareFunc::None };
SamplerState SamplerState::k_pointWrap   = { eTextureFilter::MinPoint_MagPoint_MipPoint, eTextureAddressMode::Wrap, eTextureAddressMode::Wrap, eTextureAddressMode::Wrap, eCompareFunc::None };
SamplerState SamplerState::k_linearWrap  = { eTextureFilter::MinLinear_MagLinear_MipLinear, eTextureAddressMode::Wrap, eTextureAddressMode::Wrap, eTextureAddressMode::Wrap, eCompareFunc::None };

bool IsDepthFormat(const eTextureFormat _format)
{
    switch (_format)
    {
        case eTextureFormat::D16_UNorm:
        case eTextureFormat::D24_UNorm_S8_UInt:
        case eTextureFormat::D32_Float:
        case eTextureFormat::D32_Float_S8_UInt:
            return true;

        default:
            return false;
    }
}

Int32 BytePerPixelOf(const eTextureFormat _format)
{
    switch (_format)
    {
        case eTextureFormat::Unknown:
            return 0;

        case eTextureFormat::R8_UNorm:
        case eTextureFormat::R8_SNorm:
        case eTextureFormat::R8_UInt:
        case eTextureFormat::R8_Int:
            return 1;

        case eTextureFormat::R16_UNorm:
        case eTextureFormat::R16_SNorm:
        case eTextureFormat::R16_UInt:
        case eTextureFormat::R16_Int:
        case eTextureFormat::R16_Float:
            return 2;

        case eTextureFormat::R32_UInt:
        case eTextureFormat::R32_Int:
        case eTextureFormat::R32_Float:
            return 4;

        case eTextureFormat::RG8_UNorm:
        case eTextureFormat::RG8_SNorm:
        case eTextureFormat::RG8_UInt:
        case eTextureFormat::RG8_Int:
            return 2;

        case eTextureFormat::RG16_UNorm:
        case eTextureFormat::RG16_SNorm:
        case eTextureFormat::RG16_UInt:
        case eTextureFormat::RG16_Int:
        case eTextureFormat::RG16_Float:
            return 4;

        case eTextureFormat::RG32_UInt:
        case eTextureFormat::RG32_Int:
        case eTextureFormat::RG32_Float:
            return 8;

        case eTextureFormat::RGBA8_UNorm:
        case eTextureFormat::RGBA8_SNorm:
        case eTextureFormat::RGBA8_UInt:
        case eTextureFormat::RGBA8_Int:
        case eTextureFormat::RGBA8_UNorm_SRGB:
        case eTextureFormat::BGRA8_UNorm:
        case eTextureFormat::BGRA8_UNorm_SRGB:
            return 4;

        case eTextureFormat::RGBA16_UNorm:
        case eTextureFormat::RGBA16_SNorm:
        case eTextureFormat::RGBA16_UInt:
        case eTextureFormat::RGBA16_Int:
        case eTextureFormat::RGBA16_Float:
            return 8;

        case eTextureFormat::RGBA32_UInt:
        case eTextureFormat::RGBA32_Int:
        case eTextureFormat::RGBA32_Float:
            return 16;

        case eTextureFormat::D16_UNorm:
            return 2;

        case eTextureFormat::D24_UNorm_S8_UInt:
            return 4;

        case eTextureFormat::D32_Float:
            return 4;

        case eTextureFormat::D32_Float_S8_UInt:
            return 8;

        default:
            JAM_UNREACHABLE;
    }
}

Int32 CalcNumMips(
    const Int32 _width,
    const Int32 _height,
    const Int32 _depth)
{
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "텍스처 크기는 0보다 커야합니다.");
    return 1 + Log2(Max(_width, _height, _depth));
}

TextureResolution CalcTextureResolution(
    const Int32 _width,
    const Int32 _height,
    const Int32 _depth,
    const Int32 _mipIndex)
{
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "texture width, height, depth must be greater than 0");
    JAM_ASSERT(_mipIndex >= 0, "mip index must be non-negative");

    const Int32 mipWidth  = Max(1, _width >> _mipIndex);
    const Int32 mipHeight = Max(1, _height >> _mipIndex);
    const Int32 mipDepth  = Max(1, _depth >> _mipIndex);
    return { mipWidth, mipHeight, mipDepth };
}

Int32 CalcTextureIndex(
    const Int32 _mipIndex,
    const Int32 _arrayIndex,
    const Int32 _numMips)
{
    return _mipIndex + _arrayIndex * _numMips;
}

}   // namespace Jam