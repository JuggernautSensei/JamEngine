#pragma once

namespace jam
{

enum eTextureFormat : Int8
{
    Unknown = 0,

    R8_UNorm,
    R8_SNorm,
    R8_UInt,
    R8_Int,

    R16_UNorm,
    R16_SNorm,
    R16_UInt,
    R16_Int,
    R16_Float,

    R32_UInt,
    R32_Int,
    R32_Float,

    RG8_UNorm,
    RG8_SNorm,
    RG8_UInt,
    RG8_Int,

    RG16_UNorm,
    RG16_SNorm,
    RG16_UInt,
    RG16_Int,
    RG16_Float,

    RG32_UInt,
    RG32_Int,
    RG32_Float,

    RGBA8_UNorm,
    RGBA8_SNorm,
    RGBA8_UInt,
    RGBA8_Int,
    RGBA8_UNorm_SRGB,

    BGRA8_UNorm,
    BGRA8_UNorm_SRGB,

    RGBA16_UNorm,
    RGBA16_SNorm,
    RGBA16_UInt,
    RGBA16_Int,
    RGBA16_Float,

    RGBA32_UInt,
    RGBA32_Int,
    RGBA32_Float,

    B5G6R5_UNorm,
    BGRA4_UNorm,
    BGR5A1_UNorm,
    RGB10A2_UNorm,
    RG11B10_Float,

    D16_UNorm,
    D24_UNorm_S8_UInt,
    D32_Float,
    D32_Float_S8_UInt,
};

using PlatformTextureFormat = DXGI_FORMAT;   // todo: change this alias per platform
struct TextureFormatInfo
{
    PlatformTextureFormat texFormat   = static_cast<PlatformTextureFormat>(0);
    PlatformTextureFormat colorFormat = static_cast<PlatformTextureFormat>(0);
    PlatformTextureFormat depthFormat = static_cast<PlatformTextureFormat>(0);
};

[[nodiscard]] TextureFormatInfo GetTextureFormatInfo(eTextureFormat _format);
[[nodiscard]] eTextureFormat    GetTextureFormatFromPlatform(PlatformTextureFormat _format);

}   // namespace jam