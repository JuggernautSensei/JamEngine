#pragma once
#include "HandleAllocator.h"
#include "MathUtilities.h"
#include "VertexLayout.h"

namespace jam
{

///////////////////////////////////
///          CONSTANTS          ///
///////////////////////////////////

constexpr Int32 k_renderTargetCountMax              = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
constexpr Int32 k_viewportCountMax                  = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
constexpr Int32 k_samplerStateCountMax              = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
constexpr Int32 k_shaderResourceCountMax            = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
constexpr Int32 k_constantBufferCountMax            = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
constexpr Int32 k_renderingPipelineShaderStageCount = 5;   // vs, ps, gs, hs, ds

constexpr Int32 k_vertexLayoutColorMaxIndex    = 4;
constexpr Int32 k_vertexLayoutTexCoordMaxIndex = 4;

///////////////////////////////
///          ENUMS          ///
///////////////////////////////

enum eFormat : Int8
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

    RGBA16_UNorm,
    RGBA16_SNorm,
    RGBA16_UInt,
    RGBA16_Int,
    RGBA16_Float,

    RGBA32_UInt,
    RGBA32_Int,
    RGBA32_Float,

    D16_UNorm,
    D24_UNorm_S8_UInt,
    D32_Float,
    D32_Float_S8_UInt,
};

enum class eIndexFormat : Int8
{
    U16 = 0,
    U32,
};

enum eTextureCreateFlags_ : UInt8
{
    eTextureCreateFlags_None         = 0,
    eTextureCreateFlags_GenerateMips = 1 << 0,
    eTextureCreateFlags_SRGB         = 1 << 1,

    eTextureCreateFlags_MSAAx4  = 1 << 2,   // 4x MSAA  (다른 MSAA 옵션과 함께 사용시 낮은 MSAA 옵션이 우선됨)
    eTextureCreateFlags_MSAAx8  = 1 << 3,   // 8x MSAA  (``)
    eTextureCreateFlags_MSAAx16 = 1 << 4,   // 16x MSAA (``)

    eTextureCreateFlags_WriteOnly    = 1 << 5,   // 텍스처를 GPU 내부에서 읽기 용도로 쓸 수 없음. (SRV를 만들지 않음)
    eTextureCreateFlags_RenderTarget = 1 << 6,   // 렌더 타겟으로 사용됨.
    eTextureCreateFlags_DepthStencil = 1 << 7,   // 뎁스 스텐실로 사용됨.

    eTextureCreateFlags_MultiSampled_Internal = eTextureCreateFlags_MSAAx4 | eTextureCreateFlags_MSAAx8 | eTextureCreateFlags_MSAAx16,
};
using eTextureCreateFlags = UInt8;

enum class eShader : Int8
{
    Unknown = -1,

    VertexShader = 0,
    PixelShader,
    GeometryShader,
    HullShader,
    DomainShader,
    ComputeShader,
};

enum class eTextureType : Int8
{
    Unknown = -1,

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
    None  = 0,
    Back  = 1,
    Front = 2,
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
    Never,   // 8
};

enum eWriteFlags_ : UInt8
{
    eWriteFlags_None  = 0,
    eWriteFlags_Red   = 1 << 0,
    eWriteFlags_Green = 1 << 1,
    eWriteFlags_Blue  = 1 << 2,
    eWriteFlags_Alpha = 1 << 3,
    eWriteFlags_All   = eWriteFlags_Red | eWriteFlags_Green | eWriteFlags_Blue | eWriteFlags_Alpha,
};
using eWriteFlags = UInt8;

enum class eStencilOp : UInt8
{
    Keep = 0,   // 현재 스텐실 값을 유지
    Zero,       // 스텐실 값을 0으로 설정
    Replace,    // 스텐실 값을 참조 값으로 교체
    IncrSat,    // 스텐실 값을 1 증가 (최대값에 도달하면 유지)
    DecrSat,    // 스텐실 값을 1 감소 (최소값에 도달하면 유지)
    Invert,     // 스텐실 값을 비트 반전
    Incr,       // 스텐실 값을 1 증가 (최대값에 도달하면 0으로 래핑)
    Decr,       // 스텐실 값을 1 감소 (최소값에 도달하면 최대값으로 래핑)
};

enum class eTextureAddress : UInt8
{
    Wrap       = 0,   // 반복
    Mirror     = 1,   // 미러링
    Clamp      = 2,   // 클램핑
    Border     = 3,   // 경계색
    MirrorOnce = 4,   // 미러링(한 번만)
};

enum class eTextureFilter : UInt8
{
    // 기본 필터링
    MinPoint_MagPoint_MipPoint = 0,
    MinPoint_MagPoint_MipLinear,
    MinPoint_MagLinear_MipPoint,
    MinPoint_MagLinear_MipLinear,
    MinLinear_MagPoint_MipPoint,
    MinLinear_MagPoint_MipLinear,
    MinLinear_MagLinear_MipPoint,
    MinLinear_MagLinear_MipLinear,

    // 이방성 필터링
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

enum eShaderCompileFlags_ : UInt8
{
    eShaderCompileFlags_None                  = 0,
    eShaderCompileFlags_Debug                 = 1 << 0,   // 디버그 정보 포함
    eShaderCompileFlags_Optimization          = 1 << 1,   // 최적화 활성화
    eShaderCompileFlags_SkipOptimization      = 1 << 2,   // 최적화 비활성화
    eShaderCompileFlags_WarningsAsErrors      = 1 << 3,   // 경고를 오류로 처리
    eShaderCompileFlags_AvoidFlowControl      = 1 << 4,   // 흐름 제어 피하기
    eShaderCompileFlags_PackMatrixRowMajor    = 1 << 5,   // 행 우선 행렬 패킹
    eShaderCompileFlags_PackMatrixColumnMajor = 1 << 6,   // 열 우선 행렬 패킹

    eShaderCompileFlags_DebugDefault   = eShaderCompileFlags_Debug | eShaderCompileFlags_WarningsAsErrors,
    eShaderCompileFlags_ReleaseDefault = eShaderCompileFlags_Optimization,

#ifdef JAM_DEBUG
    eShaderCompileFlags_Default = eShaderCompileFlags_DebugDefault,
#else
    eShaderCompileFlags_Default = eShaderCompileFlags_ReleaseDefault,
#endif
};
using eShaderCompileFlags = UInt8;

using ShaderCacheKey       = UInt64;
using VertexLayoutCacheKey = UInt64;

///////////////////////////////////
///          RESOURCES          ///
///////////////////////////////////

/// input layout (오직 내부에서만 관리됨)
struct VertexLayoutRef
{
    ID3D11InputLayout*   pInputLayout = nullptr;
    VertexLayout         layout       = {};
    Int32                refCount     = 0;
    VertexLayoutCacheKey hash         = 0;
};
using VertexLayoutHandle = Handle<VertexLayoutRef>;

/// buffer
struct Buffer
{
    ID3D11Buffer* pBuffer   = nullptr;
    Int32         byteWidth = 0;
    bool          bDynamic  = false;
};

/// vertex buffer
struct VertexBuffer
{
    std::string        name         = {};
    Buffer             buffer       = {};
    Int32              vertexCount  = 0;
    Int32              vertexStride = 0;
    VertexLayoutHandle layoutHandle = k_invalidHandle;
};
using VertexBufferHandle = Handle<VertexBuffer>;

/// index buffer
struct IndexBuffer
{
    std::string  name        = {};
    Buffer       buffer      = {};
    Int32        indexCount  = 0;
    Int32        indexStride = 0;
    eIndexFormat format      = eIndexFormat::U16;
};
using IndexBufferHandle = Handle<IndexBuffer>;

/// constant buffer
struct ConstantBuffer
{
    std::string name   = {};
    Buffer      buffer = {};
};
using ConstantBufferHandle = Handle<ConstantBuffer>;

/// texture
struct TextureRef
{
    union
    {
        ID3D11Resource*  pResource = nullptr;
        ID3D11Texture2D* pTexture2D;
        ID3D11Texture3D* pTexture3D;
    };
    ID3D11ShaderResourceView* pSRV = nullptr;

    std::string  name           = {};
    Int32        width          = 0;
    Int32        height         = 0;
    Int32        depth          = 0;
    eTextureType type           = eTextureType::Texture2D;
    eFormat      format         = eFormat::Unknown;
    Int32        arraySize      = 1;   // cubemap count = arraySize / 6
    Int32        samples        = 1;
    Int32        mipsCount      = 1;
    bool         bMutable       = false;
    Int32        refCount       = 0;
    bool         bManaged       = false;
    bool         bRTVAttachable = false;
    bool         bDSVAttachable = false;
};
using TextureHandle = Handle<TextureRef>;

/// frame buffer
struct FrameBuffer
{
    std::string name              = {};
    Int32       renderTargetCount = 0;

    TextureHandle renderTargetTextureHandles[k_renderTargetCountMax] = { k_invalidHandle };
    TextureHandle depthStencilTextureHandle                          = { k_invalidHandle };

    ID3D11RenderTargetView* pRTV[k_renderTargetCountMax] = { nullptr };
    ID3D11DepthStencilView* pDSV                         = nullptr;
};
using FrameBufferHandle = Handle<FrameBuffer>;

/// shader
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

    std::string    name     = {};
    eShader        stage    = eShader::VertexShader;
    Int32          refCount = 0;
    ShaderCacheKey cacheKey = 0;   // 캐시용 키
};
using ShaderHandle = Handle<ShaderRef>;

/// 프로그램
struct Program
{
    ShaderHandle shaderHandles[k_renderingPipelineShaderStageCount] = { k_invalidHandle };
};
using ProgramHandle = Handle<Program>;

////////////////////////////////////////
///          CLASS / STRUCT          ///
////////////////////////////////////////

struct RGBA
{
    constexpr RGBA()
    {
    }

    explicit constexpr RGBA(const UInt32 _rgba)
        : r(static_cast<UInt8>((_rgba >> 24) & 0xFF))
        , g(static_cast<UInt8>((_rgba >> 16) & 0xFF))
        , b(static_cast<UInt8>((_rgba >> 8) & 0xFF))
        , a(static_cast<UInt8>(_rgba & 0xFF))
    {
    }

    constexpr RGBA(const UInt8 _r, const UInt8 _g, const UInt8 _b, const UInt8 _a = 255)
        : r(_r)
        , g(_g)
        , b(_b)
        , a(_a)
    {
    }

    JAM_NODISCARD constexpr static RGBA FromUNorm(const float _r, const float _g, const float _b, const float _a = 1.0f)
    {
        return {
            static_cast<UInt8>(Clamp(_r, 0.0f, 1.0f) * 255.0f),
            static_cast<UInt8>(Clamp(_g, 0.0f, 1.0f) * 255.0f),
            static_cast<UInt8>(Clamp(_b, 0.0f, 1.0f) * 255.0f),
            static_cast<UInt8>(Clamp(_a, 0.0f, 1.0f) * 255.0f)
        };
    }

    JAM_NODISCARD bool operator==(const RGBA& _other) const
    {
        return color == _other.color;
    }

    JAM_NODISCARD bool operator!=(const RGBA& _other) const
    {
        return color != _other.color;
    }

    static RGBA k_white;
    static RGBA k_black;

    union
    {
        struct
        {
            UInt8 r;
            UInt8 g;
            UInt8 b;
            UInt8 a;
        };
        UInt32 color = 0;
    };
};

struct Viewport
{
    Viewport()
    {
    }

    Viewport(const UInt16 _x, const UInt16 _y, const UInt16 _w, const UInt16 _h)
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

struct GfxState
{
    constexpr GfxState()
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
        , bDepthWrite(true)
    {
    }

    constexpr GfxState(
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

    using IntType = UInt32;
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
        IntType value = 0;
    };

    static GfxState k_default;     // 기본 렌더링 상태 (삼각형 리스트, 뒷면 컬링)
    static GfxState k_depthTest;   // 깊이 테스트 및 쓰기 활성화된 상태, 나머지는 기본값
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
        , writeMask(eWriteFlags_All)
    {
    }

    constexpr BlendState(
        const bool        _bEnable,
        const eBlend      _srcBlend,
        const eBlend      _dstBlend,
        const eBlendOp    _blendOp,
        const eBlend      _srcBlendAlpha,
        const eBlend      _dstBlendAlpha,
        const eBlendOp    _blendOpAlpha,
        const eWriteFlags _writeMask)
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

    // 블랜드 공식
    // FinalColor = (SrcColor * SrcBlend) (BlendOp) (DstColor * DstBlend)
    using IntType = UInt32;
    union
    {
        struct
        {
            bool        bEnable : 1;         // 블렌딩 활성화 여부 (기본: 비활성화, true: 활성화)
            eBlend      srcBlend : 4;        // 소스 블렌드 팩터 (기본: One)
            eBlend      dstBlend : 4;        // 대상 블렌드 팩터 (기본: Zero)
            eBlendOp    blendOp : 3;         // 블렌드 연산자 (기본: Add)
            eBlend      srcBlendAlpha : 4;   // 소스 알파 블렌드 팩터 (기본: One)
            eBlend      dstBlendAlpha : 4;   // 대상 알파 블렌드 팩터 (기본: Zero)
            eBlendOp    blendOpAlpha : 3;    // 알파 블렌드 연산자 (기본: Add)
            eWriteFlags writeMask : 4;       // 렌더 타겟에 쓰기 마스크 (기본: RGBA 모두 활성화)
        };
        IntType value = 0;
    };

    static BlendState k_opaque;        // 블렌딩 비활성화 (Src=1, Dst=0). 픽셀을 그대로 덮어씀.
    static BlendState k_alpha;         // 알파 블렌딩 (Src=SrcAlpha, Dst=InvSrcAlpha). 투명/반투명 표현.
    static BlendState k_additive;      // 가산 블렌딩 (Src=1, Dst=1). 색상을 더해 밝게 만듦.
    static BlendState k_premult;       // 프리멀티 알파 (Src=1, Dst=InvSrcAlpha). SrcColor가 이미 알파 곱 적용된 경우 사용.
    static BlendState k_subtractive;   // 감산 블렌딩 (Src=1, Dst=1, BlendOp=ReverseSubtract). 기존 색에서 소스 색을 뺌.
    static BlendState k_multiply;      // 곱셈 블렌딩 (Src=DstColor, Dst=Zero). 색을 곱해 어둡게 만듦.
    static BlendState k_screen;        // 스크린 블렌딩 (Src=1, Dst=InvSrcColor). 반전된 곱셈, 색이 겹칠수록 밝아짐.
    static BlendState k_overlay;       // 오버레이 블렌딩. Dst<0.5면 Multiply, Dst>=0.5면 Screen. 명암 대비 강조.
    static BlendState k_darkening;     // 다크닝 블렌딩 (Src=DstColor, Dst=SrcColor). 색이 겹칠수록 어두워짐.
    static BlendState k_lightening;    // 라이트닝 블렌딩 (Src=1, Dst=InvSrcColor). 색이 겹칠수록 밝아짐.
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

    using IntType = UInt16;
    union
    {
        struct
        {
            eStencilOp   stencilFailOp : 3;   // 뎁스 통과 & 스텐실 실패 시 수행할 작업
            eStencilOp   depthFailOp : 3;     // 뎁스 실패 & 스텐실 통과 시 수행할 작업
            eStencilOp   passOp : 3;          // 뎁스 통과 & 스텐실 통과 시 수행할 작업
            eCompareFunc compareFunc : 4;     // 스텐실 테스트 비교 함수 (기본: None, 즉, 스텐실 테스트 비활성화)
        };
        IntType value = 0;
    };

    static StencilState k_disable;   // 기본 스텐실 상태 (스텐실 비활성화)
};

struct SamplerState
{
    constexpr SamplerState()
        : filter(eTextureFilter::MinPoint_MagPoint_MipPoint)
        , addressU(eTextureAddress::Wrap)
        , addressV(eTextureAddress::Wrap)
        , addressW(eTextureAddress::Wrap)
        , compareFunc(eCompareFunc::None)
    {
    }

    constexpr SamplerState(
        const eTextureFilter  _filter,
        const eTextureAddress _addressU,
        const eTextureAddress _addressV,
        const eTextureAddress _addressW,
        const eCompareFunc    _compareFunc)
        : filter(_filter)
        , addressU(_addressU)
        , addressV(_addressV)
        , addressW(_addressW)
        , compareFunc(_compareFunc)
    {
    }

    using IntType = UInt32;
    union
    {
        struct
        {
            eTextureFilter  filter : 5;        // 필터링 모드 (기본값은 MinMagMipPoint)
            eTextureAddress addressU : 3;      // U 좌표 래핑 모드 (기본값은 Wrap)
            eTextureAddress addressV : 3;      // V 좌표 래핑 모드 (기본값은 Wrap)
            eTextureAddress addressW : 3;      // W 좌표 래핑 모드 (기본값은 Wrap)
            eCompareFunc    compareFunc : 4;   // 비교 함수 (기본값은 None 비교 필터링 비활성화. 다른 값으로 선택시 비교 필터링 활성화)
        };
        IntType value = 0;
    };

    static SamplerState k_default;       // 기본 샘플러 상태 (점 샘플링, 래핑)
    static SamplerState k_pointClamp;    // 점 샘플링, 클램프
    static SamplerState k_linearClamp;   // 선형 샘플링, 클램프
    static SamplerState k_pointWrap;     // 점 샘플링, 래핑
    static SamplerState k_linearWrap;    // 선형 샘플링, 래핑
    static SamplerState k_shadow;        // 그림자 맵용 (비교 필터링, 선형, 클램프)
};

///////////////////////////////////////
///          RESOURCE INFO          ///
///////////////////////////////////////

struct VertexBufferInfo
{
    Int32              byteWidth    = 0;
    Int32              vertexCount  = 0;
    Int32              vertexStride = 0;
    bool               bDynamic     = false;
    VertexLayoutHandle layoutHandle = k_invalidHandle;
};

struct IndexBufferInfo
{
    Int32        byteWidth  = 0;
    Int32        indexCount = 0;
    eIndexFormat format     = eIndexFormat::U16;
    bool         bDynamic   = false;
};

struct ConstantBufferInfo
{
    Int32 byteWidth = 0;
};

struct TextureInfo
{
    Int32        width                   = 0;
    Int32        height                  = 0;
    Int32        depth                   = 0;
    eTextureType type                    = eTextureType::Texture2D;
    eFormat      format                  = eFormat::Unknown;
    Int32        arraySize               = 1;
    Int32        samples                 = 1;
    Int32        mipsCount               = 1;
    bool         bMutable                = false;
    bool         bRenderTargetAttachable = false;
    bool         bDepthStencilAttachable = false;
};

struct FrameBufferInfo
{
    Int32 renderTargetCount = 0;
    bool  bHasDepthStencil  = false;

    TextureHandle renderTargetTextureHandles[k_renderTargetCountMax] = { k_invalidHandle };
    TextureHandle depthStencilTextureHandle                          = { k_invalidHandle };
};

struct ShaderInfo
{
    eShader stage = eShader::VertexShader;
};

struct ProgramInfo
{
    ShaderHandle shaderHandles[k_renderingPipelineShaderStageCount] = { k_invalidHandle };
};

///////////////////////////////////
///          UTILITIES          ///
///////////////////////////////////

JAM_NODISCARD Int32 GetPixelByte(eFormat _format);
JAM_NODISCARD bool  IsDepthFormat(eFormat _format);
JAM_NODISCARD Int32 GetIndexFormatByte(eIndexFormat _format);
JAM_NODISCARD Int32 CalcMipsCount(Int32 _width, Int32 _height, Int32 _depth);
JAM_NODISCARD std::tuple<Int32, Int32, Int32> CalcMipSize(Int32 _width, Int32 _height, Int32 _depth, Int32 _mipIndex);
JAM_NODISCARD Int32                           CalcTextureIndex(Int32 _mipIndex, Int32 _arrayIndex, Int32 _mipsCount);

}   // namespace jam