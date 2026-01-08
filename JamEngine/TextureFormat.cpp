#include "pch.h"

#include "TextureFormat.h"

namespace jam
{

TextureFormatInfo TextureFormatInfoFrom(
    const eTextureFormat _format)
{
    switch (_format)
    {
        case eTextureFormat::Unknown:
            return { "Unknown", DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::R8_UNorm:
            return { "R8_UNorm", DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R8_SNorm:
            return { "R8_SNorm", DXGI_FORMAT_R8_SNORM, DXGI_FORMAT_R8_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R8_UInt:
            return { "R8_UInt", DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R8_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R8_Int:
            return { "R8_Int", DXGI_FORMAT_R8_SINT, DXGI_FORMAT_R8_SINT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::R16_UNorm:
            return { "R16_UNorm", DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R16_SNorm:
            return { "R16_SNorm", DXGI_FORMAT_R16_SNORM, DXGI_FORMAT_R16_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R16_UInt:
            return { "R16_UInt", DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R16_Int:
            return { "R16_Int", DXGI_FORMAT_R16_SINT, DXGI_FORMAT_R16_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R16_Float:
            return { "R16_Float", DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::R32_UInt:
            return { "R32_UInt", DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R32_Int:
            return { "R32_Int", DXGI_FORMAT_R32_SINT, DXGI_FORMAT_R32_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::R32_Float:
            return { "R32_Float", DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RG8_UNorm:
            return { "RG8_UNorm", DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG8_SNorm:
            return { "RG8_SNorm", DXGI_FORMAT_R8G8_SNORM, DXGI_FORMAT_R8G8_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG8_UInt:
            return { "RG8_UInt", DXGI_FORMAT_R8G8_UINT, DXGI_FORMAT_R8G8_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG8_Int:
            return { "RG8_Int", DXGI_FORMAT_R8G8_SINT, DXGI_FORMAT_R8G8_SINT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RG16_UNorm:
            return { "RG16_UNorm", DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG16_SNorm:
            return { "RG16_SNorm", DXGI_FORMAT_R16G16_SNORM, DXGI_FORMAT_R16G16_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG16_UInt:
            return { "RG16_UInt", DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG16_Int:
            return { "RG16_Int", DXGI_FORMAT_R16G16_SINT, DXGI_FORMAT_R16G16_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG16_Float:
            return { "RG16_Float", DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RG32_UInt:
            return { "RG32_UInt", DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG32_Int:
            return { "RG32_Int", DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG32_Float:
            return { "RG32_Float", DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RGBA8_UNorm:
            return { "RGBA8_UNorm", DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA8_SNorm:
            return { "RGBA8_SNorm", DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA8_UInt:
            return { "RGBA8_UInt", DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA8_Int:
            return { "RGBA8_Int", DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA8_UNorm_SRGB:
            return { "RGBA8_UNorm_SRGB", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::BGRA8_UNorm:
            return { "BGRA8_UNorm", DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::BGRA8_UNorm_SRGB:
            return { "BGRA8_UNorm_SRGB", DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RGBA16_UNorm:
            return { "RGBA16_UNorm", DXGI_FORMAT_R16G16B16A16_UNORM, DXGI_FORMAT_R16G16B16A16_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA16_SNorm:
            return { "RGBA16_SNorm", DXGI_FORMAT_R16G16B16A16_SNORM, DXGI_FORMAT_R16G16B16A16_SNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA16_UInt:
            return { "RGBA16_UInt", DXGI_FORMAT_R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA16_Int:
            return { "RGBA16_Int", DXGI_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA16_Float:
            return { "RGBA16_Float", DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::RGBA32_UInt:
            return { "RGBA32_UInt", DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA32_Int:
            return { "RGBA32_Int", DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGBA32_Float:
            return { "RGBA32_Float", DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::B5G6R5_UNorm:
            return { "B5G6R5_UNorm", DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::BGRA4_UNorm:
            return { "BGRA4_UNorm", DXGI_FORMAT_B4G4R4A4_UNORM, DXGI_FORMAT_B4G4R4A4_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::BGR5A1_UNorm:
            return { "BGR5A1_UNorm", DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RGB10A2_UNorm:
            return { "RGB10A2_UNorm", DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_UNKNOWN };
        case eTextureFormat::RG11B10_Float:
            return { "RG11B10_Float", DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_UNKNOWN };

        case eTextureFormat::D16_UNorm:
            return { "D16_UNorm", DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_D16_UNORM };
        case eTextureFormat::D24_UNorm_S8_UInt:
            return { "D24_UNorm_S8_UInt", DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT };
        case eTextureFormat::D32_Float:
            return { "D32_Float", DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_D32_FLOAT };
        case eTextureFormat::D32_Float_S8_UInt:
            return { "D32_Float_S8_UInt", DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT };

        default:
            return { "Unknown", DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN };
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

}   // namespace jam