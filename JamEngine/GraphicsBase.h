#pragma once
// ReSharper disable once CppUnusedIncludeDirective
#include <DirectXTex.h>
#include "HandleAllocator.h"
#include "Math.h"
#include "TextureFormat.h"
#include "VertexLayout.h"

namespace Jam
{

constexpr Int32 k_maxNumRenderTargetViewSlots   = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
constexpr Int32 k_maxNumAttachmentSlots         = k_maxNumRenderTargetViewSlots + 1;   // render target + depth stencil
constexpr Int32 k_maxNumSamplerSlots            = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
constexpr Int32 k_maxNumShaderResourceViewSlots = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
constexpr Int32 k_maxNumConstantBufferSlots     = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;

enum class eTextureCreateFlags : UInt32
{
    None         = 0,
    GenerateMips = 1 << 0,

    // if multiple MSAA flags are set, the lowest one takes precedence
    MSAAx4  = 1 << 2,   // 4x MSAA
    MSAAx8  = 1 << 3,   // 8x MSAA
    MSAAx16 = 1 << 4,   // 16x MSAA

    WriteOnly    = 1 << 5,   // do not create shader resource view. read only
    RenderTarget = 1 << 6,   // for render target usage
    DepthStencil = 1 << 7,   // for depth stencil usage

    Mutable = 1 << 8,   // when creating with initial renderable, create immutable texture. this flag makes it mutable even though initial renderable is provided

    MultiSampled_Internal = MSAAx4 | MSAAx8 | MSAAx16,
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

constexpr eShader k_gfxShaderStages[] = {
    eShader::VertexShader,
    eShader::PixelShader,
    eShader::GeometryShader,
    eShader::HullShader,
    eShader::DomainShader
    // exclude compute shader
};

enum class eTextureType
{
    Texture2D = 0,
    Texture3D,
    TextureCube,

};

enum class eBlend : UInt8
{
    Zero = 0,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DstAlpha,
    InvDstAlpha,
    DstColor,
    InvDstColor,
    SrcAlphaSat,
    BlendFactor,
    InvBlendFactor
};

enum class eBlendOp : UInt8
{
    Add = 0,
    Subtract,
    RevSubtract,
    Min,
    Max
};

enum class eCullMode : UInt8
{
    None = 0,
    Back,
    Front,
};

enum class ePrimitive : UInt8
{
    TriangleList = 0,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList,
};

enum class eCompareFunc : UInt8
{
    None,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    Always,
    Never,
};

enum class eWriteFlags : UInt8
{
    None  = 0,
    Red   = 1 << 0,
    Green = 1 << 1,
    Blue  = 1 << 2,
    Alpha = 1 << 3,
    All   = Red | Green | Blue | Alpha,
};

enum class eStencilOp : UInt8
{
    Keep = 0,
    Zero,
    Replace,
    IncrSat,
    DecrSat,
    Invert,
    Incr,
    Decr,
};

enum class eTextureAddressMode : UInt8
{
    Wrap       = 0,
    Mirror     = 1,
    Clamp      = 2,
    Border     = 3,
    MirrorOnce = 4,
};

enum class eTextureFilter : UInt8
{
    MinPoint_MagPoint_MipPoint = 0,
    MinPoint_MagPoint_MipLinear,
    MinPoint_MagLinear_MipPoint,
    MinPoint_MagLinear_MipLinear,
    MinLinear_MagPoint_MipPoint,
    MinLinear_MagPoint_MipLinear,
    MinLinear_MagLinear_MipPoint,
    MinLinear_MagLinear_MipLinear,
    Anisotropic4,
    Anisotropic8,
    Anisotropic16,
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

enum class eSubmitParameterType
{
    StartVertexLocation = 0,
    StartIndexLocation,
    BaseVertexLocation,
};

struct VertexLayoutRef
{
    ID3D11InputLayout* pInputLayout = nullptr;
    VertexLayout       vertexLayout = {};
    Int32              refCount     = 0;
    UInt64             hash         = 0;
};
using VertexLayoutHandle = Handle<VertexLayoutRef>;

struct HardwareBuffer
{
    ID3D11Buffer* pBuffer   = nullptr;
    Int32         byteWidth = 0;
    bool          bDynamic  = false;
};

struct VertexBuffer : HardwareBuffer
{
    String             name     = {};
    Int32              numVerts = 0;
    Int32              stride   = 0;
    VertexLayoutHandle vlh      = k_nullHandle;
};
using VertexBufferHandle = Handle<VertexBuffer>;

struct IndexBuffer : HardwareBuffer
{
    String name     = {};
    Int32  numIdxes = 0;
    Int32  stride   = 0;
    bool   bUseU32  = false;
};
using IndexBufferHandle = Handle<IndexBuffer>;

struct ConstantBuffer : HardwareBuffer
{
    String name = {};
};
using ConstantBufferHandle = Handle<ConstantBuffer>;

struct HardwareTextureRef
{
    union
    {
        ID3D11Resource*  pResource = nullptr;
        ID3D11Texture2D* pTexture2D;
        ID3D11Texture3D* pTexture3D;
    };
    ID3D11ShaderResourceView* pSrv = nullptr;

    String         name           = {};
    Int32          width          = 0;
    Int32          height         = 0;
    Int32          depth          = 0;
    eTextureType   type           = eTextureType::Texture2D;
    eTextureFormat format         = eTextureFormat::Unknown;
    Int32          arraySize      = 1;   // cubemap count = arraySize / 6
    Int32          samples        = 1;
    Int32          numMips        = 1;
    bool           bMutable       = false;
    Int32          refCount       = 0;
    bool           bManaged       = false;
    bool           bRtvAttachable = false;
    bool           bDsvAttachable = false;
};
using TextureHandle = Handle<HardwareTextureRef>;

struct FrameBuffer
{
    String name             = {};
    Int32  numRenderTargets = 0;

    Array<TextureHandle, k_maxNumRenderTargetViewSlots> renderTargetTexhs = { k_nullHandle };
    TextureHandle                                       depthStencilTexh  = { k_nullHandle };

    Array<ID3D11RenderTargetView*, k_maxNumRenderTargetViewSlots> pRtvs = { nullptr };
    ID3D11DepthStencilView*                                       pDsv  = nullptr;
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
    UInt64  cache    = 0;
};
using ShaderHandle = Handle<ShaderRef>;

struct Program
{
    EnumArray<eShader, ShaderHandle> shs = {};
};
using ProgramHandle = Handle<Program>;

struct RGBA
{
    constexpr RGBA()
    {
    }

    explicit constexpr RGBA(const UInt32 _rgba)
        : r(NumericCast<UInt8>((_rgba >> 24) & 0xFF))
        , g(NumericCast<UInt8>((_rgba >> 16) & 0xFF))
        , b(NumericCast<UInt8>((_rgba >> 8) & 0xFF))
        , a(NumericCast<UInt8>(_rgba & 0xFF))
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
            NumericCast<UInt8>(Clamp(_r, 0.0f, 1.0f) * 255.0f),
            NumericCast<UInt8>(Clamp(_g, 0.0f, 1.0f) * 255.0f),
            NumericCast<UInt8>(Clamp(_b, 0.0f, 1.0f) * 255.0f),
            NumericCast<UInt8>(Clamp(_a, 0.0f, 1.0f) * 255.0f)
        };
    }

    [[nodiscard]] constexpr Vector4 ToLinear() const
    {
        return {
            NumericCast<float>(r) / 255.0f,
            NumericCast<float>(g) / 255.0f,
            NumericCast<float>(b) / 255.0f,
            NumericCast<float>(a) / 255.0f
        };
    }

    // RRGGBBAA
    [[nodiscard]] constexpr UInt32 ToRGBA() const
    {
        return (NumericCast<UInt32>(r) << 24)
             | (NumericCast<UInt32>(g) << 16)
             | (NumericCast<UInt32>(b) << 8)
             | NumericCast<UInt32>(a);
    }

    // AABBGGRR
    [[nodiscard]] constexpr UInt32 ToABGR() const
    {
        return (NumericCast<UInt32>(a) << 24)
             | (NumericCast<UInt32>(b) << 16)
             | (NumericCast<UInt32>(g) << 8)
             | NumericCast<UInt32>(r);
    }

    // BBGGRRAA
    [[nodiscard]] constexpr UInt32 ToBGRA() const
    {
        return (NumericCast<UInt32>(b) << 24)
             | (NumericCast<UInt32>(g) << 16)
             | (NumericCast<UInt32>(r) << 8)
             | NumericCast<UInt32>(a);
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

struct RenderState
{
    constexpr RenderState()
        : primitive(ePrimitive::TriangleList)
        , cullMode(eCullMode::Back)
        , bWireframe(false)
        , bDepthClamp(false)
        , bMultiSample(false)
        , bLineAA(false)
        , bScissor(false)
        , bCCWFront(false)
        , bAlphaToCoverage(false)
        , bIndependentBlend(false)
        , depthTestFunc(eCompareFunc::None)
        , bDepthWrite(false)
    {
    }

    constexpr RenderState(
        const ePrimitive   _primitive,
        const eCullMode    _cullMode,
        const bool         _bWireframe,
        const bool         _bDepthClamp,
        const bool         _bMultiSample,
        const bool         _bLineAA,
        const bool         _bScissor,
        const bool         _bCCWFront,
        const bool         _bAlphaToCoverage,
        const bool         _bIndependentBlend,
        const eCompareFunc _depthTestFunc,
        const bool         _bDepthWrite)
        : primitive(_primitive)
        , cullMode(_cullMode)
        , bWireframe(_bWireframe)
        , bDepthClamp(_bDepthClamp)
        , bMultiSample(_bMultiSample)
        , bLineAA(_bLineAA)
        , bScissor(_bScissor)
        , bCCWFront(_bCCWFront)
        , bAlphaToCoverage(_bAlphaToCoverage)
        , bIndependentBlend(_bIndependentBlend)
        , depthTestFunc(_depthTestFunc)
        , bDepthWrite(_bDepthWrite)
    {
    }

    static RenderState k_default;
    static RenderState k_depthTest;

    union
    {
        struct
        {
            ePrimitive   primitive : 3;           // 프리미티브 타입 (기본: 삼각형 리스트)
            eCullMode    cullMode : 2;            // 컬링 모드 (기본: 백 페이스 컬링)
            bool         bWireframe : 1;          // Fill 모드 (기본: 솔리드, true: 와이어프레임)
            bool         bDepthClamp : 1;         // 깊이 클램프 활성화 여부 (기본: 깊이를 벗어나는 픽셀은 클리핑, true: 깊이를 벗어나는 픽셀 클립하지 않음)
            bool         bMultiSample : 1;        // 멀티샘플링 활성화 여부 (기본: 비활성화, true: 활성화) 이는 MSAA 렌더 타겟에만 영향을 미침
            bool         bLineAA : 1;             // 라인 안티앨리어싱 활성화 여부 (기본: 비활성화, true: 활성화) 이는 MSAA 렌더 타겟에만 영향을 미침
            bool         bScissor : 1;            // 스크리소 활성화 여부 (기본: 비활성화, true: 활성화)
            bool         bCCWFront : 1;           // 반시계방향을 앞면으로 설정 (기본: 시계방향이 앞면, true: 반시계방향이 앞면)
            bool         bAlphaToCoverage : 1;    // 알파 투 커버리지 활성화 여부 (기본: 비활성화, true: 활성화) 이는 MSAA 렌더 타겟에만 영향을 미침
            bool         bIndependentBlend : 1;   // 독립적 블렌딩 활성화 여부 (기본: 비활성화, true: 활성화) 이는 8개 렌더 타겟 각각에 대해 별도의 블렌드 상태를 설정할 수 있게 함
            eCompareFunc depthTestFunc : 4;       // 깊이 테스트 비교 함수 (기본: None. 즉, 깊이 테스트 비활성화)
            bool         bDepthWrite : 1;         // 깊이 쓰기 활성화 여부 (기본: 비활성화, true: 활성화)
        };
        UInt32 value = 0;
    };
};

struct BlendState
{
    constexpr BlendState()
        : bEnable(false)
        , srcBlend(eBlend::One)
        , dstBlend(eBlend::Zero)
        , blendOp(eBlendOp::Add)
        , srcBlendAlpha(eBlend::One)
        , dstBlendAlpha(eBlend::Zero)
        , blendOpAlpha(eBlendOp::Add)
        , writeMask(eWriteFlags::All)
    {
    }

    constexpr BlendState(
        const bool               _bEnable,
        const eBlend             _srcBlend,
        const eBlend             _dstBlend,
        const eBlendOp           _blendOp,
        const eBlend             _srcBlendAlpha,
        const eBlend             _dstBlendAlpha,
        const eBlendOp           _blendOpAlpha,
        const Flags<eWriteFlags> _writeMask)
        : bEnable(_bEnable)
        , srcBlend(_srcBlend)
        , dstBlend(_dstBlend)
        , blendOp(_blendOp)
        , srcBlendAlpha(_srcBlendAlpha)
        , dstBlendAlpha(_dstBlendAlpha)
        , blendOpAlpha(_blendOpAlpha)
        , writeMask(_writeMask)
    {
    }

    static BlendState k_opaque;
    static BlendState k_alphaBlend;
    static BlendState k_additive;
    static BlendState k_premultipliedAlpha;
    static BlendState k_subtractive;
    static BlendState k_multiply;
    static BlendState k_screen;
    static BlendState k_overlay;

    // final m_value = (src m_value * src blendBinds) (blendBinds op) (dst m_value * dst blendBinds)
    union
    {
        struct
        {
            bool               bEnable : 1;         // 블렌딩 활성화 여부 (기본: 비활성화, true: 활성화)
            eBlend             srcBlend : 4;        // 소스 블렌드 팩터 (기본: One)
            eBlend             dstBlend : 4;        // 대상 블렌드 팩터 (기본: Zero)
            eBlendOp           blendOp : 3;         // 블렌드 연산자 (기본: Add_)
            eBlend             srcBlendAlpha : 4;   // 소스 알파 블렌드 팩터 (기본: One)
            eBlend             dstBlendAlpha : 4;   // 대상 알파 블렌드 팩터 (기본: Zero)
            eBlendOp           blendOpAlpha : 3;    // 알파 블렌드 연산자 (기본: Add_)
            Flags<eWriteFlags> writeMask;           // 렌더 타겟에 쓰기 마스크 (기본: RGBA 모두 활성화)
        };
        UInt32 value = 0;
    };
};

struct StencilState
{
    constexpr StencilState()
        : stencilFailOp(eStencilOp::Keep)
        , depthFailOp(eStencilOp::Keep)
        , passOp(eStencilOp::Keep)
        , compareFunc(eCompareFunc::None)
    {
    }

    constexpr StencilState(
        const eStencilOp   _stencilFailOp,
        const eStencilOp   _depthFailOp,
        const eStencilOp   _passOp,
        const eCompareFunc _compareFunc)
        : stencilFailOp(_stencilFailOp)
        , depthFailOp(_depthFailOp)
        , passOp(_passOp)
        , compareFunc(_compareFunc)
    {
    }

    static StencilState k_disable;

    union
    {
        struct
        {
            eStencilOp   stencilFailOp : 3;   // 뎁스 통과 & 스텐실 실패 시 수행할 작업
            eStencilOp   depthFailOp : 3;     // 뎁스 실패 & 스텐실 통과 시 수행할 작업
            eStencilOp   passOp : 3;          // 뎁스 통과 & 스텐실 통과 시 수행할 작업
            eCompareFunc compareFunc : 4;     // 스텐실 테스트 비교 함수 (기본: None, 즉, 스텐실 테스트 비활성화)
        };
        UInt16 value = 0;
    };
};

struct SamplerState
{
    constexpr SamplerState()
        : filter(eTextureFilter::MinPoint_MagPoint_MipPoint)
        , addressU(eTextureAddressMode::Wrap)
        , addressV(eTextureAddressMode::Wrap)
        , addressW(eTextureAddressMode::Wrap)
        , compareFunc(eCompareFunc::None)
    {
    }

    constexpr SamplerState(
        const eTextureFilter      _filter,
        const eTextureAddressMode _addressU,
        const eTextureAddressMode _addressV,
        const eTextureAddressMode _addressW,
        const eCompareFunc        _compareFunc)
        : filter(_filter)
        , addressU(_addressU)
        , addressV(_addressV)
        , addressW(_addressW)
        , compareFunc(_compareFunc)
    {
    }

    static SamplerState k_pointClamp;
    static SamplerState k_linearClamp;
    static SamplerState k_pointWrap;
    static SamplerState k_linearWrap;

    union
    {
        struct
        {
            eTextureFilter      filter : 5;        // 필터링 모드 (기본값은 MinMagMipPoint)
            eTextureAddressMode addressU : 3;      // U 좌표 래핑 모드 (기본값은 Wrap)
            eTextureAddressMode addressV : 3;      // V 좌표 래핑 모드 (기본값은 Wrap)
            eTextureAddressMode addressW : 3;      // W 좌표 래핑 모드 (기본값은 Wrap)
            eCompareFunc        compareFunc : 4;   // 비교 함수 (기본값은 None 비교 필터링 비활성화. 다른 값으로 선택시 비교 필터링 활성화)
        };
        UInt32 value = 0;
    };
};

///////////////////////////////////////
///          RESOURCE INFO          ///
///////////////////////////////////////

struct VertexBufferInfo
{
    Int32              byteWidth    = 0;
    Int32              numVerts     = 0;
    Int32              vertexStride = 0;
    bool               bDynamic     = false;
    VertexLayoutHandle layoutHandle = k_nullHandle;
};

struct IndexBufferInfo
{
    Int32 byteWidth  = 0;
    Int32 numIndices = 0;
    bool  bUseU32    = false;
    bool  bDynamic   = false;
};

struct ConstantBufferInfo
{
    Int32 byteWidth = 0;
};

struct TextureInfo
{
    Int32          width                   = 0;
    Int32          height                  = 0;
    Int32          depth                   = 0;
    eTextureType   type                    = eTextureType::Texture2D;
    eTextureFormat format                  = eTextureFormat::Unknown;
    Int32          arraySize               = 1;
    Int32          samples                 = 1;
    Int32          numMips                 = 1;
    bool           bMutable                = false;
    bool           bRenderTargetAttachable = false;
    bool           bDepthStencilAttachable = false;
};

struct TextureResolution
{
    Int32 width  = 0;
    Int32 height = 0;
    Int32 depth  = 0;
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
    EnumArray<eShader, ShaderHandle> shs = {};
};

[[nodiscard]] bool              IsDepthFormat(eTextureFormat _format);
[[nodiscard]] Int32             BytePerPixelOf(eTextureFormat _format);
[[nodiscard]] Int32             CalcNumMips(Int32 _width, Int32 _height, Int32 _depth);
[[nodiscard]] TextureResolution CalcTextureResolution(Int32 _width, Int32 _height, Int32 _depth, Int32 _mipIndex);
[[nodiscard]] Int32             CalcTextureIndex(Int32 _mipIndex, Int32 _arrayIndex, Int32 _numMips);

}   // namespace Jam