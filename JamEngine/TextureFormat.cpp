#include "pch.h"

#include "TextureFormat.h"

namespace Jam
{

TextureFormatInfo TextureFormatInfoFrom(
    const eTextureFormat _format)
{
    switch (_format)
    {
        case eTextureFormat::Unknown:
            return { DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::R8_UNorm:
            return { DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R8_SNorm:
            return { DXGI_FORMAT_R8_SNORM, DXGI_FORMAT_R8_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R8_UInt:
            return { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R8_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R8_Int:
            return { DXGI_FORMAT_R8_SINT, DXGI_FORMAT_R8_SINT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::R16_UNorm:
            return { DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R16_SNorm:
            return { DXGI_FORMAT_R16_SNORM, DXGI_FORMAT_R16_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R16_UInt:
            return { DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R16_Int:
            return { DXGI_FORMAT_R16_SINT, DXGI_FORMAT_R16_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R16_Float:
            return { DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::R32_UInt:
            return { DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R32_Int:
            return { DXGI_FORMAT_R32_SINT, DXGI_FORMAT_R32_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R32_Float:
            return { DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RG8_UNorm:
            return { DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG8_SNorm:
            return { DXGI_FORMAT_R8G8_SNORM, DXGI_FORMAT_R8G8_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG8_UInt:
            return { DXGI_FORMAT_R8G8_UINT, DXGI_FORMAT_R8G8_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG8_Int:
            return { DXGI_FORMAT_R8G8_SINT, DXGI_FORMAT_R8G8_SINT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RG16_UNorm:
            return { DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG16_SNorm:
            return { DXGI_FORMAT_R16G16_SNORM, DXGI_FORMAT_R16G16_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG16_UInt:
            return { DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG16_Int:
            return { DXGI_FORMAT_R16G16_SINT, DXGI_FORMAT_R16G16_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG16_Float:
            return { DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RG32_UInt:
            return { DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG32_Int:
            return { DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG32_Float:
            return { DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RGBA8_UNorm:
            return { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA8_SNorm:
            return { DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA8_UInt:
            return { DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA8_Int:
            return { DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA8_UNorm_SRGB:
            return { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::BGRA8_UNorm:
            return { DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::BGRA8_UNorm_SRGB:
            return { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RGBA16_UNorm:
            return { DXGI_FORMAT_R16G16B16A16_UNORM, DXGI_FORMAT_R16G16B16A16_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA16_SNorm:
            return { DXGI_FORMAT_R16G16B16A16_SNORM, DXGI_FORMAT_R16G16B16A16_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA16_UInt:
            return { DXGI_FORMAT_R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA16_Int:
            return { DXGI_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA16_Float:
            return { DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RGBA32_UInt:
            return { DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA32_Int:
            return { DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA32_Float:
            return { DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::B5G6R5_UNorm:
            return { DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::BGRA4_UNorm:
            return { DXGI_FORMAT_B4G4R4A4_UNORM, DXGI_FORMAT_B4G4R4A4_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::BGR5A1_UNorm:
            return { DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGB10A2_UNorm:
            return { DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG11B10_Float:
            return { DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::D16_UNorm:
            return { DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_D16_UNORM };
        case eTextureFormat::D24_UNorm_S8_UInt:
            return { DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT };
        case eTextureFormat::D32_Float:
            return { DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_D32_FLOAT };
        case eTextureFormat::D32_Float_S8_UInt:
            return { DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT };

        default:
            return { DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN };
    }
}

eTextureFormat TextureFormatFrom(
    const PlatformTextureFormat _format)
{
    switch (_format)
    {
        case DXGI_FORMAT_UNKNOWN:
            return eTextureFormat::Unknown;

        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            return eTextureFormat::RGBA32_Float;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return eTextureFormat::RGBA32_UInt;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return eTextureFormat::RGBA32_Int;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            return eTextureFormat::RGBA16_Float;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            return eTextureFormat::RGBA16_UNorm;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            return eTextureFormat::RGBA16_UInt;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            return eTextureFormat::RGBA16_SNorm;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return eTextureFormat::RGBA16_Int;

        case DXGI_FORMAT_R32G32_FLOAT:
            return eTextureFormat::RG32_Float;
        case DXGI_FORMAT_R32G32_UINT:
            return eTextureFormat::RG32_UInt;
        case DXGI_FORMAT_R32G32_SINT:
            return eTextureFormat::RG32_Int;

        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return eTextureFormat::D32_Float_S8_UInt;

        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return eTextureFormat::RGB10A2_UNorm;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            return eTextureFormat::RG11B10_Float;

        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return eTextureFormat::RGBA8_UNorm;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return eTextureFormat::RGBA8_UNorm_SRGB;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return eTextureFormat::RGBA8_UInt;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return eTextureFormat::RGBA8_SNorm;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return eTextureFormat::RGBA8_Int;

        case DXGI_FORMAT_R16G16_FLOAT:
            return eTextureFormat::RG16_Float;
        case DXGI_FORMAT_R16G16_UNORM:
            return eTextureFormat::RG16_UNorm;
        case DXGI_FORMAT_R16G16_UINT:
            return eTextureFormat::RG16_UInt;
        case DXGI_FORMAT_R16G16_SNORM:
            return eTextureFormat::RG16_SNorm;
        case DXGI_FORMAT_R16G16_SINT:
            return eTextureFormat::RG16_Int;

        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
            return eTextureFormat::R32_Float;
        case DXGI_FORMAT_R32_UINT:
            return eTextureFormat::R32_UInt;
        case DXGI_FORMAT_R32_SINT:
            return eTextureFormat::R32_Int;

        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            return eTextureFormat::D24_UNorm_S8_UInt;

        case DXGI_FORMAT_R8G8_UNORM:
            return eTextureFormat::RG8_UNorm;
        case DXGI_FORMAT_R8G8_UINT:
            return eTextureFormat::RG8_UInt;
        case DXGI_FORMAT_R8G8_SNORM:
            return eTextureFormat::RG8_SNorm;
        case DXGI_FORMAT_R8G8_SINT:
            return eTextureFormat::RG8_Int;

        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
            return eTextureFormat::R16_Float;
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
            return eTextureFormat::R16_UNorm;
        case DXGI_FORMAT_R16_UINT:
            return eTextureFormat::R16_UInt;
        case DXGI_FORMAT_R16_SNORM:
            return eTextureFormat::R16_SNorm;
        case DXGI_FORMAT_R16_SINT:
            return eTextureFormat::R16_Int;

        case DXGI_FORMAT_R8_UNORM:
            return eTextureFormat::R8_UNorm;
        case DXGI_FORMAT_R8_UINT:
            return eTextureFormat::R8_UInt;
        case DXGI_FORMAT_R8_SNORM:
            return eTextureFormat::R8_SNorm;
        case DXGI_FORMAT_R8_SINT:
            return eTextureFormat::R8_Int;

        case DXGI_FORMAT_B5G6R5_UNORM:
            return eTextureFormat::B5G6R5_UNorm;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            return eTextureFormat::BGR5A1_UNorm;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return eTextureFormat::BGRA8_UNorm;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return eTextureFormat::BGRA8_UNorm_SRGB;
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return eTextureFormat::BGRA4_UNorm;

        // Unsupported formats return Unknown
        default:
            return eTextureFormat::Unknown;
    }
}

}   // namespace Jam