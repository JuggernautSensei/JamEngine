#pragma once
#include <DirectXTex.h>

#include "HandleAllocator.h"
#include "Math.h"
#include "TextureFormat.h"
#include "VertexLayout.h"

namespace Jam
{

/////////// CONSTANTS ////////////////////////////////////////////////////////////////////////////////////////////////

constexpr Int32 k_maxNumRenderTargetViewSlots    = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
constexpr Int32 k_maxNumAttachmentSlots          = k_maxNumRenderTargetViewSlots + 1;   // render target + depth stencil
constexpr Int32 k_maxNumSamplerSlots             = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
constexpr Int32 k_maxNumShaderResourceViewSlots  = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
constexpr Int32 k_maxNumUnorderedAccessViewSlots = D3D11_1_UAV_SLOT_COUNT;
constexpr Int32 k_maxNumConstantBufferSlots      = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
constexpr Int32 k_maxNumVSInputSlots             = D3D11_VS_INPUT_REGISTER_COUNT;
constexpr Int32 k_maxNumVertexStream             = 4;
constexpr Int32 k_maxNumVertexAttribs            = NarrowCast<Int32>(CountOf<eVertexAttribute>());
constexpr Int32 k_maxNumInstanceAttribs          = k_maxNumVSInputSlots - k_maxNumVertexAttribs;

////////// ENUMS ////////////////////////////////////////////////////////////////////////////////////////////////////

enum class eTextureCreateFlags : UInt32
{
    None = 0,

    RenderTargetAttachment = 1 << 0,   // usable as render target attachment
    DepthStencilAttachment = 1 << 1,   // usable as depth stencil attachment

    // access
    Mutable           = 1 << 2,   // force mutable                  (if init data is provided, texture is immutable. this flag overrides it)
    Readback          = 1 << 3,   // staging readback
    DisableShaderRead = 1 << 4,   // disable shader read            (no SRV created)
    ShaderReadWrite   = 1 << 5,   // shader read & write enabled    (create UAV)

    MSAAx4  = 1 << 6,   // 4x MSAA
    MSAAx8  = 2 << 6,   // 8x MSAA
    MSAAx16 = 3 << 6,   // 16x MSAA
};

[[nodiscard]] Flags<eTextureCreateFlags> FilterMsaaFlags(Flags<eTextureCreateFlags> _flags) noexcept;

enum class eStorageBufferCreateFlags : UInt32
{
    None = 0,

    Dynamic         = 1 << 0,   // dynamic buffer                      (CPU write access)
    ShaderRead      = 1 << 1,   // shader read enabled                 (SRV)
    ShaderReadWrite = 1 << 2,   // shader read & write enabled         (UAV)
    IndirectArgs    = 1 << 3,   // usable as indirect argument buffer
};

enum class eStorageBufferData
{
    UInt8x1,
    UInt8x2,
    UInt8x4,
    Int8x1,
    Int8x2,
    Int8x4,
    UInt16x1,
    UInt16x2,
    UInt16x4,
    Int16x1,
    Int16x2,
    Int16x4,
    Float16x1,
    Float16x2,
    Float16x4,
    UInt32x1,
    UInt32x2,
    UInt32x4,
    Int32x1,
    Int32x2,
    Int32x4,
    Float32x1,
    Float32x2,
    Float32x4,
};

enum class eShader
{
    VertexShader = 0,
    PixelShader,
    GeometryShader,
    HullShader,
    DomainShader,
    ComputeShader,
};

enum class eTexture
{
    Texture2D = 0,
    Texture3D,
    TextureCube,
};

enum class eBuffer
{
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,
    StorageBuffer,
};

enum class eStorageBuffer
{
    Raw = 0,
    Structured,
    Typed,
    Readback
};

enum class eCubeFace : Int8
{
    Right = 0,   // positive X
    Left,        // negative X
    Top,         // positive Y
    Bottom,      // negative Y
    Front,       // positive Z
    Back,        // negative Z
};

enum class eShaderCompileFlags : UInt32
{
    None                  = 0,
    DebugInfo             = 1 << 0,
    Optimization          = 1 << 1,
    SkipOptimization      = 1 << 2,
    WarningsAsErrors      = 1 << 3,
    AvoidFlowControl      = 1 << 4,
    PackMatrixRowMajor    = 1 << 5,
    PackMatrixColumnMajor = 1 << 6,

    DebugDefault   = DebugInfo | SkipOptimization | WarningsAsErrors,
    ReleaseDefault = Optimization,

#ifdef JAM_DEBUG
    Default = DebugDefault,
#else
    Default = ReleaseDefault,
#endif
};

enum class eSubmitParameter
{
    StartVertexLocation = 0,
    StartIndexLocation,
    StartInstanceLocation,
    BaseVertexLocation,
};

enum class eStreamMaskFlags : UInt32
{
    Stream0,
    Stream1,
    Stream2,
    Stream3,
};

enum class eShaderResource
{
    Texture = 0,
    Buffer,
};

enum class eResourceAccess
{
    Read = 0,
    ReadWrite,
};

////////// STATES ////////////////////////////////////////////////////////////////////////////////////////////////////

enum class eRenderFlags : UInt32
{
    None = 0,

    // Primitive [0:2] : 3 bit
    Primitive_TriangleList  = 0 << 0,
    Primitive_TriangleStrip = 1 << 0,
    Primitive_LineList      = 2 << 0,
    Primitive_LineStrip     = 3 << 0,
    Primitive_PointList     = 4 << 0,

    // CullMode [3:4] : 2 bit
    CullMode_None  = 0 << 3,
    CullMode_Back  = 1 << 3,
    CullMode_Front = 2 << 3,

    // Rasterizer Flags [5:10] : 6 bit
    Wireframe   = 1 << 5,
    DepthClamp  = 1 << 6,
    MultiSample = 1 << 7,
    LineAA      = 1 << 8,
    Scissor     = 1 << 9,
    CCWFront    = 1 << 10,

    // Blend Flags [11:12] : 2 bit
    AlphaToCoverage  = 1 << 11,
    IndependentBlend = 1 << 12,

    // DepthFunc [13:16] : 4 bit
    DepthFunc_None         = 0 << 13,
    DepthFunc_Less         = 1 << 13,
    DepthFunc_LessEqual    = 2 << 13,
    DepthFunc_Greater      = 3 << 13,
    DepthFunc_GreaterEqual = 4 << 13,
    DepthFunc_Equal        = 5 << 13,
    DepthFunc_NotEqual     = 6 << 13,
    DepthFunc_Always       = 7 << 13,
    DepthFunc_Never        = 8 << 13,

    // DepthWrite [17] : 1 bit
    DepthWrite = 1 << 17,
};

// Filter helper
[[nodiscard]] Flags<eRenderFlags> FilterPrimitiveFlags(Flags<eRenderFlags> _flags);
[[nodiscard]] Flags<eRenderFlags> FilterCullModeFlags(Flags<eRenderFlags> _flags);
[[nodiscard]] Flags<eRenderFlags> FilterRasterizerFlags(Flags<eRenderFlags> _flags);
[[nodiscard]] Flags<eRenderFlags> FilterBlendFlags(Flags<eRenderFlags> _flags);
[[nodiscard]] Flags<eRenderFlags> FilterDepthFuncFlags(Flags<eRenderFlags> _flags);
[[nodiscard]] Flags<eRenderFlags> FilterDepthFlags(Flags<eRenderFlags> _flags);

// set helper (not bit flags)
[[nodiscard]] Flags<eRenderFlags> SetPrimitiveFlags(Flags<eRenderFlags> _flags, Flags<eRenderFlags> _primitiveFlags);
[[nodiscard]] Flags<eRenderFlags> SetCullModeFlags(Flags<eRenderFlags> _flags, Flags<eRenderFlags> _cullModeFlags);
[[nodiscard]] Flags<eRenderFlags> SetDepthFuncFlags(Flags<eRenderFlags> _flags, Flags<eRenderFlags> _depthFuncFlags);

// resolve helper
[[nodiscard]] UInt32 ResolvePrimitiveFlags(Flags<eRenderFlags> _flags);
[[nodiscard]] UInt32 ResolveDepthFuncFlags(Flags<eRenderFlags> _flags);

enum class eBlendFlags : UInt32
{
    None = 0,

    // Enable [0] : 1 bit
    Enable = 1 << 0,

    // SrcBlend [1:4] : 4 bit
    SrcBlend_Zero           = 0 << 1,
    SrcBlend_One            = 1 << 1,
    SrcBlend_SrcColor       = 2 << 1,
    SrcBlend_InvSrcColor    = 3 << 1,
    SrcBlend_SrcAlpha       = 4 << 1,
    SrcBlend_InvSrcAlpha    = 5 << 1,
    SrcBlend_DstAlpha       = 6 << 1,
    SrcBlend_InvDstAlpha    = 7 << 1,
    SrcBlend_DstColor       = 8 << 1,
    SrcBlend_InvDstColor    = 9 << 1,
    SrcBlend_SrcAlphaSat    = 10 << 1,
    SrcBlend_BlendFactor    = 11 << 1,
    SrcBlend_InvBlendFactor = 12 << 1,

    // DstBlend [5:8] : 4 bit
    DstBlend_Zero           = 0 << 5,
    DstBlend_One            = 1 << 5,
    DstBlend_SrcColor       = 2 << 5,
    DstBlend_InvSrcColor    = 3 << 5,
    DstBlend_SrcAlpha       = 4 << 5,
    DstBlend_InvSrcAlpha    = 5 << 5,
    DstBlend_DstAlpha       = 6 << 5,
    DstBlend_InvDstAlpha    = 7 << 5,
    DstBlend_DstColor       = 8 << 5,
    DstBlend_InvDstColor    = 9 << 5,
    DstBlend_SrcAlphaSat    = 10 << 5,
    DstBlend_BlendFactor    = 11 << 5,
    DstBlend_InvBlendFactor = 12 << 5,

    // BlendOp [9:11] : 3 bit
    BlendOp_Add         = 0 << 9,
    BlendOp_Subtract    = 1 << 9,
    BlendOp_RevSubtract = 2 << 9,
    BlendOp_Min         = 3 << 9,
    BlendOp_Max         = 4 << 9,

    // SrcBlendAlpha [12:15] : 4 bit
    SrcBlendAlpha_Zero           = 0 << 12,
    SrcBlendAlpha_One            = 1 << 12,
    SrcBlendAlpha_SrcAlpha       = 4 << 12,
    SrcBlendAlpha_InvSrcAlpha    = 5 << 12,
    SrcBlendAlpha_DstAlpha       = 6 << 12,
    SrcBlendAlpha_InvDstAlpha    = 7 << 12,
    SrcBlendAlpha_BlendFactor    = 11 << 12,
    SrcBlendAlpha_InvBlendFactor = 12 << 12,

    // DstBlendAlpha [16:19] : 4 bit
    DstBlendAlpha_Zero           = 0 << 16,
    DstBlendAlpha_One            = 1 << 16,
    DstBlendAlpha_SrcAlpha       = 4 << 16,
    DstBlendAlpha_InvSrcAlpha    = 5 << 16,
    DstBlendAlpha_DstAlpha       = 6 << 16,
    DstBlendAlpha_InvDstAlpha    = 7 << 16,
    DstBlendAlpha_BlendFactor    = 11 << 16,
    DstBlendAlpha_InvBlendFactor = 12 << 16,

    // BlendOpAlpha [20:22] : 3 bit
    BlendOpAlpha_Add         = 0 << 20,
    BlendOpAlpha_Subtract    = 1 << 20,
    BlendOpAlpha_RevSubtract = 2 << 20,
    BlendOpAlpha_Min         = 3 << 20,
    BlendOpAlpha_Max         = 4 << 20,

    // WriteMask [23:26] : 4 bit
    WriteMask_R   = 1 << 23,
    WriteMask_G   = 1 << 24,
    WriteMask_B   = 1 << 25,
    WriteMask_A   = 1 << 26,
    WriteMask_All = WriteMask_R | WriteMask_G | WriteMask_B | WriteMask_A,
};

// Filter helper
[[nodiscard]] Flags<eBlendFlags> FilterSrcBlendFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] Flags<eBlendFlags> FilterDstBlendFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] Flags<eBlendFlags> FilterBlendOpFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] Flags<eBlendFlags> FilterSrcBlendAlphaFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] Flags<eBlendFlags> FilterDstBlendAlphaFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] Flags<eBlendFlags> FilterBlendOpAlphaFlags(Flags<eBlendFlags> _flags);

// set helper (not bit flags)
[[nodiscard]] Flags<eBlendFlags> SetSrcBlendFlags(Flags<eBlendFlags> _flags, Flags<eBlendFlags> _srcBlendFlags);
[[nodiscard]] Flags<eBlendFlags> SetDstBlendFlags(Flags<eBlendFlags> _flags, Flags<eBlendFlags> _dstBlendFlags);
[[nodiscard]] Flags<eBlendFlags> SetBlendOpFlags(Flags<eBlendFlags> _flags, Flags<eBlendFlags> _blendOpFlags);
[[nodiscard]] Flags<eBlendFlags> SetSrcBlendAlphaFlags(Flags<eBlendFlags> _flags, Flags<eBlendFlags> _srcBlendAlphaFlags);
[[nodiscard]] Flags<eBlendFlags> SetDstBlendAlphaFlags(Flags<eBlendFlags> _flags, Flags<eBlendFlags> _dstBlendAlphaFlags);
[[nodiscard]] Flags<eBlendFlags> SetBlendOpAlphaFlags(Flags<eBlendFlags> _flags, Flags<eBlendFlags> _blendOpAlphaFlags);

// resolve helper
[[nodiscard]] UInt32 ResolveSrcBlendFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] UInt32 ResolveDstBlendFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] UInt32 ResolveBlendOpFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] UInt32 ResolveSrcBlendAlphaFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] UInt32 ResolveDstBlendAlphaFlags(Flags<eBlendFlags> _flags);
[[nodiscard]] UInt32 ResolveBlendOpAlphaFlags(Flags<eBlendFlags> _flags);

enum class eStencilFlags : UInt16
{
    None = 0,

    // StencilFailOp [0:2] : 3 bit
    StencilFailOp_Keep    = 0 << 0,
    StencilFailOp_Zero    = 1 << 0,
    StencilFailOp_Replace = 2 << 0,
    StencilFailOp_IncrSat = 3 << 0,
    StencilFailOp_DecrSat = 4 << 0,
    StencilFailOp_Invert  = 5 << 0,
    StencilFailOp_Incr    = 6 << 0,
    StencilFailOp_Decr    = 7 << 0,

    // DepthFailOp [3:5] : 3 bit
    DepthFailOp_Keep    = 0 << 3,
    DepthFailOp_Zero    = 1 << 3,
    DepthFailOp_Replace = 2 << 3,
    DepthFailOp_IncrSat = 3 << 3,
    DepthFailOp_DecrSat = 4 << 3,
    DepthFailOp_Invert  = 5 << 3,
    DepthFailOp_Incr    = 6 << 3,
    DepthFailOp_Decr    = 7 << 3,

    // PassOp [6:8] : 3 bit
    PassOp_Keep    = 0 << 6,
    PassOp_Zero    = 1 << 6,
    PassOp_Replace = 2 << 6,
    PassOp_IncrSat = 3 << 6,
    PassOp_DecrSat = 4 << 6,
    PassOp_Invert  = 5 << 6,
    PassOp_Incr    = 6 << 6,
    PassOp_Decr    = 7 << 6,

    // CompareFunc [9:12] : 4 bit
    CompareFunc_None         = 0 << 9,
    CompareFunc_Less         = 1 << 9,
    CompareFunc_LessEqual    = 2 << 9,
    CompareFunc_Greater      = 3 << 9,
    CompareFunc_GreaterEqual = 4 << 9,
    CompareFunc_Equal        = 5 << 9,
    CompareFunc_NotEqual     = 6 << 9,
    CompareFunc_Always       = 7 << 9,
    CompareFunc_Never        = 8 << 9,
};

// Filter helper
[[nodiscard]] Flags<eStencilFlags> FilterStencilFailOpFlags(Flags<eStencilFlags> _flags);
[[nodiscard]] Flags<eStencilFlags> FilterDepthFailOpFlags(Flags<eStencilFlags> _flags);
[[nodiscard]] Flags<eStencilFlags> FilterPassOpFlags(Flags<eStencilFlags> _flags);
[[nodiscard]] Flags<eStencilFlags> FilterCompareFuncFlags(Flags<eStencilFlags> _flags);

// set helper (not bit flags)
[[nodiscard]] Flags<eStencilFlags> SetStencilFailOpFlags(Flags<eStencilFlags> _flags, Flags<eStencilFlags> _stencilFailOpFlags);
[[nodiscard]] Flags<eStencilFlags> SetDepthFailOpFlags(Flags<eStencilFlags> _flags, Flags<eStencilFlags> _depthFailOpFlags);
[[nodiscard]] Flags<eStencilFlags> SetPassOpFlags(Flags<eStencilFlags> _flags, Flags<eStencilFlags> _passOpFlags);
[[nodiscard]] Flags<eStencilFlags> SetCompareFuncFlags(Flags<eStencilFlags> _flags, Flags<eStencilFlags> _compareFuncFlags);

// resolve helper
[[nodiscard]] UInt32 ResolveStencilFailOpFlags(Flags<eStencilFlags> _flags);
[[nodiscard]] UInt32 ResolveDepthFailOpFlags(Flags<eStencilFlags> _flags);
[[nodiscard]] UInt32 ResolvePassOpFlags(Flags<eStencilFlags> _flags);
[[nodiscard]] UInt32 ResolveCompareFuncFlags(Flags<eStencilFlags> _flags);

enum class eSamplerFlags : UInt32
{
    None = 0,

    // Filter [0:4] : 5 bit
    Filter_MinPoint_MagPoint_MipPoint    = 0 << 0,
    Filter_MinPoint_MagPoint_MipLinear   = 1 << 0,
    Filter_MinPoint_MagLinear_MipPoint   = 2 << 0,
    Filter_MinPoint_MagLinear_MipLinear  = 3 << 0,
    Filter_MinLinear_MagPoint_MipPoint   = 4 << 0,
    Filter_MinLinear_MagPoint_MipLinear  = 5 << 0,
    Filter_MinLinear_MagLinear_MipPoint  = 6 << 0,
    Filter_MinLinear_MagLinear_MipLinear = 7 << 0,
    Filter_Anisotropic4                  = 8 << 0,
    Filter_Anisotropic8                  = 9 << 0,
    Filter_Anisotropic16                 = 10 << 0,

    // AddressU [5:7] : 3 bit
    AddressU_Wrap       = 0 << 5,
    AddressU_Mirror     = 1 << 5,
    AddressU_Clamp      = 2 << 5,
    AddressU_Border     = 3 << 5,
    AddressU_MirrorOnce = 4 << 5,

    // AddressV [8:10] : 3 bit
    AddressV_Wrap       = 0 << 8,
    AddressV_Mirror     = 1 << 8,
    AddressV_Clamp      = 2 << 8,
    AddressV_Border     = 3 << 8,
    AddressV_MirrorOnce = 4 << 8,

    // AddressW [11:13] : 3 bit
    AddressW_Wrap       = 0 << 11,
    AddressW_Mirror     = 1 << 11,
    AddressW_Clamp      = 2 << 11,
    AddressW_Border     = 3 << 11,
    AddressW_MirrorOnce = 4 << 11,

    // CompareFunc [14:17] : 4 bit
    CompareFunc_None         = 0 << 14,
    CompareFunc_Less         = 1 << 14,
    CompareFunc_LessEqual    = 2 << 14,
    CompareFunc_Greater      = 3 << 14,
    CompareFunc_GreaterEqual = 4 << 14,
    CompareFunc_Equal        = 5 << 14,
    CompareFunc_NotEqual     = 6 << 14,
    CompareFunc_Always       = 7 << 14,
    CompareFunc_Never        = 8 << 14,
};

// Filter helper
[[nodiscard]] Flags<eSamplerFlags> FilterFilterFlags(Flags<eSamplerFlags> _flags);
[[nodiscard]] Flags<eSamplerFlags> FilterAddressUFlags(Flags<eSamplerFlags> _flags);
[[nodiscard]] Flags<eSamplerFlags> FilterAddressVFlags(Flags<eSamplerFlags> _flags);
[[nodiscard]] Flags<eSamplerFlags> FilterAddressWFlags(Flags<eSamplerFlags> _flags);
[[nodiscard]] Flags<eSamplerFlags> FilterCompareFuncFlags(Flags<eSamplerFlags> _flags);

// set helper (not bit flags)
[[nodiscard]] Flags<eSamplerFlags> SetFilterFlags(Flags<eSamplerFlags> _flags, Flags<eSamplerFlags> _filterFlags);
[[nodiscard]] Flags<eSamplerFlags> SetAddressUFlags(Flags<eSamplerFlags> _flags, Flags<eSamplerFlags> _addressUFlags);
[[nodiscard]] Flags<eSamplerFlags> SetAddressVFlags(Flags<eSamplerFlags> _flags, Flags<eSamplerFlags> _addressVFlags);
[[nodiscard]] Flags<eSamplerFlags> SetAddressWFlags(Flags<eSamplerFlags> _flags, Flags<eSamplerFlags> _addressWFlags);
[[nodiscard]] Flags<eSamplerFlags> SetCompareFuncFlags(Flags<eSamplerFlags> _flags, Flags<eSamplerFlags> _compareFuncFlags);

// resolve helper
[[nodiscard]] UInt32 ResolveFilterFlags(Flags<eSamplerFlags> _flags);
[[nodiscard]] UInt32 ResolveAddressUFlags(Flags<eSamplerFlags> _flags);
[[nodiscard]] UInt32 ResolveAddressVFlags(Flags<eSamplerFlags> _flags);
[[nodiscard]] UInt32 ResolveAddressWFlags(Flags<eSamplerFlags> _flags);
[[nodiscard]] UInt32 ResolveCompareFuncFlags(Flags<eSamplerFlags> _flags);

////////// RESOURCE //////////////////////////////////////////////////////////////////////////////////////////////////

struct VertexLayoutRef
{
    VertexLayout vl   = {};
    UInt64       hash = 0;
};
using VertexLayoutHandle = Handle<VertexLayoutRef>;

struct HardwareBuffer
{
    ID3D11Buffer* pBuffer = nullptr;

    String  name       = {};
    eBuffer type       = eBuffer::VertexBuffer;
    Int32   byteWidth  = 0;
    bool    bImmutable = false;
    bool    bDynamic   = false;
};

struct VertexBuffer : HardwareBuffer
{
    Int32              stride = 0;
    VertexLayoutHandle vlh    = k_nullHandle;
};
using VertexBufferHandle = Handle<VertexBuffer>;

struct IndexBuffer : HardwareBuffer
{
    bool bUseU32 = false;
};
using IndexBufferHandle = Handle<IndexBuffer>;

struct ConstantBuffer : HardwareBuffer
{
};
using ConstantBufferHandle = Handle<ConstantBuffer>;

struct StorageBuffer : HardwareBuffer
{
    ID3D11ShaderResourceView*  pSrv = nullptr;
    ID3D11UnorderedAccessView* pUav = nullptr;

    eStorageBuffer storageType    = eStorageBuffer::Raw;
    Int32          stride         = 0;
    Int32          numElems       = 0;
    bool           bReadback      = false;
    bool           bIndirectArgs  = false;
    bool           bAppendConsume = false;   // option for structured buffer
    bool           bCounter       = false;   // option for structured buffer
};
using StorageBufferHandle = Handle<StorageBuffer>;

struct HardwareTextureRef
{
    union
    {
        ID3D11Resource*  pResource = nullptr;
        ID3D11Texture2D* pTexture2D;
        ID3D11Texture3D* pTexture3D;
    };

    ID3D11ShaderResourceView*  pSrv = nullptr;
    ID3D11UnorderedAccessView* pUav = nullptr;

    String         name           = {};
    Int32          width          = 0;
    Int32          height         = 0;
    Int32          depth          = 0;
    eTexture       type           = eTexture::Texture2D;
    eTextureFormat format         = eTextureFormat::Unknown;
    Int32          arraySize      = 1;   // cubemap count = arraySize / 6
    Int32          samples        = 1;
    Int32          numMips        = 1;
    Int32          refCount       = 0;
    bool           bRtvAttachable = false;
    bool           bDsvAttachable = false;
    bool           bImmutable     = false;
    bool           bReadback      = false;
};
using TextureHandle = Handle<HardwareTextureRef>;

struct FrameBuffer
{
    String name             = {};
    Int32  numRenderTargets = 0;

    Array<TextureHandle, k_maxNumRenderTargetViewSlots>           renderTargetTexhs = { k_nullHandle };
    TextureHandle                                                 depthStencilTexh  = { k_nullHandle };
    Array<ID3D11RenderTargetView*, k_maxNumRenderTargetViewSlots> pRtvs             = { nullptr };
    ID3D11DepthStencilView*                                       pDsv              = nullptr;
};
using FrameBufferHandle = Handle<FrameBuffer>;

struct ShaderRef
{
    union
    {
        ID3D11VertexShader*   pVertexShader = nullptr;
        ID3D11PixelShader*    pPixelShader;
        ID3D11GeometryShader* pGeometryShader;
        ID3D11HullShader*     pHullShader;
        ID3D11DomainShader*   pDomainShader;
        ID3D11ComputeShader*  pComputeShader;
    };

    String  name     = {};
    eShader stage    = eShader::VertexShader;
    Int32   refCount = 0;
    UInt64  hash     = 0;
};
using ShaderHandle = Handle<ShaderRef>;

struct Program
{
    ShaderHandle vsh = k_nullHandle;
    ShaderHandle psh = k_nullHandle;
    ShaderHandle gsh = k_nullHandle;
    ShaderHandle hsh = k_nullHandle;
    ShaderHandle dsh = k_nullHandle;
    ShaderHandle csh = k_nullHandle;
};
using ProgramHandle = Handle<Program>;

////////// STRUCTS ////////////////////////////////////////////////////////////////////////////////////////////////////

struct RGBA
{
    constexpr RGBA()
    {
    }

    explicit constexpr RGBA(const UInt32 _rgba)
        : r(NarrowCast<UInt8>((_rgba >> 24) & 0xFF))
        , g(NarrowCast<UInt8>((_rgba >> 16) & 0xFF))
        , b(NarrowCast<UInt8>((_rgba >> 8) & 0xFF))
        , a(NarrowCast<UInt8>(_rgba & 0xFF))
    {
    }

    constexpr RGBA(const UInt8 _r, const UInt8 _g, const UInt8 _b, const UInt8 _a = 255)
        : r(_r)
        , g(_g)
        , b(_b)
        , a(_a)
    {
    }

    [[nodiscard]] constexpr static RGBA FromLinear(const float _r, const float _g, const float _b, const float _a = 1.0f)
    {
        return {
            static_cast<UInt8>(Clamp(_r, 0.0f, 1.0f) * 255.0f),
            static_cast<UInt8>(Clamp(_g, 0.0f, 1.0f) * 255.0f),
            static_cast<UInt8>(Clamp(_b, 0.0f, 1.0f) * 255.0f),
            static_cast<UInt8>(Clamp(_a, 0.0f, 1.0f) * 255.0f)
        };
    }

    [[nodiscard]] constexpr Vec4 ToLinear() const
    {
        return {
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f
        };
    }

    // RRGGBBAA
    [[nodiscard]] constexpr UInt32 ToRGBA() const
    {
        return (NarrowCast<UInt32>(r) << 24)
             | (NarrowCast<UInt32>(g) << 16)
             | (NarrowCast<UInt32>(b) << 8)
             | NarrowCast<UInt32>(a);
    }

    // AABBGGRR
    [[nodiscard]] constexpr UInt32 ToABGR() const
    {
        return (NarrowCast<UInt32>(a) << 24)
             | (NarrowCast<UInt32>(b) << 16)
             | (NarrowCast<UInt32>(g) << 8)
             | NarrowCast<UInt32>(r);
    }

    // BBGGRRAA
    [[nodiscard]] constexpr UInt32 ToBGRA() const
    {
        return (NarrowCast<UInt32>(b) << 24)
             | (NarrowCast<UInt32>(g) << 16)
             | (NarrowCast<UInt32>(r) << 8)
             | NarrowCast<UInt32>(a);
    }

    [[nodiscard]] constexpr bool operator==(const RGBA& _other) const
    {
        return value == _other.value;
    }

    [[nodiscard]] constexpr bool operator!=(const RGBA& _other) const
    {
        return value != _other.value;
    }

    static RGBA k_white;
    static RGBA k_black;
    static RGBA k_transparent;

    union
    {
        struct
        {
            UInt8 r;
            UInt8 g;
            UInt8 b;
            UInt8 a;
        };
        UInt32 value = 0;
    };
};

struct Viewport
{
    constexpr Viewport()
    {
    }

    constexpr Viewport(const UInt16 _w, const UInt16 _h)
        : x(0)
        , y(0)
        , w(_w)
        , h(_h)
    {
    }

    constexpr Viewport(const UInt16 _x, const UInt16 _y, const UInt16 _w, const UInt16 _h)
        : x(_x)
        , y(_y)
        , w(_w)
        , h(_h)
    {
    }

    [[nodiscard]] constexpr bool operator==(const Viewport& _other) const
    {
        return viewport == _other.viewport;
    }

    [[nodiscard]] constexpr bool operator!=(const Viewport& _other) const
    {
        return viewport != _other.viewport;
    }

    union
    {
        struct
        {
            UInt16 x;   // 뷰포트의 왼쪽 상단 X 좌표
            UInt16 y;   // 뷰포트의 왼쪽 상단 Y 좌표
            UInt16 w;   // 뷰포트의 너비
            UInt16 h;   // 뷰포트의 높이
        };
        UInt64 viewport = 0;
    };
};

struct Rect
{
    constexpr Rect()
    {
    }

    constexpr Rect(const UInt16 _left, const UInt16 _top, const UInt16 _right, const UInt16 _bottom)
        : left(_left)
        , top(_top)
        , right(_right)
        , bottom(_bottom)
    {
    }

    [[nodiscard]] constexpr bool operator==(const Rect& _other) const
    {
        return rect == _other.rect;
    }

    [[nodiscard]] constexpr bool operator!=(const Rect& _other) const
    {
        return rect != _other.rect;
    }

    union
    {
        struct
        {
            UInt16 left;
            UInt16 top;
            UInt16 right;
            UInt16 bottom;
        };
        UInt64 rect = 0;
    };
};

class AnyBufferHandle
{
public:
    AnyBufferHandle() = default;

    /* implicit */ AnyBufferHandle(const VertexBufferHandle _vbh)
        : m_handleValue(_vbh.GetValue())
        , m_type(eBuffer::VertexBuffer)
    {
    }

    /* implicit */ AnyBufferHandle(const IndexBufferHandle _ibh)
        : m_handleValue(_ibh.GetValue())
        , m_type(eBuffer::IndexBuffer)
    {
    }

    /* implicit */ AnyBufferHandle(const ConstantBufferHandle _cbh)
        : m_handleValue(_cbh.GetValue())
        , m_type(eBuffer::ConstantBuffer)
    {
    }

    /* implicit */ AnyBufferHandle(const StorageBufferHandle _sbh)
        : m_handleValue(_sbh.GetValue())
        , m_type(eBuffer::StorageBuffer)
    {
    }

    [[nodiscard]] VertexBufferHandle AsVertexBufferHandle() const
    {
        JAM_ASSERT(m_type == eBuffer::VertexBuffer, "Buffer type mismatch! Expected: VertexBuffer, Actual: {}", NameOf(m_type));
        return m_handleValue;
    }

    [[nodiscard]] IndexBufferHandle AsIndexBufferHandle() const
    {
        JAM_ASSERT(m_type == eBuffer::IndexBuffer, "Buffer type mismatch! Expected: IndexBuffer, Actual: {}", NameOf(m_type));
        return m_handleValue;
    }

    [[nodiscard]] ConstantBufferHandle AsConstantBufferHandle() const
    {
        JAM_ASSERT(m_type == eBuffer::ConstantBuffer, "Buffer type mismatch! Expected: ConstantBuffer, Actual: {}", NameOf(m_type));
        return m_handleValue;
    }

    [[nodiscard]] StorageBufferHandle AsStorageBufferHandle() const
    {
        JAM_ASSERT(m_type == eBuffer::StorageBuffer, "Buffer type mismatch! Expected: StorageBuffer, Actual: {}", NameOf(m_type));
        return m_handleValue;
    }

    [[nodiscard]] UInt32 GetValue() const
    {
        return m_handleValue;
    }

    [[nodiscard]] eBuffer GetType() const
    {
        return m_type;
    }

private:
    UInt32  m_handleValue = k_nullHandleValue;
    eBuffer m_type        = eBuffer::VertexBuffer;
};

struct TextureResolution
{
    Int32 width  = 0;
    Int32 height = 0;
    Int32 depth  = 0;
};

////////// RESOURCE INFO STRUCTS ////////////////////////////////////////////////////////////////////////////////////////////////

struct VertexBufferInfo
{
    Int32              byteWidth    = 0;
    Int32              stride       = 0;
    bool               bImmutable   = false;
    bool               bDynamic     = false;
    VertexLayoutHandle layoutHandle = k_nullHandle;
};

struct IndexBufferInfo
{
    Int32 byteWidth  = 0;
    bool  bImmutable = false;
    bool  bDynamic   = false;
    bool  bUseU32    = false;
};

struct ConstantBufferInfo
{
    Int32 byteWidth = 0;
};

struct StorageBufferInfo
{
    Int32          byteWidth      = 0;
    Int32          numElems       = 0;
    Int32          stride         = 0;
    eStorageBuffer type           = eStorageBuffer::Raw;
    bool           bImmutable     = false;
    bool           bDynamic       = false;
    bool           bReadback      = false;
    bool           bIndirectArgs  = false;
    bool           bAppendConsume = false;
    bool           bCounter       = false;
};

struct TextureInfo
{
    Int32          width          = 0;
    Int32          height         = 0;
    Int32          depth          = 0;
    eTexture       type           = eTexture::Texture2D;
    eTextureFormat format         = eTextureFormat::Unknown;
    Int32          arraySize      = 1;
    Int32          samples        = 1;
    Int32          numMips        = 1;
    bool           bImmutable     = false;
    bool           bReadback      = false;
    bool           bRtvAttachable = false;
    bool           bDsvAttachable = false;
};

struct FrameBufferInfo
{
    Int32 numRenderTargets = 0;
    bool  bHasDepthStencil = false;

    Array<TextureHandle, k_maxNumRenderTargetViewSlots> renderTargetTexhs = { k_nullHandle };
    TextureHandle                                       depthStencilTexh  = { k_nullHandle };
};

struct ShaderInfo
{
    eShader stage = eShader::VertexShader;
};

struct ProgramInfo
{
    ShaderHandle vsh = k_nullHandle;
    ShaderHandle psh = k_nullHandle;
    ShaderHandle hsh = k_nullHandle;
    ShaderHandle dsh = k_nullHandle;
    ShaderHandle gsh = k_nullHandle;
    ShaderHandle csh = k_nullHandle;
};

////////// UTILS ////////////////////////////////////////////////////////////////////////////////////////////////////

[[nodiscard]] bool              IsDepthFormat(eTextureFormat _format);
[[nodiscard]] Int32             BitPerPixelOf(eTextureFormat _format);
[[nodiscard]] Int32             CalcNumMips(Int32 _width, Int32 _height, Int32 _depth);
[[nodiscard]] TextureResolution CalcTextureResolution(Int32 _width, Int32 _height, Int32 _depth, Int32 _mipIndex);
[[nodiscard]] Int32             CalcTextureIndex(Int32 _mipIndex, Int32 _arrayIndex, Int32 _numMips);

}   // namespace Jam
