#include "pch.h"

#include "GraphicsBase.h"

namespace jam
{

RGBA RGBA::k_black = RGBA(0, 0, 0, 255);
RGBA RGBA::k_white = RGBA(255, 255, 255, 255);

GfxState GfxState::k_default = {};

GfxState GfxState::k_depthTest = {
    ePrimitive::TriangleList,
    eCullMode::Back,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    eCompareFunc::LessEqual,
    true
};

BlendState BlendState::k_opaque = {};

BlendState BlendState::k_alpha = {
    true,
    eBlend::SrcAlpha,
    eBlend::InvSrcAlpha,
    eBlendOp::Add,
    eBlend::One,
    eBlend::InvSrcAlpha,
    eBlendOp::Add,
    eWriteFlags_All
};

BlendState BlendState::k_additive = {
    true,
    eBlend::One,
    eBlend::One,
    eBlendOp::Add,
    eBlend::One,
    eBlend::One,
    eBlendOp::Add,
    eWriteFlags_All
};

BlendState BlendState::k_overlay = {
    true,
    eBlend::DstColor,
    eBlend::InvSrcColor,
    eBlendOp::Add,
    eBlend::One,
    eBlend::Zero,
    eBlendOp::Add,
    eWriteFlags_All
};

BlendState BlendState::k_premult = {
    true,
    eBlend::One,
    eBlend::InvSrcAlpha,
    eBlendOp::Add,
    eBlend::One,
    eBlend::InvSrcAlpha,
    eBlendOp::Add,
    eWriteFlags_All
};

BlendState BlendState::k_subtractive = {
    true,
    eBlend::InvDstColor,
    eBlend::Zero,
    eBlendOp::Add,
    eBlend::InvDstAlpha,
    eBlend::Zero,
    eBlendOp::Add,
    eWriteFlags_All
};

BlendState BlendState::k_multiply = {
    true,
    eBlend::DstColor,
    eBlend::Zero,
    eBlendOp::Add,
    eBlend::DstAlpha,
    eBlend::Zero,
    eBlendOp::Add,
    eWriteFlags_All
};

BlendState BlendState::k_screen = {
    true,
    eBlend::One,
    eBlend::InvSrcColor,
    eBlendOp::Add,
    eBlend::One,
    eBlend::InvSrcAlpha,
    eBlendOp::Add,
    eWriteFlags_All
};

BlendState BlendState::k_darkening = {
    true,
    eBlend::DstColor,
    eBlend::SrcColor,
    eBlendOp::Add,
    eBlend::DstAlpha,
    eBlend::SrcAlpha,
    eBlendOp::Add,
    eWriteFlags_All
};

BlendState BlendState::k_lightening = {
    true,
    eBlend::One,
    eBlend::InvSrcColor,
    eBlendOp::Add,
    eBlend::One,
    eBlend::InvSrcAlpha,
    eBlendOp::Add,
    eWriteFlags_All
};

StencilState StencilState::k_disable = {};

SamplerState SamplerState::k_default = {};

SamplerState SamplerState::k_pointClamp = {
    eTextureFilter::MinPoint_MagPoint_MipPoint,
    eTextureAddress::Clamp,
    eTextureAddress::Clamp,
    eTextureAddress::Clamp,
    eCompareFunc::None
};
SamplerState SamplerState::k_linearClamp = {
    eTextureFilter::MinLinear_MagLinear_MipLinear,
    eTextureAddress::Clamp,
    eTextureAddress::Clamp,
    eTextureAddress::Clamp,
    eCompareFunc::None
};

SamplerState SamplerState::k_pointWrap = {
    eTextureFilter::MinPoint_MagPoint_MipPoint,
    eTextureAddress::Wrap,
    eTextureAddress::Wrap,
    eTextureAddress::Wrap,
    eCompareFunc::None
};

SamplerState SamplerState::k_linearWrap = {
    eTextureFilter::MinLinear_MagLinear_MipLinear,
    eTextureAddress::Wrap,
    eTextureAddress::Wrap,
    eTextureAddress::Wrap,
    eCompareFunc::None
};

SamplerState SamplerState::k_shadow = {
    eTextureFilter::MinLinear_MagLinear_MipLinear,
    eTextureAddress::Clamp,
    eTextureAddress::Clamp,
    eTextureAddress::Clamp,
    eCompareFunc::LessEqual
};

Int32 GetPixelByte(const eFormat _format)
{
    switch (_format)
    {
        case Unknown:
            return 0;

        case R8_UNorm:
        case R8_SNorm:
        case R8_UInt:
        case R8_Int:
            return 1;

        case R16_UNorm:
        case R16_SNorm:
        case R16_UInt:
        case R16_Int:
        case R16_Float:
            return 2;

        case R32_UInt:
        case R32_Int:
        case R32_Float:
            return 4;

        case RG8_UNorm:
        case RG8_SNorm:
        case RG8_UInt:
        case RG8_Int:
            return 2;

        case RG16_UNorm:
        case RG16_SNorm:
        case RG16_UInt:
        case RG16_Int:
        case RG16_Float:
            return 4;

        case RG32_UInt:
        case RG32_Int:
        case RG32_Float:
            return 8;

        case RGBA8_UNorm:
        case RGBA8_SNorm:
        case RGBA8_UInt:
        case RGBA8_Int:
            return 4;

        case RGBA16_UNorm:
        case RGBA16_SNorm:
        case RGBA16_UInt:
        case RGBA16_Int:
        case RGBA16_Float:
            return 8;

        case RGBA32_UInt:
        case RGBA32_Int:
        case RGBA32_Float:
            return 16;

        case D16_UNorm:
            return 2;

        case D24_UNorm_S8_UInt:
            return 4;

        case D32_Float:
            return 4;

        case D32_Float_S8_UInt:
            return 8;

        default:
            JAM_ASSERT(0, "Unknown format: {}", EnumToString(_format));
            return 0;
    }
}

bool IsDepthFormat(const eFormat _format)
{
    switch (_format)
    {
        case D16_UNorm:
        case D24_UNorm_S8_UInt:
        case D32_Float:
        case D32_Float_S8_UInt:
            return true;

        default:
            return false;
    }
}

Int32 GetIndexFormatByte(const eIndexFormat _format)
{
    return _format == eIndexFormat::U16 ? 2 : 4;
}

Int32 CalcMipsCount(const Int32 _width, const Int32 _height, const Int32 _depth)
{
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "텍스처 크기는 0보다 커야합니다.");
    return 1 + Log2_Int32(Max(_width, _height, _depth));
}

std::tuple<Int32, Int32, Int32> CalcMipSize(
    const Int32 _width,
    const Int32 _height,
    const Int32 _depth,
    const Int32 _mipIndex)
{
    JAM_ASSERT(_width >= 0 && _height >= 0 && _depth >= 0, "텍스처 크기는 0 이상이어야 합니다.");
    JAM_ASSERT(_mipIndex >= 0, "밉 인덱스는 0 이상이어야 합니다.");

    Int32 mipWidth  = Max(1, _width >> _mipIndex);
    Int32 mipHeight = Max(1, _height >> _mipIndex);
    Int32 mipDepth  = Max(1, _depth >> _mipIndex);
    return { mipWidth, mipHeight, mipDepth };
}

Int32 CalcTextureIndex(const Int32 _mipIndex, const Int32 _arrayIndex, const Int32 _mipsCount)
{
    return _mipIndex + _arrayIndex * _mipsCount;
}

}   // namespace jam