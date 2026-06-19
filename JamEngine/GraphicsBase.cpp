#include "pch.h"

#include "GraphicsBase.h"

#include "Bit.h"

namespace Jam
{

RGBA RGBA::kWhite       = RGBA(255, 255, 255, 255);
RGBA RGBA::kBlack       = RGBA(0, 0, 0, 255);
RGBA RGBA::kTransparent = RGBA(0, 0, 0, 0);

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

Int32 BitPerPixelOf(const eTextureFormat _format)
{
    switch (_format)
    {
        case eTextureFormat::Unknown:
            return 0;

        case eTextureFormat::R8_UNorm:
        case eTextureFormat::R8_SNorm:
        case eTextureFormat::R8_UInt:
        case eTextureFormat::R8_Int:
            return 8;

        case eTextureFormat::R16_UNorm:
        case eTextureFormat::R16_SNorm:
        case eTextureFormat::R16_UInt:
        case eTextureFormat::R16_Int:
        case eTextureFormat::R16_Float:
            return 16;

        case eTextureFormat::R32_UInt:
        case eTextureFormat::R32_Int:
        case eTextureFormat::R32_Float:
            return 32;

        case eTextureFormat::RG8_UNorm:
        case eTextureFormat::RG8_SNorm:
        case eTextureFormat::RG8_UInt:
        case eTextureFormat::RG8_Int:
            return 16;

        case eTextureFormat::RG16_UNorm:
        case eTextureFormat::RG16_SNorm:
        case eTextureFormat::RG16_UInt:
        case eTextureFormat::RG16_Int:
        case eTextureFormat::RG16_Float:
            return 32;

        case eTextureFormat::RG32_UInt:
        case eTextureFormat::RG32_Int:
        case eTextureFormat::RG32_Float:
            return 64;

        case eTextureFormat::RGBA8_UNorm:
        case eTextureFormat::RGBA8_SNorm:
        case eTextureFormat::RGBA8_UInt:
        case eTextureFormat::RGBA8_Int:
        case eTextureFormat::RGBA8_UNorm_SRGB:
        case eTextureFormat::BGRA8_UNorm:
        case eTextureFormat::BGRA8_UNorm_SRGB:
            return 32;

        case eTextureFormat::RGBA16_UNorm:
        case eTextureFormat::RGBA16_SNorm:
        case eTextureFormat::RGBA16_UInt:
        case eTextureFormat::RGBA16_Int:
        case eTextureFormat::RGBA16_Float:
            return 64;

        case eTextureFormat::RGBA32_UInt:
        case eTextureFormat::RGBA32_Int:
        case eTextureFormat::RGBA32_Float:
            return 128;

        case eTextureFormat::D16_UNorm:
            return 16;

        case eTextureFormat::D24_UNorm_S8_UInt:
            return 32;

        case eTextureFormat::D32_Float:
            return 32;

        case eTextureFormat::D32_Float_S8_UInt:
            return 64;

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
    return 1 + static_cast<Int32>(Log2(static_cast<float>(Max(_width, _height, _depth))));
}

Vec3i CalcTextureResolution(
    const Int32 _width,
    const Int32 _height,
    const Int32 _depth,
    const Int32 _mip)
{
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "texture width, height, depth must be greater than 0");
    JAM_ASSERT(_mip >= 0, "mip index must be non-negative");

    const Int32 mipWidth  = Max(1, _width >> _mip);
    const Int32 mipHeight = Max(1, _height >> _mip);
    const Int32 mipDepth  = Max(1, _depth >> _mip);
    return { mipWidth, mipHeight, mipDepth };
}

Int32 CalcTextureIndex(
    const Int32 _mip,
    const Int32 _layer,
    const Int32 _numMips)
{
    return _mip + _layer * _numMips;
}

}   // namespace Jam