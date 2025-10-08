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

enum class eBlend : Int8
{
    Zero           = 0x00,
    One            = 0x01,
    SrcColor       = 0x02,
    InvSrcColor    = 0x03,
    SrcAlpha       = 0x04,
    InvSrcAlpha    = 0x05,
    DstAlpha       = 0x06,
    InvDstAlpha    = 0x07,
    DstColor       = 0x08,
    InvDstColor    = 0x09,
    SrcAlphaSat    = 0x0A,
    BlendFactor    = 0x0B,
    InvBlendFactor = 0x0C
};

enum class eBlendOp : Int8
{
    Add         = 0x00,
    Subtract    = 0x01,
    RevSubtract = 0x02,
    Min         = 0x03,
    Max         = 0x04
};

enum class eTextureType : Int8
{
    Unknown = -1,

    Texture2D = 0,
    Texture3D,
    TextureCube,
};

/// eStateFlags : 64 bit
///
/// [44bit: blend state] [8 bit: depth stencil state] [12 bit: rasterizer state]
///  │
///  └ [12 bit: RT2] [12 bit: RT1] [12 bit: RT0] [4 bit: blend misc] [4 bit: write mask]
///
/// 하위 16비트 0b0000.0000.0000.0000
///               │    │   │ │   └── -> 4bit: wireframe, depth clip, multisampling, line AA
///               │    │   │ └─ -> 2bit: scissor, CCW front
///               │    │   └─ -> 2bit: cull mode
///               │    └── -> 4bit: primitive topology
///               └── -> 4bit: depth test + death read mask
enum eStateFlags_ : UInt64
{
    eStateFlags_None = UINT64_C(0x0000'0000'0000'0000),

    // 필 모드 | 뎁스 클립 | 멀티샘플링 | 선 안티앨리어싱
    eStateFlags_Wireframe   = UINT64_C(0x0000'0000'0000'0001),   // fill mode (기본은 솔리드)
    eStateFlags_DepthClamp  = UINT64_C(0x0000'0000'0000'0002),   // 뎁스 0.0 ~ 1.0 밖의 값들을 클리핑하지않음. (뎁스 클립 비활성화)
    eStateFlags_MultiSample = UINT64_C(0x0000'0000'0000'0004),   // multi-sample enable (멀티샘플링 안티앨리어싱)
    eStateFlags_LineAA      = UINT64_C(0x0000'0000'0000'0008),   // line anti-aliasing (DX9 스타일의 선 안티앨리어싱, DX10 이상에서는 항상 활성화됨)

    // 시저 | 전면 방향 설정
    eStateFlags_Scissor  = UINT64_C(0x0000'0000'0000'0010),   // scissor enable (시저 영역 밖의 픽셀 클리핑)
    eStateFlags_CCWFront = UINT64_C(0x0000'0000'0000'0020),   // counter-clockwise front (시계 반대 방향을 앞면으로 설정, 기본값은 시계 방향이 앞면)

    // 컬링
    eStateFlags_CullNone  = 0 << 6,   // 컬링 비활성화 (기본값)
    eStateFlags_CullFront = 1 << 6,   // cull front face (앞면 컬링 활성화)
    eStateFlags_CullBack  = 2 << 6,   // 뒷면 컬링 활성화

    // 프리미티브 토폴로지
    eStateFlags_PrimitiveTopology_TriangleList  = UINT64_C(0x0000'0000'0000'0000),   // 삼각형 리스트 (기본값)
    eStateFlags_PrimitiveTopology_TriangleStrip = UINT64_C(0x0000'0000'0000'0100),   // 삼각형 스트립
    eStateFlags_PrimitiveTopology_LineList      = UINT64_C(0x0000'0000'0000'0200),   // 선 리스트
    eStateFlags_PrimitiveTopology_LineStrip     = UINT64_C(0x0000'0000'0000'0300),   // 선 스트립
    eStateFlags_PrimitiveTopology_PointList     = UINT64_C(0x0000'0000'0000'0400),   // 점 리스트

    eStateFlags_DepthWrite = UINT64_C(0x0000'0000'0000'1000),   // 뎁스 버퍼에 쓰기 (뎁스 테스트가 활성화 되어 있어야 함)

    // 뎁스 테스트 함수. 미설정시 뎁스 테스트 비활성화
    eStateFlags_DepthTest_Less         = UINT64_C(0x0000'0000'0001'0000),   // 뎁스 비교 함수 (적힐 픽셀의 뎁스 값 < 현재 뎁스 버퍼의 뎁스 값. 통과. 기본값)
    eStateFlags_DepthTest_LessEqual    = UINT64_C(0x0000'0000'0002'0000),   // 뎁스 비교 함수 (적힐 픽셀의 뎁스 값 <= 현재 뎁스 버퍼의 뎁스 값. 통과)
    eStateFlags_DepthTest_Greater      = UINT64_C(0x0000'0000'0003'0000),   // 뎁스 비교 함수 (적힐 픽셀의 뎁스 값 > 현재 뎁스 버퍼의 뎁스 값. 통과)
    eStateFlags_DepthTest_GreaterEqual = UINT64_C(0x0000'0000'0004'0000),   // 뎁스 비교 함수 (적힐 픽셀의 뎁스 값 >= 현재 뎁스 버퍼의 뎁스 값. 통과)
    eStateFlags_DepthTest_Equal        = UINT64_C(0x0000'0000'0005'0000),   // 뎁스 비교 함수 (적힐 픽셀의 뎁스 값 == 현재 뎁스 버퍼의 뎁스 값. 통과)
    eStateFlags_DepthTest_NotEqual     = UINT64_C(0x0000'0000'0006'0000),   // 뎁스 비교 함수 (적힐 픽셀의 뎁스 값 != 현재 뎁스 버퍼의 뎁스 값. 통과)
    eStateFlags_DepthTest_Always       = UINT64_C(0x0000'0000'0007'0000),   // 뎁스 비교 함수 (항상 통과)
    eStateFlags_DepthTest_Never        = UINT64_C(0x0000'0000'0008'0000),   // 뎁스 비교 함수 (항상 실패)

    // RGBA WriteMask
    eStateFlags_WriteMask_R    = UINT64_C(0x0000'0000'0010'0000),   // 블렌딩 시 출력 컬러의 R 채널을 렌더 타겟에 쓰기
    eStateFlags_WriteMask_G    = UINT64_C(0x0000'0000'0020'0000),   // 블렌딩 시 출력 컬러의 G 채널을 렌더 타겟에 쓰기
    eStateFlags_WriteMask_B    = UINT64_C(0x0000'0000'0040'0000),   // 블렌딩 시 출력 컬러의 B 채널을 렌더 타겟에 쓰기
    eStateFlags_WriteMask_A    = UINT64_C(0x0000'0000'0080'0000),   // 블렌딩 시 출력 컬러의 A 채널을 렌더 타겟에 쓰기
    eStateFlags_WriteMask_RGB  = eStateFlags_WriteMask_R | eStateFlags_WriteMask_G | eStateFlags_WriteMask_B,
    eStateFlags_WriteMask_RGBA = eStateFlags_WriteMask_RGB | eStateFlags_WriteMask_A,

    eStateFlags_IndependentBlend = UINT64_C(0x0000'0000'0100'0000),   // 렌더 타켓 마다 독립적인 블렌드 스테이트 설정 (최대 3개까지 지원)
    eStateFlags_AlphaToCoverage  = UINT64_C(0x0000'0000'0200'0000),   // 알파 투 커버리지 (멀티샘플링 안티앨리어싱 시, 알파 채널을 이용해 샘플링 커버리지 결정)

    ///////////////////////////////////////////
    /// Blend를 설정하고 싶다면              ///
    /// MakeBlendState 함수를 이용할 것.     ///
    ///////////////////////////////////////////

    /* internal */
    eStateFlags_DepthStateShift_Internal         = 12ull,                             // depth stencil state shift
    eStateFlags_DepthStateMask_Internal          = UINT64_C(0x0000'0000'000F'F000),   // depth stencil state mask
    eStateFlags_BlendStateMask_Internal          = UINT64_C(0xFFFF'FFFF'FFF0'0000),   // blend state mask
    eStateFlags_BlendStateRT0Shift_Internal      = 28ull,
    eStateFlags_BlendStateRT0Mask_Internal       = UINT64_C(0x0000'00FF'F000'0000),   // rasterizer state mask
    eStateFlags_BlendStateRT1Mask_Internal       = UINT64_C(0x000F'FF00'0000'0000),   // rasterizer state mask
    eStateFlags_BlendStateRT2Mask_Internal       = UINT64_C(0xFFF0'0000'F000'0000),   // rasterizer state mask
    eStateFlags_RasterizerStateMask_Internal     = UINT64_C(0x0000'0000'0000'0FFF),   // rasterizer state mask
    eStateFlags_BlendStateWriteMaskMask_Internal = UINT64_C(0x0000'0000'00F0'0000),   // write mask state mask
    eStateFlags_DepthTestMask_Internal           = UINT64_C(0x0000'0000'000F'0000),   // depth compare state mask
    eStateFlags_PrimitiveTopologyShift_Internal  = 8ull,
    eStateFlags_PrimitiveTopologyMask_Internal   = UINT64_C(0x0000'0000'0000'0F00),
};
using eStateFlags = std::underlying_type_t<eStateFlags_>;

/// eStencilFlags : 16 bit
/// [4 bit: stencil comparison] [4 bit: stencil pass op] [4 bit: stencil depth fail op] [4 bit: stencil fail op]
enum eStencilFlags_ : UInt16
{
    eStencilFlags_None = 0x0000,

    eStencilFlags_StencilFailOp_Keep             = 0x0000,   // 스텐실 테스트 실패 시 현재 스텐실 값 유지
    eStencilFlags_StencilFailOp_Zero             = 0x0001,   // 스텐실 테스트 실패 시 스텐실 값을 0으로 설정
    eStencilFlags_StencilFailOp_Replace          = 0x0002,   // 스텐실 테스트 실패 시 스텐실 값을 참조값으로 설정
    eStencilFlags_StencilFailOp_IncreaseSaturate = 0x0003,   // 스텐실 테스트 실패 시 스텐실 값을 1 증가 (최대값은 0xFF)
    eStencilFlags_StencilFailOp_DecreaseSaturate = 0x0004,   // 스텐실 테스트 실패 시 스텐실 값을 1 감소 (최소값은 0x00)
    eStencilFlags_StencilFailOp_Invert           = 0x0005,   // 스텐실 테스트 실패 시 스텐실 값을 비트 반전
    eStencilFlags_StencilFailOp_Increase         = 0x0006,   // 스텐실 테스트 실패 시 스텐실 값을 1 증가 (0xFF 초과 시 0x00으로 순환)
    eStencilFlags_StencilFailOp_Decrease         = 0x0007,   // 스텐실 테스트 실패 시 스텐실 값을 1 감소 (0x00 미만 시 0xFF로 순환)

    eStencilFlags_DepthFailOp_Keep             = 0x0000,   // 뎁스 테스트 실패 시 현재 스텐실 값 유지
    eStencilFlags_DepthFailOp_Zero             = 0x0010,   // 뎁스 테스트 실패 시 스텐실 값을 0으로 설정
    eStencilFlags_DepthFailOp_Replace          = 0x0020,   // 뎁스 테스트 실패 시 스텐실 값을 참조값으로 설정
    eStencilFlags_DepthFailOp_IncreaseSaturate = 0x0030,   // 뎁스 테스트 실패 시 스텐실 값을 1 증가 (최대값은 0xFF)
    eStencilFlags_DepthFailOp_DecreaseSaturate = 0x0040,   // 뎁스 테스트 실패 시 스텐실 값을 1 감소 (최소값은 0x00)
    eStencilFlags_DepthFailOp_Invert           = 0x0050,   // 뎁스 테스트 실패 시 스텐실 값을 비트 반전
    eStencilFlags_DepthFailOp_Increase         = 0x0060,   // 뎁스 테스트 실패 시 스텐실 값을 1 증가 (0xFF 초과 시 0x00으로 순환)
    eStencilFlags_DepthFailOp_Decrease         = 0x0070,   // 뎁스 테스트 실패 시 스텐실 값을 1 감소 (0x00 미만 시 0xFF로 순환)

    eStencilFlags_PassOp_Keep             = 0x0000,   // 뎁스 & 스텐실 테스트 모두 통과 시 현재 스텐실 값 유지
    eStencilFlags_PassOp_Zero             = 0x0100,   // 뎁스 & 스텐실 테스트 모두 통과 시 스텐실 값을 0으로 설정
    eStencilFlags_PassOp_Replace          = 0x0200,   // 뎁스 & 스텐실 테스트 모두 통과 시 스텐실 값을 참조값으로 설정
    eStencilFlags_PassOp_IncreaseSaturate = 0x0300,   // 뎁스 & 스텐실 테스트 모두 통과 시 스텐실 값을 1 증가 (최대값은 0xFF)
    eStencilFlags_PassOp_DecreaseSaturate = 0x0400,   // 뎁스 & 스텐실 테스트 모두 통과 시 스텐실 값을 1 감소 (최소값은 0x00)
    eStencilFlags_PassOp_Invert           = 0x0500,   // 뎁스 & 스텐실 테스트 모두 통과 시 스텐실 값을 비트 반전
    eStencilFlags_PassOp_Increase         = 0x0600,   // 뎁스 & 스텐실 테스트 모두 통과 시 스텐실 값을 1 증가 (0xFF 초과 시 0x00으로 순환)
    eStencilFlags_PassOp_Decrease         = 0x0700,   // 뎁스 & 스텐실 테스트 모두 통과 시 스텐실 값을 1 감소 (0x00 미만 시 0xFF로 순환)

    // 스텐실 테스트 함수. 미설정시 스텐실 테스트 비활성화
    eStencilFlags_StencilTest_Less         = 0x1000,   // 스텐실 비교 함수 (적힐 픽셀의 스텐실 값 < 현재 스텐실 버퍼의 스텐실 값. 통과)
    eStencilFlags_StencilTest_LessEqual    = 0x2000,   // 스텐실 비교 함수 (적힐 픽셀의 스텐실 값 <= 현재 스텐실 버퍼의 스텐실 값. 통과)
    eStencilFlags_StencilTest_Greater      = 0x3000,   // 스텐실 비교 함수 (적힐 픽셀의 스텐실 값 > 현재 스텐실 버퍼의 스텐실 값. 통과)
    eStencilFlags_StencilTest_GreaterEqual = 0x4000,   // 스텐실 비교 함수 (적힐 픽셀의 스텐실 값 >= 현재 스텐실 버퍼의 스텐실 값. 통과)
    eStencilFlags_StencilTest_Equal        = 0x5000,   // 스텐실 비교 함수 (적힐 픽셀의 스텐실 값 == 현재 스텐실 버퍼의 스텐실 값. 통과)
    eStencilFlags_StencilTest_NotEqual     = 0x6000,   // 스텐실 비교 함수 (적힐 픽셀의 스텐실 값 != 현재 스텐실 버퍼의 스텐실 값. 통과)
    eStencilFlags_StencilTest_Always       = 0x7000,   // 스텐실 비교 함수 (항상 통과)
    eStencilFlags_StencilTest_Never        = 0x8000,   // 스텐실 비교 함수 (절대 통과하지 않음)

    eStencilFlags_StencilTestMask_Internal = 0xF000,   // 스텐실 비교 함수 마스크
};
using eStencilFlags = std::underlying_type_t<eStencilFlags_>;

/// eSamplerFlags : 32 bit
/// [4 bit: compare func] [12 bit: addressW] [12 bit: addressV] [12 bit: addressU] [8 bit: filter]
enum eSamplerFlags_ : UInt32
{
    eSamplerFlags_None = 0x0000'0000,

    eSamplerFlags_MinPoint  = 0b0000'0000'0000'0000,   // 축소 필터링: 포인트 샘플링
    eSamplerFlags_MinLinear = 0b0000'0000'0000'0001,   // 축소 필터링: 선형 필터링

    eSamplerFlags_MagPoint  = 0b0000'0000'0000'0000,   // 확대 필터링: 포인트 샘플링
    eSamplerFlags_MagLinear = 0b0000'0000'0000'0100,   // 확대 필터링: 선형 필터링

    eSamplerFlags_MipPoint  = 0b0000'0000'0000'0000,   // MIP 필터링: 포인트 샘플링
    eSamplerFlags_MipLinear = 0b0000'0000'0001'0000,   // MIP 필터링: 선형 필터링

    eSamplerFlags_Anisotropic4  = 0x0000'0100,   // 이방성 필터링 (Min, Mag, Mip 필터링 설정 무시)
    eSamplerFlags_Anisotropic8  = 0x0000'0200,   // 이방성 필터링 (Min, Mag, Mip 필터링 설정 무시)
    eSamplerFlags_Anisotropic16 = 0x0000'0300,   // 이방성 필터링 (Min, Mag, Mip 필터링 설정 무시)

    eSamplerFlags_AddressU_Wrap       = 0x0000'0000,   // U 좌표 래핑 모드: 반복
    eSamplerFlags_AddressU_Mirror     = 0x0001'0000,   // U 좌표 래핑 모드: 미러링
    eSamplerFlags_AddressU_Clamp      = 0x0002'0000,   // U 좌표 래핑 모드: 클램핑
    eSamplerFlags_AddressU_Border     = 0x0003'0000,   // U 좌표 래핑 모드: 경계색
    eSamplerFlags_AddressU_MirrorOnce = 0x0004'0000,   // U 좌표 래핑 모드: 미러링(한 번만)

    eSamplerFlags_AddressV_Wrap       = 0x0000'0000,   // V 좌표 래핑 모드: 반복
    eSamplerFlags_AddressV_Mirror     = 0x0010'0000,   // V 좌표 래핑 모드: 미러링
    eSamplerFlags_AddressV_Clamp      = 0x0020'0000,   // V 좌표 래핑 모드: 클램핑
    eSamplerFlags_AddressV_Border     = 0x0030'0000,   // V 좌표 래핑 모드: 경계색
    eSamplerFlags_AddressV_MirrorOnce = 0x0040'0000,   // V 좌표 래핑 모드: 미러링(한 번만)

    eSamplerFlags_AddressW_Wrap       = 0x0000'0000,   // W 좌표 래핑 모드: 반복
    eSamplerFlags_AddressW_Mirror     = 0x0100'0000,   // W 좌표 래핑 모드: 미러링
    eSamplerFlags_AddressW_Clamp      = 0x0200'0000,   // W 좌표 래핑 모드: 클램핑
    eSamplerFlags_AddressW_Border     = 0x0300'0000,   // W 좌표 래핑 모드: 경계색
    eSamplerFlags_AddressW_MirrorOnce = 0x0400'0000,   // W 좌표 래핑 모드: 미러링(한 번만)

    // 비교 샘플링 (미설정시 일반 샘플링)
    eSamplerFlags_Compare_Less       = 0x1000'0000,   // 비교 샘플링 비교 함수 (적힐 픽셀의 뎁스 값 < 현재 뎁스 버퍼의 뎁스 값. 통과)
    eSamplerFlags_Compare_LessEqual  = 0x2000'0000,   // 비교 샘플링 비교 함수 (적힐 픽셀의 뎁스 값 <= 현재 뎁스 버퍼의 뎁스 값. 통과)
    eSamplerFlags_Compare_Greater    = 0x3000'0000,   // 비교 샘플링 비교 함수 (적힐 픽셀의 뎁스 값 > 현재 뎁스 버퍼의 뎁스 값. 통과)
    eSamplerFlags_Compare_GreatEqual = 0x4000'0000,   // 비교 샘플링 비교 함수 (적힐 픽셀의 뎁스 값 >= 현재 뎁스 버퍼의 뎁스 값. 통과)
    eSamplerFlags_Compare_Equal      = 0x5000'0000,   // 비교 샘플링 비교 함수 (적힐 픽셀의 뎁스 값 == 현재 뎁스 버퍼의 뎁스 값. 통과)
    eSamplerFlags_Compare_NotEqual   = 0x6000'0000,   // 비교 샘플링 비교 함수 (적힐 픽셀의 뎁스 값 != 현재 뎁스 버퍼의 뎁스 값. 통과)
    eSamplerFlags_Compare_Always     = 0x7000'0000,   // 비교 샘플링 비교 함수 (항상 통과)
    eSamplerFlags_Compare_Never      = 0x8000'0000,   // 비교 샘플링 비교 함수 (절대 통과하지 않음)

    // [internal]
    eSamplerFlags_AnisotropicMask_Internal = 0x0000'0F00,
    eSamplerFlags_FilterMask_Internal      = 0x000F'FFFF,

    eSamplerFlags_CompareMask_Internal  = 0xF000'0000,
    eSamplerFlags_CompareShift_Internal = 28ull,

    eSamplerFlags_AddressUMask_Internal  = 0x000F'0000,
    eSamplerFlags_AddressVMask_Internal  = 0x00F0'0000,
    eSamplerFlags_AddressWMask_Internal  = 0x0F00'0000,
    eSamplerFlags_AddressUShift_Internal = 16ull,
    eSamplerFlags_AddressVShift_Internal = 20ull,
    eSamplerFlags_AddressWShift_Internal = 24ull,
};
using eSamplerFlags = std::underlying_type_t<eSamplerFlags_>;

enum class eCubeFace : Int8
{
    Right = 0,
    Left,
    Top,
    Bottom,
    Front,
    Back,
};

enum class eDetailLevel : Int8
{
    Low = 0,
    Medium,
    High,
    Ultra,
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

    eShaderCompileFlags_Default = 0
#ifdef JAM_DEBUG
                                | eShaderCompileFlags_Debug
                                | eShaderCompileFlags_WarningsAsErrors
#else
                                | eShaderCompileFlags_Optimization
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
    RGBA()
    {
    }

    /* implicit */ RGBA(const UInt32 _color)
        : color(_color)
    {
    }

    RGBA(const UInt8 _r, const UInt8 _g, const UInt8 _b, const UInt8 _a = 255)
        : r(_r)
        , g(_g)
        , b(_b)
        , a(_a)
    {
    }

    RGBA(const float _r, const float _g, const float _b, const float _a = 1.0f)
        : r(static_cast<UInt8>(Clamp(_r, 0.f, 1.f) * 255.f))
        , g(static_cast<UInt8>(Clamp(_g, 0.f, 1.f) * 255.f))
        , b(static_cast<UInt8>(Clamp(_b, 0.f, 1.f) * 255.f))
        , a(static_cast<UInt8>(Clamp(_a, 0.f, 1.f) * 255.f))
    {
    }

    JAM_NODISCARD bool operator==(const RGBA& _other) const
    {
        return color == _other.color;
    }

    JAM_NODISCARD bool operator!=(const RGBA& _other) const
    {
        return color != _other.color;
    }

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

///////////////////////////////////
///          UTILITIES          ///
///////////////////////////////////

/// 결과 색상 = (적힐 픽셀 색상 * 적힐 픽셀 블렌드 계수) op (배경 픽셀 색상 * 배경 픽셀 블렌드 계수)
JAM_NODISCARD eStateFlags MakeBlendState(
    eBlend   _srcBlend,               // 적힐 픽셀의 블렌드 계수
    eBlendOp _op,                     // 블렌드 연산자
    eBlend   _dstBlend,               // 배경 픽셀의 블렌드 계수
    Int32    _renderTargetIndex = 0   // 렌더 타겟 인덱스 (0 ~ 2, 3 이상은 RT0와 동일하게 적용됨)
);

JAM_NODISCARD Int32 GetPixelByte(eFormat _format);
JAM_NODISCARD bool  IsDepthFormat(eFormat _format);
JAM_NODISCARD Int32 GetIndexFormatByte(eIndexFormat _format);
JAM_NODISCARD Int32 CalcMipsCount(Int32 _width, Int32 _height, Int32 _depth);
JAM_NODISCARD std::tuple<Int32, Int32, Int32> CalcMipSize(Int32 _width, Int32 _height, Int32 _depth, Int32 _mipIndex);
JAM_NODISCARD Int32                           CalcTextureIndex(Int32 _mipIndex, Int32 _arrayIndex, Int32 _mipsCount);

}   // namespace jam