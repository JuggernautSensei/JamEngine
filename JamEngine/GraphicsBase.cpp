#include "pch.h"

#include "GraphicsBase.h"

#include "Bit.h"

namespace Jam
{

namespace
{
    constexpr eTextureCreateFlags kTextureCreateFlagsMsaaMask = static_cast<eTextureCreateFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0000'1100'0000));   // [7:9]

    constexpr eRenderFlags kRenderFlagsPrimitiveMask  = static_cast<eRenderFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0000'0000'0111));   // [0:2]
    constexpr eRenderFlags kRenderFlagsCullModeMask   = static_cast<eRenderFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0000'0001'1000));   // [3:4]
    constexpr eRenderFlags kRenderFlagsRasterizerMask = static_cast<eRenderFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0111'1111'1000));   // [3:10] (CullMode 포함)
    constexpr eRenderFlags kRenderFlagsBlendMask      = static_cast<eRenderFlags>(UINT32_C(0b0000'0000'0000'0000'0001'1000'0000'0000));   // [11:12]
    constexpr eRenderFlags kRenderFlagsDepthMask      = static_cast<eRenderFlags>(UINT32_C(0b0000'0000'0000'0011'1110'0000'0000'0000));   // [13:17] (DepthWrite 포함)
    constexpr eRenderFlags kRenderFlagsDepthFuncMask  = static_cast<eRenderFlags>(UINT32_C(0b0000'0000'0000'0001'1110'0000'0000'0000));   // [13:16]
    constexpr UInt32       kRenderFlagsDepthFuncShift = UInt32CountRZero(UnderlyingFrom(kRenderFlagsDepthFuncMask));

    constexpr eBlendFlags kBlendFlagsSrcBlendMask       = static_cast<eBlendFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0000'0001'1110));   // [1:4]
    constexpr eBlendFlags kBlendFlagsDstBlendMask       = static_cast<eBlendFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0001'1110'0000));   // [5:8]
    constexpr eBlendFlags kBlendFlagsBlendOpMask        = static_cast<eBlendFlags>(UINT32_C(0b0000'0000'0000'0000'0000'1110'0000'0000));   // [9:11]
    constexpr eBlendFlags kBlendFlagsSrcBlendAlphaMask  = static_cast<eBlendFlags>(UINT32_C(0b0000'0000'0000'0000'1111'0000'0000'0000));   // [12:15]
    constexpr eBlendFlags kBlendFlagsDstBlendAlphaMask  = static_cast<eBlendFlags>(UINT32_C(0b0000'0000'0000'1111'0000'0000'0000'0000));   // [16:19]
    constexpr eBlendFlags kBlendFlagsBlendOpAlphaMask   = static_cast<eBlendFlags>(UINT32_C(0b0000'0000'0111'0000'0000'0000'0000'0000));   // [20:22]
    constexpr UInt32      kBlendFlagsSrcBlendShift      = UInt32CountRZero(UnderlyingFrom(kBlendFlagsSrcBlendMask));
    constexpr UInt32      kBlendFlagsDstBlendShift      = UInt32CountRZero(UnderlyingFrom(kBlendFlagsDstBlendMask));
    constexpr UInt32      kBlendFlagsBlendOpShift       = UInt32CountRZero(UnderlyingFrom(kBlendFlagsBlendOpMask));
    constexpr UInt32      kBlendFlagsSrcBlendAlphaShift = UInt32CountRZero(UnderlyingFrom(kBlendFlagsSrcBlendAlphaMask));
    constexpr UInt32      kBlendFlagsDstBlendAlphaShift = UInt32CountRZero(UnderlyingFrom(kBlendFlagsDstBlendAlphaMask));
    constexpr UInt32      kBlendFlagsBlendOpAlphaShift  = UInt32CountRZero(UnderlyingFrom(kBlendFlagsBlendOpAlphaMask));

    constexpr eStencilFlags kStencilFlagsStencilFailOpMask  = static_cast<eStencilFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0000'0000'0111));   // [0:2]
    constexpr eStencilFlags kStencilFlagsDepthFailOpMask    = static_cast<eStencilFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0000'0011'1000));   // [3:5]
    constexpr eStencilFlags kStencilFlagsStencilPassOpMask  = static_cast<eStencilFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0001'1100'0000));   // [6:8]
    constexpr eStencilFlags kStencilFlagsCompareFuncMask    = static_cast<eStencilFlags>(UINT32_C(0b0000'0000'0000'0000'0001'1110'0000'0000));   // [9:12]
    constexpr UInt32        kStencilFlagsDepthFailOpShift   = UInt32CountRZero(UnderlyingFrom(kStencilFlagsDepthFailOpMask));
    constexpr UInt32        kStencilFlagsStencilPassOpShift = UInt32CountRZero(UnderlyingFrom(kStencilFlagsStencilPassOpMask));
    constexpr UInt32        kStencilFlagsCompareFuncShift   = UInt32CountRZero(UnderlyingFrom(kStencilFlagsCompareFuncMask));

    constexpr eSamplerFlags kSampleFlagsFilterMask       = static_cast<eSamplerFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0000'0001'1111));   // [0:4]
    constexpr eSamplerFlags kSampleFlagsAddressUMask     = static_cast<eSamplerFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0000'1110'0000));   // [5:7]
    constexpr eSamplerFlags kSampleFlagsAddressVMask     = static_cast<eSamplerFlags>(UINT32_C(0b0000'0000'0000'0000'0000'0111'0000'0000));   // [8:10]
    constexpr eSamplerFlags kSampleFlagsAddressWMask     = static_cast<eSamplerFlags>(UINT32_C(0b0000'0000'0000'0000'0011'1000'0000'0000));   // [11:13]
    constexpr eSamplerFlags kSampleFlagsCompareFuncMask  = static_cast<eSamplerFlags>(UINT32_C(0b0000'0000'0000'0011'1100'0000'0000'0000));   // [14:17]
    constexpr UInt32        kSampleFlagsAddressUShift    = UInt32CountRZero(UnderlyingFrom(kSampleFlagsAddressUMask));
    constexpr UInt32        kSampleFlagsAddressVShift    = UInt32CountRZero(UnderlyingFrom(kSampleFlagsAddressVMask));
    constexpr UInt32        kSampleFlagsAddressWShift    = UInt32CountRZero(UnderlyingFrom(kSampleFlagsAddressWMask));
    constexpr UInt32        kSampleFlagsCompareFuncShift = UInt32CountRZero(UnderlyingFrom(kSampleFlagsCompareFuncMask));

}   // namespace

RGBA RGBA::kWhite       = RGBA(255, 255, 255, 255);
RGBA RGBA::kBlack       = RGBA(0, 0, 0, 255);
RGBA RGBA::kTransparent = RGBA(0, 0, 0, 0);

Flags<eTextureCreateFlags> FilterMsaaFlags(
    const Flags<eTextureCreateFlags> _flags) noexcept
{
    return _flags & kTextureCreateFlagsMsaaMask;
}

Flags<eRenderFlags> FilterPrimitiveFlags(
    const Flags<eRenderFlags> _flags)
{
    return _flags & kRenderFlagsPrimitiveMask;
}

Flags<eRenderFlags> FilterCullModeFlags(
    const Flags<eRenderFlags> _flags)
{
    return _flags & kRenderFlagsCullModeMask;
}

Flags<eRenderFlags> FilterRasterizerFlags(
    const Flags<eRenderFlags> _flags)
{
    return _flags & kRenderFlagsRasterizerMask;
}

Flags<eRenderFlags> FilterBlendFlags(
    const Flags<eRenderFlags> _flags)
{
    return _flags & kRenderFlagsBlendMask;
}

Flags<eRenderFlags> FilterDepthFuncFlags(
    const Flags<eRenderFlags> _flags)
{
    return _flags & kRenderFlagsDepthFuncMask;
}

Flags<eRenderFlags> FilterDepthFlags(
    const Flags<eRenderFlags> _flags)
{
    return _flags & kRenderFlagsDepthMask;
}

UInt32 ResolveDepthFuncFlags(
    const Flags<eRenderFlags> _flags)
{
    return FilterDepthFuncFlags(_flags).GetValue() >> kRenderFlagsDepthFuncShift;
}

Flags<eRenderFlags> SetPrimitiveFlags(
    const Flags<eRenderFlags> _flags,
    const Flags<eRenderFlags> _primitiveFlags)
{
    JAM_ASSERT(FilterPrimitiveFlags(_primitiveFlags) == _primitiveFlags, "Invalid primitive flags.");
    return (_flags & ~kRenderFlagsPrimitiveMask) | _primitiveFlags;
}

Flags<eRenderFlags> SetCullModeFlags(
    const Flags<eRenderFlags> _flags,
    const Flags<eRenderFlags> _cullModeFlags)
{
    JAM_ASSERT(FilterCullModeFlags(_cullModeFlags) == _cullModeFlags, "Invalid cull mode flags.");
    return (_flags & ~kRenderFlagsCullModeMask) | _cullModeFlags;
}

Flags<eRenderFlags> SetDepthFuncFlags(
    const Flags<eRenderFlags> _flags,
    const Flags<eRenderFlags> _depthFuncFlags)
{
    JAM_ASSERT(FilterDepthFuncFlags(_depthFuncFlags) == _depthFuncFlags, "Invalid depth func flags.");
    return (_flags & ~kRenderFlagsDepthFuncMask) | _depthFuncFlags;
}

UInt32 ResolvePrimitiveFlags(
    const Flags<eRenderFlags> _flags)
{
    return FilterPrimitiveFlags(_flags).GetValue();
}

Flags<eBlendFlags> FilterSrcBlendFlags(
    const Flags<eBlendFlags> _flags)
{
    return _flags & kBlendFlagsSrcBlendMask;
}

Flags<eBlendFlags> FilterDstBlendFlags(
    const Flags<eBlendFlags> _flags)
{
    return _flags & kBlendFlagsDstBlendMask;
}

Flags<eBlendFlags> FilterBlendOpFlags(
    const Flags<eBlendFlags> _flags)
{
    return _flags & kBlendFlagsBlendOpMask;
}

Flags<eBlendFlags> FilterSrcBlendAlphaFlags(
    const Flags<eBlendFlags> _flags)
{
    return _flags & kBlendFlagsSrcBlendAlphaMask;
}

Flags<eBlendFlags> FilterDstBlendAlphaFlags(
    const Flags<eBlendFlags> _flags)
{
    return _flags & kBlendFlagsDstBlendAlphaMask;
}

Flags<eBlendFlags> FilterBlendOpAlphaFlags(
    const Flags<eBlendFlags> _flags)
{
    return _flags & kBlendFlagsBlendOpAlphaMask;
}

Flags<eBlendFlags> SetSrcBlendFlags(
    const Flags<eBlendFlags> _flags,
    const Flags<eBlendFlags> _srcBlendFlags)
{
    JAM_ASSERT(FilterSrcBlendFlags(_srcBlendFlags) == _srcBlendFlags, "Invalid src blend flags.");
    return (_flags & ~kBlendFlagsSrcBlendMask) | _srcBlendFlags;
}

Flags<eBlendFlags> SetDstBlendFlags(
    const Flags<eBlendFlags> _flags,
    const Flags<eBlendFlags> _dstBlendFlags)
{
    JAM_ASSERT(FilterDstBlendFlags(_dstBlendFlags) == _dstBlendFlags, "Invalid dst blend flags.");
    return (_flags & ~kBlendFlagsDstBlendMask) | _dstBlendFlags;
}

Flags<eBlendFlags> SetBlendOpFlags(
    const Flags<eBlendFlags> _flags,
    const Flags<eBlendFlags> _blendOpFlags)
{
    JAM_ASSERT(FilterBlendOpFlags(_blendOpFlags) == _blendOpFlags, "Invalid blend op flags.");
    return (_flags & ~kBlendFlagsBlendOpMask) | _blendOpFlags;
}

Flags<eBlendFlags> SetSrcBlendAlphaFlags(
    const Flags<eBlendFlags> _flags,
    const Flags<eBlendFlags> _srcBlendAlphaFlags)
{
    JAM_ASSERT(FilterSrcBlendAlphaFlags(_srcBlendAlphaFlags) == _srcBlendAlphaFlags, "Invalid src blend alpha flags.");
    return (_flags & ~kBlendFlagsSrcBlendAlphaMask) | _srcBlendAlphaFlags;
}

Flags<eBlendFlags> SetDstBlendAlphaFlags(
    const Flags<eBlendFlags> _flags,
    const Flags<eBlendFlags> _dstBlendAlphaFlags)
{
    JAM_ASSERT(FilterDstBlendAlphaFlags(_dstBlendAlphaFlags) == _dstBlendAlphaFlags, "Invalid dst blend alpha flags.");
    return (_flags & ~kBlendFlagsDstBlendAlphaMask) | _dstBlendAlphaFlags;
}

Flags<eBlendFlags> SetBlendOpAlphaFlags(
    const Flags<eBlendFlags> _flags,
    const Flags<eBlendFlags> _blendOpAlphaFlags)
{
    JAM_ASSERT(FilterBlendOpAlphaFlags(_blendOpAlphaFlags) == _blendOpAlphaFlags, "Invalid blend op alpha flags.");
    return (_flags & ~kBlendFlagsBlendOpAlphaMask) | _blendOpAlphaFlags;
}

UInt32 ResolveSrcBlendFlags(
    const Flags<eBlendFlags> _flags)
{
    return FilterSrcBlendFlags(_flags).GetValue() >> kBlendFlagsSrcBlendShift;
}

UInt32 ResolveDstBlendFlags(
    const Flags<eBlendFlags> _flags)
{
    return FilterDstBlendFlags(_flags).GetValue() >> kBlendFlagsDstBlendShift;
}

UInt32 ResolveBlendOpFlags(
    const Flags<eBlendFlags> _flags)
{
    return FilterBlendOpFlags(_flags).GetValue() >> kBlendFlagsBlendOpShift;
}

UInt32 ResolveSrcBlendAlphaFlags(
    const Flags<eBlendFlags> _flags)
{
    return FilterSrcBlendAlphaFlags(_flags).GetValue() >> kBlendFlagsSrcBlendAlphaShift;
}

UInt32 ResolveDstBlendAlphaFlags(
    const Flags<eBlendFlags> _flags)
{
    return FilterDstBlendAlphaFlags(_flags).GetValue() >> kBlendFlagsDstBlendAlphaShift;
}

UInt32 ResolveBlendOpAlphaFlags(
    const Flags<eBlendFlags> _flags)
{
    return FilterBlendOpAlphaFlags(_flags).GetValue() >> kBlendFlagsBlendOpAlphaShift;
}

Flags<eStencilFlags> FilterStencilFailOpFlags(
    const Flags<eStencilFlags> _flags)
{
    return _flags & kStencilFlagsStencilFailOpMask;
}

Flags<eStencilFlags> FilterDepthFailOpFlags(
    const Flags<eStencilFlags> _flags)
{
    return _flags & kStencilFlagsDepthFailOpMask;
}

Flags<eStencilFlags> FilterPassOpFlags(
    const Flags<eStencilFlags> _flags)
{
    return _flags & kStencilFlagsStencilPassOpMask;
}

Flags<eStencilFlags> FilterCompareFuncFlags(
    const Flags<eStencilFlags> _flags)
{
    return _flags & kStencilFlagsCompareFuncMask;
}

Flags<eStencilFlags> SetStencilFailOpFlags(
    const Flags<eStencilFlags> _flags,
    const Flags<eStencilFlags> _stencilFailOpFlags)
{
    JAM_ASSERT(FilterStencilFailOpFlags(_stencilFailOpFlags) == _stencilFailOpFlags, "Invalid stencil fail op flags.");
    return (_flags & ~kStencilFlagsStencilFailOpMask) | _stencilFailOpFlags;
}

Flags<eStencilFlags> SetDepthFailOpFlags(
    const Flags<eStencilFlags> _flags,
    const Flags<eStencilFlags> _depthFailOpFlags)
{
    JAM_ASSERT(FilterDepthFailOpFlags(_depthFailOpFlags) == _depthFailOpFlags, "Invalid depth fail op flags.");
    return (_flags & ~kStencilFlagsDepthFailOpMask) | _depthFailOpFlags;
}

Flags<eStencilFlags> SetPassOpFlags(
    const Flags<eStencilFlags> _flags,
    const Flags<eStencilFlags> _passOpFlags)
{
    JAM_ASSERT(FilterPassOpFlags(_passOpFlags) == _passOpFlags, "Invalid pass op flags.");
    return (_flags & ~kStencilFlagsStencilPassOpMask) | _passOpFlags;
}

Flags<eStencilFlags> SetCompareFuncFlags(
    const Flags<eStencilFlags> _flags,
    const Flags<eStencilFlags> _compareFuncFlags)
{
    JAM_ASSERT(FilterCompareFuncFlags(_compareFuncFlags) == _compareFuncFlags, "Invalid compare func flags.");
    return (_flags & ~kStencilFlagsCompareFuncMask) | _compareFuncFlags;
}

UInt32 ResolveStencilFailOpFlags(
    const Flags<eStencilFlags> _flags)
{
    return FilterStencilFailOpFlags(_flags).GetValue();
}

UInt32 ResolveDepthFailOpFlags(
    const Flags<eStencilFlags> _flags)
{
    return FilterDepthFailOpFlags(_flags).GetValue() >> kStencilFlagsDepthFailOpShift;
}

UInt32 ResolvePassOpFlags(
    const Flags<eStencilFlags> _flags)
{
    return FilterPassOpFlags(_flags).GetValue() >> kStencilFlagsStencilPassOpShift;
}

UInt32 ResolveCompareFuncFlags(
    const Flags<eStencilFlags> _flags)
{
    return FilterCompareFuncFlags(_flags).GetValue() >> kStencilFlagsCompareFuncShift;
}

Flags<eSamplerFlags> FilterFilterFlags(
    const Flags<eSamplerFlags> _flags)
{
    return _flags & kSampleFlagsFilterMask;
}

Flags<eSamplerFlags> FilterAddressUFlags(
    const Flags<eSamplerFlags> _flags)
{
    return _flags & kSampleFlagsAddressUMask;
}

Flags<eSamplerFlags> FilterAddressVFlags(
    const Flags<eSamplerFlags> _flags)
{
    return _flags & kSampleFlagsAddressVMask;
}

Flags<eSamplerFlags> FilterAddressWFlags(
    const Flags<eSamplerFlags> _flags)
{
    return _flags & kSampleFlagsAddressWMask;
}

Flags<eSamplerFlags> FilterCompareFuncFlags(
    const Flags<eSamplerFlags> _flags)
{
    return _flags & kSampleFlagsCompareFuncMask;
}

Flags<eSamplerFlags> SetFilterFlags(
    const Flags<eSamplerFlags> _flags,
    const Flags<eSamplerFlags> _filterFlags)
{
    JAM_ASSERT(FilterFilterFlags(_filterFlags) == _filterFlags, "Invalid filter flags.");
    return (_flags & ~kSampleFlagsFilterMask) | _filterFlags;
}

Flags<eSamplerFlags> SetAddressUFlags(
    const Flags<eSamplerFlags> _flags,
    const Flags<eSamplerFlags> _addressUFlags)
{
    JAM_ASSERT(FilterAddressUFlags(_addressUFlags) == _addressUFlags, "Invalid address U flags.");
    return (_flags & ~kSampleFlagsAddressUMask) | _addressUFlags;
}

Flags<eSamplerFlags> SetAddressVFlags(
    const Flags<eSamplerFlags> _flags,
    const Flags<eSamplerFlags> _addressVFlags)
{
    JAM_ASSERT(FilterAddressVFlags(_addressVFlags) == _addressVFlags, "Invalid address V flags.");
    return (_flags & ~kSampleFlagsAddressVMask) | _addressVFlags;
}

Flags<eSamplerFlags> SetAddressWFlags(
    const Flags<eSamplerFlags> _flags,
    const Flags<eSamplerFlags> _addressWFlags)
{
    JAM_ASSERT(FilterAddressWFlags(_addressWFlags) == _addressWFlags, "Invalid address W flags.");
    return (_flags & ~kSampleFlagsAddressWMask) | _addressWFlags;
}

Flags<eSamplerFlags> SetCompareFuncFlags(
    const Flags<eSamplerFlags> _flags,
    const Flags<eSamplerFlags> _compareFuncFlags)
{
    JAM_ASSERT(FilterCompareFuncFlags(_compareFuncFlags) == _compareFuncFlags, "Invalid sampler compare func flags.");
    return (_flags & ~kSampleFlagsCompareFuncMask) | _compareFuncFlags;
}

UInt32 ResolveFilterFlags(
    const Flags<eSamplerFlags> _flags)
{
    return FilterFilterFlags(_flags).GetValue();
}

UInt32 ResolveAddressUFlags(
    const Flags<eSamplerFlags> _flags)
{
    return FilterAddressUFlags(_flags).GetValue() >> kSampleFlagsAddressUShift;
}

UInt32 ResolveAddressVFlags(
    const Flags<eSamplerFlags> _flags)
{
    return FilterAddressVFlags(_flags).GetValue() >> kSampleFlagsAddressVShift;
}

UInt32 ResolveAddressWFlags(
    const Flags<eSamplerFlags> _flags)
{
    return FilterAddressWFlags(_flags).GetValue() >> kSampleFlagsAddressWShift;
}

UInt32 ResolveCompareFuncFlags(
    const Flags<eSamplerFlags> _flags)
{
    return FilterCompareFuncFlags(_flags).GetValue() >> kSampleFlagsCompareFuncShift;
}

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