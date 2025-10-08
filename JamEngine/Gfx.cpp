#include "pch.h"

#include "Gfx.h"

#include "Application.h"
#include "BaseUtilities.h"

#include <d3d11shader.h>

#include "Event.h"
#include "Hash.h"
#include "ImageImporter.h"
#include "Memory.h"
#include "Mesh.h"
#include "ResourcePool.h"
#include "ShaderCompiler.h"
#include "Window.h"

#include <DirectXTex.h>
#include <d3dcompiler.h>
using namespace jam;

/////////////////////////////////
///          PRIVATE          ///
/////////////////////////////////

namespace
{

///////////////////////////////////
///          DATA TYPE          ///
///////////////////////////////////

using RasterizerStateCacheKey = UInt16;   // [4 bit: unused] [12 bit: rasterizer state]
using BlendStateCacheKey      = UInt64;   // [12 bit: RT2] [12 bit: RT1] [12 bit: RT0] [4 bit: blend misc] [4 bit: write mask] [20 bit: unused]
using DepthStencilCacheKey    = UInt64;   // [24 bit: unused] [8 bit: depth state] [16 bit: front face stencil] [16 bit: back face stencil]
using SamplerStateCacheKey    = UInt64;   // [32 bit: RGBA for border] [32 bit: sampler state]
using PrimitiveTopologyBind   = UInt8;

///////////////////////////////
///          ENUMS          ///
///////////////////////////////

enum ePipelineDirtyFlags_ : UInt32
{
    ePipelineDirtyFlags_None = 0,

    ePipelineDirtyFlags_PrimitiveTopology = 1 << 0,
    ePipelineDirtyFlags_VertexBuffer      = 1 << 1,
    ePipelineDirtyFlags_IndexBuffer       = 1 << 2,

    ePipelineDirtyFlags_VSConstantBuffer = 1 << 3,
    ePipelineDirtyFlags_PSConstantBuffer = 1 << 4,
    ePipelineDirtyFlags_GSConstantBuffer = 1 << 5,
    ePipelineDirtyFlags_HSConstantBuffer = 1 << 6,
    ePipelineDirtyFlags_DSConstantBuffer = 1 << 7,
    ePipelineDirtyFlags_CSConstantBuffer = 1 << 8,

    ePipelineDirtyFlags_VSShaderResourceView = 1 << 9,
    ePipelineDirtyFlags_PSShaderResourceView = 1 << 10,
    ePipelineDirtyFlags_GSShaderResourceView = 1 << 11,
    ePipelineDirtyFlags_HSShaderResourceView = 1 << 12,
    ePipelineDirtyFlags_DSShaderResourceView = 1 << 13,
    ePipelineDirtyFlags_CSShaderResourceView = 1 << 14,

    ePipelineDirtyFlags_VSSamplerState = 1 << 15,
    ePipelineDirtyFlags_PSSamplerState = 1 << 16,
    ePipelineDirtyFlags_GSSamplerState = 1 << 17,
    ePipelineDirtyFlags_HSSamplerState = 1 << 18,
    ePipelineDirtyFlags_DSSamplerState = 1 << 19,
    ePipelineDirtyFlags_CSSamplerState = 1 << 20,

    ePipelineDirtyFlags_RasterizerState   = 1 << 21,
    ePipelineDirtyFlags_BlendState        = 1 << 22,
    ePipelineDirtyFlags_DepthStencilState = 1 << 23,
    ePipelineDirtyFlags_Viewport          = 1 << 24,
    ePipelineDirtyFlags_FrameBuffer       = 1 << 25,

    ePipelineDirtyFlags_Program = 1 << 26,

    ePipelineDirtyFlags_InputLayout = 1 << 27,

    ePipelineDirtyFlags_ConstantBufferMask = ePipelineDirtyFlags_VSConstantBuffer
                                           | ePipelineDirtyFlags_PSConstantBuffer
                                           | ePipelineDirtyFlags_GSConstantBuffer
                                           | ePipelineDirtyFlags_HSConstantBuffer
                                           | ePipelineDirtyFlags_DSConstantBuffer
                                           | ePipelineDirtyFlags_CSConstantBuffer,

    ePipelineDirtyFlags_ShaderResourceViewMask = ePipelineDirtyFlags_VSShaderResourceView
                                               | ePipelineDirtyFlags_PSShaderResourceView
                                               | ePipelineDirtyFlags_GSShaderResourceView
                                               | ePipelineDirtyFlags_HSShaderResourceView
                                               | ePipelineDirtyFlags_DSShaderResourceView
                                               | ePipelineDirtyFlags_CSShaderResourceView,

    ePipelineDirtyFlags_SamplerStateMask = ePipelineDirtyFlags_VSSamplerState
                                         | ePipelineDirtyFlags_PSSamplerState
                                         | ePipelineDirtyFlags_GSSamplerState
                                         | ePipelineDirtyFlags_HSSamplerState
                                         | ePipelineDirtyFlags_DSSamplerState
                                         | ePipelineDirtyFlags_CSSamplerState,
};
using ePipelineDirtyFlags = unsigned int;

enum class eBufferType
{
    VertexBuffer = 0,
    IndexBuffer,
    ConstantBuffer
};

///////////////////////////////////////
///          LOOK UP TABLE          ///
///////////////////////////////////////

struct FormatInfo
{
    DXGI_FORMAT texFormat  = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT srvFormat  = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT dsvFormat  = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT srgbFormat = DXGI_FORMAT_UNKNOWN;
};

constexpr FormatInfo k_formatInfoTable[EnumCount<eFormat>()] = {
    // eFormat::Unknown
    {            DXGI_FORMAT_UNKNOWN,                  DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R8_UNorm
    {           DXGI_FORMAT_R8_UNORM,                 DXGI_FORMAT_R8_UNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R8_SNorm
    {           DXGI_FORMAT_R8_SNORM,                 DXGI_FORMAT_R8_SNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R8_UInt
    {            DXGI_FORMAT_R8_UINT,                  DXGI_FORMAT_R8_UINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R8_Int
    {            DXGI_FORMAT_R8_SINT,                  DXGI_FORMAT_R8_SINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_UNorm
    {          DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_R16_UNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_SNorm
    {          DXGI_FORMAT_R16_SNORM,                DXGI_FORMAT_R16_SNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_UInt
    {           DXGI_FORMAT_R16_UINT,                 DXGI_FORMAT_R16_UINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_Int
    {           DXGI_FORMAT_R16_SINT,                 DXGI_FORMAT_R16_SINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_Float
    {          DXGI_FORMAT_R16_FLOAT,                DXGI_FORMAT_R16_FLOAT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R32_UInt
    {           DXGI_FORMAT_R32_UINT,                 DXGI_FORMAT_R32_UINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R32_Int
    {           DXGI_FORMAT_R32_SINT,                 DXGI_FORMAT_R32_SINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::R32_Float
    {          DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_R32_FLOAT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG8_UNorm
    {         DXGI_FORMAT_R8G8_UNORM,               DXGI_FORMAT_R8G8_UNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG8_SNorm
    {         DXGI_FORMAT_R8G8_SNORM,               DXGI_FORMAT_R8G8_SNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG8_UInt
    {          DXGI_FORMAT_R8G8_UINT,                DXGI_FORMAT_R8G8_UINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG8_Int
    {          DXGI_FORMAT_R8G8_SINT,                DXGI_FORMAT_R8G8_SINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_UNorm
    {       DXGI_FORMAT_R16G16_UNORM,             DXGI_FORMAT_R16G16_UNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_SNorm
    {       DXGI_FORMAT_R16G16_SNORM,             DXGI_FORMAT_R16G16_SNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_UInt
    {        DXGI_FORMAT_R16G16_UINT,              DXGI_FORMAT_R16G16_UINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_Int
    {        DXGI_FORMAT_R16G16_SINT,              DXGI_FORMAT_R16G16_SINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_Float
    {       DXGI_FORMAT_R16G16_FLOAT,             DXGI_FORMAT_R16G16_FLOAT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG32_UInt
    {        DXGI_FORMAT_R32G32_UINT,              DXGI_FORMAT_R32G32_UINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG32_Int
    {        DXGI_FORMAT_R32G32_SINT,              DXGI_FORMAT_R32G32_SINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RG32_Float
    {       DXGI_FORMAT_R32G32_FLOAT,             DXGI_FORMAT_R32G32_FLOAT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA8_UNorm
    {     DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM,              DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },

    // eFormat::RGBA8_SNorm
    {     DXGI_FORMAT_R8G8B8A8_SNORM,           DXGI_FORMAT_R8G8B8A8_SNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA8_UInt
    {      DXGI_FORMAT_R8G8B8A8_UINT,            DXGI_FORMAT_R8G8B8A8_UINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA8_Int
    {      DXGI_FORMAT_R8G8B8A8_SINT,            DXGI_FORMAT_R8G8B8A8_SINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_UNorm
    { DXGI_FORMAT_R16G16B16A16_UNORM,       DXGI_FORMAT_R16G16B16A16_UNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_SNorm
    { DXGI_FORMAT_R16G16B16A16_SNORM,       DXGI_FORMAT_R16G16B16A16_SNORM,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_UInt
    {  DXGI_FORMAT_R16G16B16A16_UINT,        DXGI_FORMAT_R16G16B16A16_UINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_Int
    {  DXGI_FORMAT_R16G16B16A16_SINT,        DXGI_FORMAT_R16G16B16A16_SINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_Float
    { DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA32_UInt
    {  DXGI_FORMAT_R32G32B32A32_UINT,        DXGI_FORMAT_R32G32B32A32_UINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA32_Int
    {  DXGI_FORMAT_R32G32B32A32_SINT,        DXGI_FORMAT_R32G32B32A32_SINT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA32_Float
    { DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN },

    // eFormat::D16_UNorm
    {       DXGI_FORMAT_R16_TYPELESS,                DXGI_FORMAT_R16_UNORM,            DXGI_FORMAT_D16_UNORM,             DXGI_FORMAT_UNKNOWN },

    // eFormat::D24_UNorm_S8_UInt
    {     DXGI_FORMAT_R24G8_TYPELESS,    DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_D24_UNORM_S8_UINT,             DXGI_FORMAT_UNKNOWN },

    // eFormat::D32_Float
    {       DXGI_FORMAT_R32_TYPELESS,                DXGI_FORMAT_R32_FLOAT,            DXGI_FORMAT_D32_FLOAT,             DXGI_FORMAT_UNKNOWN },

    // eFormat::D32_Float_S8_UInt
    {  DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT,             DXGI_FORMAT_UNKNOWN },
};

constexpr D3D11_BLEND k_d3d11BlendTable[EnumCount<eBlend>()] = {
    D3D11_BLEND_ZERO,              // eBlend_Zero
    D3D11_BLEND_ONE,               // eBlend_One
    D3D11_BLEND_SRC_COLOR,         // eBlend_SrcColor
    D3D11_BLEND_INV_SRC_COLOR,     // eBlend_InvSrcColor
    D3D11_BLEND_SRC_ALPHA,         // eBlend_SrcAlpha
    D3D11_BLEND_INV_SRC_ALPHA,     // eBlend_InvSrcAlpha
    D3D11_BLEND_DEST_ALPHA,        // eBlend_DestAlpha
    D3D11_BLEND_INV_DEST_ALPHA,    // eBlend_InvDestAlpha
    D3D11_BLEND_DEST_COLOR,        // eBlend_DestColor
    D3D11_BLEND_INV_DEST_COLOR,    // eBlend_InvDestColor
    D3D11_BLEND_SRC_ALPHA_SAT,     // eBlend_SrcAlphaSaturate
    D3D11_BLEND_BLEND_FACTOR,      // eBlend_BlendFactor
    D3D11_BLEND_INV_BLEND_FACTOR   // eBlend_InvBlendFactor
};

constexpr D3D11_BLEND_OP k_d3d11BlendOpTable[EnumCount<eBlendOp>()] = {
    D3D11_BLEND_OP_ADD,            // eBlendOp_Add
    D3D11_BLEND_OP_SUBTRACT,       // eBlendOp_Subtract
    D3D11_BLEND_OP_REV_SUBTRACT,   // eBlendOp_RevSubtract
    D3D11_BLEND_OP_MIN,            // eBlendOp_Min
    D3D11_BLEND_OP_MAX             // eBlendOp_Max
};

constexpr D3D11_COMPARISON_FUNC k_d3d11ComparisonFuncTable[EnumCount<D3D11_COMPARISON_FUNC>() + 1] = {
    static_cast<D3D11_COMPARISON_FUNC>(0),   // 더미 값
    D3D11_COMPARISON_LESS,
    D3D11_COMPARISON_LESS_EQUAL,
    D3D11_COMPARISON_GREATER,
    D3D11_COMPARISON_GREATER_EQUAL,
    D3D11_COMPARISON_EQUAL,
    D3D11_COMPARISON_NOT_EQUAL,
    D3D11_COMPARISON_ALWAYS,
    D3D11_COMPARISON_NEVER
};

constexpr D3D11_STENCIL_OP k_d3d11StencilOpTable[EnumCount<D3D11_STENCIL_OP>()] = {
    D3D11_STENCIL_OP_KEEP,
    D3D11_STENCIL_OP_ZERO,
    D3D11_STENCIL_OP_REPLACE,
    D3D11_STENCIL_OP_INCR_SAT,
    D3D11_STENCIL_OP_DECR_SAT,
    D3D11_STENCIL_OP_INVERT,
    D3D11_STENCIL_OP_INCR,
    D3D11_STENCIL_OP_DECR
};

constexpr D3D11_TEXTURE_ADDRESS_MODE k_d3d11TextureAddrModeTable[EnumCount<D3D11_TEXTURE_ADDRESS_MODE>()] = {
    D3D11_TEXTURE_ADDRESS_WRAP,
    D3D11_TEXTURE_ADDRESS_MIRROR,
    D3D11_TEXTURE_ADDRESS_CLAMP,
    D3D11_TEXTURE_ADDRESS_BORDER,
    D3D11_TEXTURE_ADDRESS_MIRROR_ONCE
};

constexpr Int32 k_jamSupportedPromitiveTopologyCount =
    ((eStateFlags_PrimitiveTopology_PointList & eStateFlags_PrimitiveTopologyMask_Internal) >> eStateFlags_PrimitiveTopologyShift_Internal) + 1;

constexpr D3D11_PRIMITIVE_TOPOLOGY k_d3d11PrimitiveTopologyTable[k_jamSupportedPromitiveTopologyCount] = {
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,    // eStateFlags_PrimitiveTopology_TriangleList
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,   // eStateFlags_PrimitiveTopology_TriangleStrip
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST,        // eStateFlags_PrimitiveTopology_LineList
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,       // eStateFlags_PrimitiveTopology_LineStrip
    D3D11_PRIMITIVE_TOPOLOGY_POINTLIST        // eStateFlags_PrimitiveTopology_PointList
};

constexpr D3D11_TEXTURECUBE_FACE k_d3d11CubeFaceTable[EnumCount<eCubeFace>()] = {
    D3D11_TEXTURECUBE_FACE_POSITIVE_X,
    D3D11_TEXTURECUBE_FACE_NEGATIVE_X,
    D3D11_TEXTURECUBE_FACE_POSITIVE_Y,
    D3D11_TEXTURECUBE_FACE_NEGATIVE_Y,
    D3D11_TEXTURECUBE_FACE_POSITIVE_Z,
    D3D11_TEXTURECUBE_FACE_NEGATIVE_Z
};

constexpr UINT k_d3d11BufferbindFlagsTable[EnumCount<eBufferType>()] = {
    D3D11_BIND_VERTEX_BUFFER,    // eVertexType::VertexBuffer
    D3D11_BIND_INDEX_BUFFER,     // eVertexType::IndexBuffer
    D3D11_BIND_CONSTANT_BUFFER   // eVertexType::ConstantBuffer
};

constexpr DXGI_FORMAT k_dxgiFormatAttributeTable[EnumCount<eAttributeFormat>()] = {
    DXGI_FORMAT_R32_FLOAT,            // eAttributeFormat::Float
    DXGI_FORMAT_R32G32_FLOAT,         // eAttributeFormat::Float2
    DXGI_FORMAT_R32G32B32_FLOAT,      // eAttributeFormat::Float3
    DXGI_FORMAT_R32G32B32A32_FLOAT,   // eAttributeFormat::Float4
    DXGI_FORMAT_R8G8B8A8_UNORM,       // eAttributeFormat::UNorm8x4
    DXGI_FORMAT_R8G8B8A8_UINT,        // eAttributeFormat::UInt8x4
};

constexpr const char* const k_hlslFormatTable[EnumCount<eAttributeFormat>()] = {
    "float",    // eAttributeFormat::Float
    "float2",   // eAttributeFormat::Float2
    "float3",   // eAttributeFormat::Float3
    "float4",   // eAttributeFormat::Float4
    "float4",   // eAttributeFormat::UNorm8x4
    "uint4"     // eAttributeFormat::UInt8x4
};

static_assert(EnumCount<eAttributeFormat>() == 6, "eAttributeFormat vertexCount mismatch");

////////////////////////////////
///          STRUCT          ///
////////////////////////////////

struct ConstantBufferBinds
{
    ConstantBufferHandle cbufferHandles[k_constantBufferCountMax] = {};

    Int32 dirtyStart = k_constantBufferCountMax;
    Int32 dirtyEnd   = 0;
};

struct TextureBinds
{
    TextureHandle textureHandles[k_shaderResourceCountMax] = {};

    Int32 dirtyStart = k_shaderResourceCountMax;
    Int32 dirtyEnd   = 0;
};

struct SamplerFlagsBinds
{
    eSamplerFlags samplerFlags[k_samplerStateCountMax];
    RGBA          borderColors[k_samplerStateCountMax];

    Int32 dirtyStart = k_samplerStateCountMax;
    Int32 dirtyEnd   = 0;
};

/////////////////////////////////
///          CONTEXT          ///
/////////////////////////////////

struct GfxContext
{
    ////////// PIPELINE //////////

    void ApplyConstantBuffers(eShader _shader);
    void ApplyShaderResourceViews(eShader _shader);
    void ApplySamplerStates(eShader _shader);
    void ApplyPipeline();

    ////////// EVENT HANDLERS //////////

    void OnResize(Int32 _width, Int32 _height);
    void OnWindowResizeEvent(const WindowResizeEvent& _event);
    void OnWindowMaxizeEvent(const WindowMaximizeEvent& _event);

    ////////// NATIVE RESOURCE FACTORIES //////////

    JAM_NODISCARD TextureRef CreateNativeTexture(
        Int32                              _width,
        Int32                              _height,
        Int32                              _depth,
        eFormat                            _format,
        Int32                              _arraySize,
        eTextureCreateFlags                _flags,
        eTextureType                       _type,
        const std::span<const MemoryView>& _initData) const;

    JAM_NODISCARD Buffer CreateNativeBuffer(
        Int32                            _byteWidth,
        eBufferType                      _type,
        const std::optional<MemoryView>& _initData) const;

    JAM_NODISCARD FrameBuffer CreateNativeFrameBuffer(
        std::span<const TextureHandle> _handles,
        bool                           _bDestoryTexture);

    ////////// DATA UPLOAD & DOWNLOAD //////////

    void UploadBuffer(
        const Buffer&     _buffer,
        const MemoryView& _uploadData,
        Int32             _offset,
        bool              _bDiscard) const;

    JAM_NODISCARD std::vector<UInt8> DownloadBuffer(
        const Buffer& _buffer) const;

    void UploadTexture(
        const TextureRef& _texture,
        const MemoryView& _uploadTexData,
        Int32             _uploadTexWidth,
        Int32             _uploadTexHeight,
        Int32             _uploadTexDepth,
        Int32             _mipIndex,
        Int32             _arrayIndex,
        Int32             _xOffset,
        Int32             _yOffset,
        Int32             _zOffset) const;

    JAM_NODISCARD std::vector<Byte> DownloadTexture(
        const TextureRef& _texture,
        Int32             _mipIndex,
        Int32             _arrayIndex) const;

    void Blit(
        const TextureRef& _srcTexture,
        const TextureRef& _dstTexture,
        Int32             _blitWidthOrZero,
        Int32             _blitHeightOrZero,
        Int32             _blitDepthOrZero,
        Int32             _srcMipIndex,
        Int32             _srcArrayIndex,
        Int32             _srcX,
        Int32             _srcY,
        Int32             _srcZ,
        Int32             _dstMipIndex,
        Int32             _dstArrayIndex,
        Int32             _dstX,
        Int32             _dstY,
        Int32             _dstZ) const;

    ////////// CACHE ///////////

    JAM_NODISCARD ID3D11RasterizerState*   GetOrCreateD3D11RasterizerState(RasterizerStateCacheKey _key);
    JAM_NODISCARD ID3D11BlendState*        GetOrCreateD3D11BlendState(BlendStateCacheKey _key);
    JAM_NODISCARD ID3D11DepthStencilState* GetOrCreateD3D11DepthStencilState(DepthStencilCacheKey _key);
    JAM_NODISCARD ID3D11SamplerState*      GetOrCreateD3D11SamplerState(SamplerStateCacheKey _key);
    JAM_NODISCARD ShaderHandle             GetOrCreateNativeShader(const MemoryView& _shaderByteCode);
    JAM_NODISCARD VertexLayoutHandle       GetOrCreateNativeVertexLayout(const VertexLayout& _vertexLayout);

    ////////// MISC //////////

    /// 포매팅된 native resource name 생성
    JAM_NODISCARD std::string CreateNativeResourceName(
        std::string_view _tag,
        AnyHandle        _handle,
        std::string_view _name = "") const;

    /// debug name 설정
    void SetD3D11Name(
        ID3D11DeviceChild* _pDeviceChild,
        std::string_view   _name) const;

    /// 샘플 카운트에 대해 지원되는 최대 멀티샘플 품질 수준 쿼리
    JAM_NODISCARD UInt32 QueryMaxMultisampleQuality(
        DXGI_FORMAT _format,
        Int32       _sampleCount) const;

    JAM_NODISCARD eFormat GetNativeFormatFromDXGIFormat(
        DXGI_FORMAT _dxgiFormat) const;

    ////////// D3D DEVICE //////////

    ID3D11Device*        pd3d11Device;
    ID3D11DeviceContext* pd3d11DeviceContext;
    IDXGISwapChain1*     pdxgiSwapchain;

    ////////// RESOURCE POOL //////////

    ResourcePool<VertexBuffer> vertexBuffers;
    ResourcePool<IndexBuffer>  indexBuffers;
    ResourcePool<TextureRef>   textures;
    ResourcePool<FrameBuffer>  frameBuffers;

    ResourcePool<ShaderRef>       shaders;
    ResourcePool<VertexLayoutRef> vertexLayouts;
    ResourcePool<ConstantBuffer>  cbuffers;
    ResourcePool<Program>         programs;

    ////////// HANDLE CACHE //////////

    HashMap<ShaderCacheKey, ShaderHandle>             shaderCache;
    HashMap<VertexLayoutCacheKey, VertexLayoutHandle> vertexLayoutCache;

    ////////// STATE CACHE //////////

    HashMap<RasterizerStateCacheKey, ID3D11RasterizerState*> rasterizerStateCache;
    HashMap<BlendStateCacheKey, ID3D11BlendState*>           blendStateCache;
    HashMap<DepthStencilCacheKey, ID3D11DepthStencilState*>  depthStencilStateCache;
    HashMap<SamplerStateCacheKey, ID3D11SamplerState*>       samplerStateCache;

    ////////// PIPELINE BIND //////////

    VertexLayoutHandle vertexLayoutHandleBind = {};
    VertexBufferHandle vertexBufferHandleBind = {};
    Int32              vertexBufferOffsetBind = 0;
    Int32              vertexCountBind        = 0;

    IndexBufferHandle indexBufferHandleBind = {};
    Int32             indexBufferOffsetBind = 0;
    Int32             indexCountBind        = 0;

    FrameBufferHandle frameBufferHandleBind = {};
    ProgramHandle     programHandleBind     = {};

    ConstantBufferBinds cbufferHandleBinds[EnumCount<eShader>()] = {};
    TextureBinds        textureHandleBinds[EnumCount<eShader>()] = {};
    SamplerFlagsBinds   samplerFlagsBinds[EnumCount<eShader>()]  = {};

    Viewport viewportBind = {};

    eStateFlags stateFlagsBind  = eStateFlags_None;
    RGBA        blendFactorBind = {};

    eStencilFlags fstencilFlagsBind = 0;
    eStencilFlags bstencilFlagsBind = 0;
    UInt8         stencilRefBind    = 0;

    PrimitiveTopologyBind   primitiveTopologyBind = 0;
    DepthStencilCacheKey    depthStencilStateBind = 0;
    BlendStateCacheKey      blendStateBind        = 0;
    RasterizerStateCacheKey rasterizerStateBind   = 0;

    ePipelineDirtyFlags pipelineDirtyFlags = ePipelineDirtyFlags_None;

    ////////// INTERNAL RESOURCE //////////

    TextureHandle     backBufferTextureHandle;
    FrameBufferHandle backBufferFrameBufferHandle;
    Mesh              fullScreenMesh;

};

GfxContext g_ctx = {};

void GfxContext::ApplyConstantBuffers(const eShader _shader)
{

    static ID3D11Buffer* s_binder[k_constantBufferCountMax];
    ConstantBufferBinds& binds = cbufferHandleBinds[EnumToInt(_shader)];

    Int32 dirtyStart = binds.dirtyStart;
    Int32 dirtyCount = binds.dirtyStart <= binds.dirtyEnd ? binds.dirtyEnd - binds.dirtyStart + 1 : 0;

    for (Int32 index = 0; index < dirtyCount; ++index)
    {
        Handle<ConstantBuffer> handle = binds.cbufferHandles[dirtyStart + index];
        s_binder[index]               = handle == k_invalidHandle ? nullptr : cbuffers[handle].buffer.pBuffer;
    }

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            pd3d11DeviceContext->VSSetConstantBuffers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::PixelShader:
            pd3d11DeviceContext->PSSetConstantBuffers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::GeometryShader:
            pd3d11DeviceContext->GSSetConstantBuffers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::ComputeShader:
            pd3d11DeviceContext->CSSetConstantBuffers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::HullShader:
            pd3d11DeviceContext->HSSetConstantBuffers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::DomainShader:
            pd3d11DeviceContext->DSSetConstantBuffers(dirtyStart, dirtyCount, s_binder);
            break;
    }

    // reset dirty range
    binds.dirtyStart = k_constantBufferCountMax;
    binds.dirtyEnd   = 0;
}

void GfxContext::ApplyShaderResourceViews(
    const eShader _shader)
{
    static ID3D11ShaderResourceView* s_binder[k_shaderResourceCountMax];
    TextureBinds&                    bindRef = textureHandleBinds[EnumToInt(_shader)];

    Int32 dirtyStart = bindRef.dirtyStart;
    Int32 dirtyCount = (bindRef.dirtyStart <= bindRef.dirtyEnd) ? bindRef.dirtyEnd - bindRef.dirtyStart + 1 : 0;

    for (Int32 i = 0; i < dirtyCount; ++i)
    {
        Handle<TextureRef> handle = bindRef.textureHandles[dirtyStart + i];
        s_binder[i]               = handle == k_invalidHandle ? nullptr : textures[handle].pSRV;
    }

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            pd3d11DeviceContext->VSSetShaderResources(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::PixelShader:
            pd3d11DeviceContext->PSSetShaderResources(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::GeometryShader:
            pd3d11DeviceContext->GSSetShaderResources(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::HullShader:
            pd3d11DeviceContext->HSSetShaderResources(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::DomainShader:
            pd3d11DeviceContext->DSSetShaderResources(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::ComputeShader:
            pd3d11DeviceContext->CSSetShaderResources(dirtyStart, dirtyCount, s_binder);
            break;
    }

    // reset dirty range
    bindRef.dirtyStart = k_shaderResourceCountMax;
    bindRef.dirtyEnd   = 0;
}

void GfxContext::ApplySamplerStates(const eShader _shader)
{
    static ID3D11SamplerState* s_binder[k_samplerStateCountMax];
    SamplerFlagsBinds&         bindRef = samplerFlagsBinds[EnumToInt(_shader)];

    Int32 dirtyStart = bindRef.dirtyStart;
    Int32 dirtyCount = (bindRef.dirtyStart <= bindRef.dirtyEnd) ? bindRef.dirtyEnd - bindRef.dirtyStart + 1 : 0;

    for (Int32 i = 0; i < dirtyCount; ++i)
    {
        SamplerStateCacheKey key = (bindRef.samplerFlags[dirtyStart + i]) | (bindRef.borderColors[dirtyStart + i].color << 32);
        s_binder[i]              = g_ctx.GetOrCreateD3D11SamplerState(key);
    }

    switch (_shader)
    {
        case eShader::VertexShader:
            g_ctx.pd3d11DeviceContext->VSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::PixelShader:
            g_ctx.pd3d11DeviceContext->PSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::GeometryShader:
            g_ctx.pd3d11DeviceContext->GSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::HullShader:
            g_ctx.pd3d11DeviceContext->HSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::DomainShader:
            g_ctx.pd3d11DeviceContext->DSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::ComputeShader:
            g_ctx.pd3d11DeviceContext->CSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
    }

    // reset dirty range
    bindRef.dirtyStart = k_samplerStateCountMax;
    bindRef.dirtyEnd   = 0;
}

void GfxContext::ApplyPipeline()
{
    // apply pipeline state if dirty
    if (pipelineDirtyFlags != ePipelineDirtyFlags_None)
    {
        constexpr ID3D11Buffer* k_nullBuffer = nullptr;

        // vertex buffer.pBuffer
        if (pipelineDirtyFlags & ePipelineDirtyFlags_VertexBuffer)
        {
            if (vertexBufferHandleBind == k_invalidHandle)
            {
                constexpr UINT k_stride = 0;
                constexpr UINT k_offset = 0;
                pd3d11DeviceContext->IASetVertexBuffers(0, 1, &k_nullBuffer, &k_stride, &k_offset);
            }
            else
            {
                const VertexBuffer& vertexBuffer = vertexBuffers[vertexBufferHandleBind];
                const UINT          stride       = vertexBuffer.vertexStride;
                const UINT          offset       = g_ctx.vertexBufferOffsetBind * stride;
                pd3d11DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer.buffer.pBuffer, &stride, &offset);
            }
        }

        // m_handle buffer.pBuffer
        if (pipelineDirtyFlags & ePipelineDirtyFlags_IndexBuffer)
        {
            if (indexBufferHandleBind == k_invalidHandle)
            {
                pd3d11DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
            }
            else
            {
                const IndexBuffer& indexBuffer = indexBuffers[indexBufferHandleBind];
                pd3d11DeviceContext->IASetIndexBuffer(
                    indexBuffer.buffer.pBuffer,
                    indexBuffer.format == eIndexFormat::U16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
                    0);
            }
        }

        // topology
        if (pipelineDirtyFlags & ePipelineDirtyFlags_PrimitiveTopology)
        {
            pd3d11DeviceContext->IASetPrimitiveTopology(k_d3d11PrimitiveTopologyTable[primitiveTopologyBind]);
        }

        // input layout
        if (pipelineDirtyFlags & ePipelineDirtyFlags_InputLayout)
        {
            const VertexLayoutRef& layout = vertexLayouts[vertexLayoutHandleBind];
            pd3d11DeviceContext->IASetInputLayout(layout.pInputLayout);
        }

        // shader program
        if (pipelineDirtyFlags & ePipelineDirtyFlags_Program)
        {
            if (programHandleBind == k_invalidHandle)
            {
                pd3d11DeviceContext->VSSetShader(nullptr, nullptr, 0);
                pd3d11DeviceContext->PSSetShader(nullptr, nullptr, 0);
                pd3d11DeviceContext->GSSetShader(nullptr, nullptr, 0);
                pd3d11DeviceContext->HSSetShader(nullptr, nullptr, 0);
                pd3d11DeviceContext->DSSetShader(nullptr, nullptr, 0);
                pd3d11DeviceContext->IASetInputLayout(nullptr);
            }
            else
            {
                const Program& program = programs[programHandleBind];
                for (Int32 i = 0; i < k_renderingPipelineShaderStageCount; ++i)
                {
                    ShaderHandle handle = program.shaderHandles[i];
                    eShader      stage  = static_cast<eShader>(i);
                    if (handle == k_invalidHandle)
                    {
                        switch (stage)
                        {
                            case eShader::VertexShader: pd3d11DeviceContext->VSSetShader(nullptr, nullptr, 0); break;
                            case eShader::PixelShader: pd3d11DeviceContext->PSSetShader(nullptr, nullptr, 0); break;
                            case eShader::GeometryShader: pd3d11DeviceContext->GSSetShader(nullptr, nullptr, 0); break;
                            case eShader::HullShader: pd3d11DeviceContext->HSSetShader(nullptr, nullptr, 0); break;
                            case eShader::DomainShader: pd3d11DeviceContext->DSSetShader(nullptr, nullptr, 0); break;
                        }
                    }
                    else
                    {
                        const ShaderRef& shader = shaders[handle];
                        switch (stage)
                        {
                            case eShader::VertexShader: pd3d11DeviceContext->VSSetShader(shader.pVertexShader, nullptr, 0); break;
                            case eShader::PixelShader: pd3d11DeviceContext->PSSetShader(shader.pPixelShader, nullptr, 0); break;
                            case eShader::GeometryShader: pd3d11DeviceContext->GSSetShader(shader.pGeometryShader, nullptr, 0); break;
                            case eShader::HullShader: pd3d11DeviceContext->HSSetShader(shader.pHullShader, nullptr, 0); break;
                            case eShader::DomainShader: pd3d11DeviceContext->DSSetShader(shader.pDomainShader, nullptr, 0); break;
                        }
                    }
                }
            }
        }

        // constant buffer.pBuffer
        if (pipelineDirtyFlags & ePipelineDirtyFlags_ConstantBufferMask)
        {
            if (pipelineDirtyFlags & ePipelineDirtyFlags_VSConstantBuffer) ApplyConstantBuffers(eShader::VertexShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_PSConstantBuffer) ApplyConstantBuffers(eShader::PixelShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_GSConstantBuffer) ApplyConstantBuffers(eShader::GeometryShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_HSConstantBuffer) ApplyConstantBuffers(eShader::HullShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_DSConstantBuffer) ApplyConstantBuffers(eShader::DomainShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_CSConstantBuffer) ApplyConstantBuffers(eShader::ComputeShader);
        }

        // shader resource view
        if (pipelineDirtyFlags & ePipelineDirtyFlags_ShaderResourceViewMask)
        {
            if (pipelineDirtyFlags & ePipelineDirtyFlags_VSShaderResourceView) ApplyShaderResourceViews(eShader::VertexShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_PSShaderResourceView) ApplyShaderResourceViews(eShader::PixelShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_GSShaderResourceView) ApplyShaderResourceViews(eShader::GeometryShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_HSShaderResourceView) ApplyShaderResourceViews(eShader::HullShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_DSShaderResourceView) ApplyShaderResourceViews(eShader::DomainShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_CSShaderResourceView) ApplyShaderResourceViews(eShader::ComputeShader);
        }

        // sampler state
        if (pipelineDirtyFlags & ePipelineDirtyFlags_SamplerStateMask)
        {
            if (pipelineDirtyFlags & ePipelineDirtyFlags_VSSamplerState) ApplySamplerStates(eShader::VertexShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_PSSamplerState) ApplySamplerStates(eShader::PixelShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_GSSamplerState) ApplySamplerStates(eShader::GeometryShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_HSSamplerState) ApplySamplerStates(eShader::HullShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_DSSamplerState) ApplySamplerStates(eShader::DomainShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_CSSamplerState) ApplySamplerStates(eShader::ComputeShader);
        }

        // blend state
        if (pipelineDirtyFlags & ePipelineDirtyFlags_BlendState)
        {
            ID3D11BlendState* pBlendState    = GetOrCreateD3D11BlendState(blendStateBind);
            FLOAT             blendFactor[4] = {
                static_cast<float>(blendFactorBind.r) / 255.f,
                static_cast<float>(blendFactorBind.g) / 255.f,
                static_cast<float>(blendFactorBind.b) / 255.f,
                static_cast<float>(blendFactorBind.a) / 255.f
            };
            pd3d11DeviceContext->OMSetBlendState(pBlendState, blendFactor, 0xFFFF'FFFF);
        }

        // depth stencil state
        if (pipelineDirtyFlags & ePipelineDirtyFlags_DepthStencilState)
        {
            ID3D11DepthStencilState* pDepthStencilState = GetOrCreateD3D11DepthStencilState(depthStencilStateBind);
            pd3d11DeviceContext->OMSetDepthStencilState(pDepthStencilState, stencilRefBind);
        }

        // rasterizer state
        if (pipelineDirtyFlags & ePipelineDirtyFlags_RasterizerState)
        {
            ID3D11RasterizerState* pRasterizerState = GetOrCreateD3D11RasterizerState(rasterizerStateBind);
            pd3d11DeviceContext->RSSetState(pRasterizerState);
        }

        // frame buffer
        if (pipelineDirtyFlags & ePipelineDirtyFlags_FrameBuffer)
        {
            if (frameBufferHandleBind == k_invalidHandle)
            {
                pd3d11DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
            }
            else
            {
                const FrameBuffer& fb = frameBuffers[frameBufferHandleBind];
                pd3d11DeviceContext->OMSetRenderTargets(fb.renderTargetCount, fb.pRTV, fb.pDSV);
            }
        }

        // viewport
        if (pipelineDirtyFlags & ePipelineDirtyFlags_Viewport)
        {
            D3D11_VIEWPORT vp {
                .TopLeftX = static_cast<FLOAT>(viewportBind.x),
                .TopLeftY = static_cast<FLOAT>(viewportBind.y),
                .Width    = static_cast<FLOAT>(viewportBind.w),
                .Height   = static_cast<FLOAT>(viewportBind.h),
                .MinDepth = 0.f,
                .MaxDepth = 1.f
            };

            pd3d11DeviceContext->RSSetViewports(1, &vp);
        }

        // clear dirty flags
        pipelineDirtyFlags = ePipelineDirtyFlags_None;
    }
}

void GfxContext::OnResize(const Int32 _width, const Int32 _height)
{
    // 백버퍼 리사이즈
    if (pdxgiSwapchain)
    {
        // 리사이즈전 백버퍼에 의존하는 리소스 해제
        JAM_DESTROY_FRAME_BUFFER(backBufferFrameBufferHandle);

#ifdef JAM_DEBUG
        // 백버퍼에 대한 참조가 0인지 확인
        ID3D11Resource* pBBResource;
        JAM_CHECK_COM(pdxgiSwapchain->GetBuffer(0, IID_PPV_ARGS(&pBBResource)));
        UINT refCount = pBBResource->Release();   // ref count를 관측해서 0인지 확인
        JAM_ASSERT(refCount == 0, "Back buffer.pBuffer reference vertexCount is not zero after cleanup. RefCount: {}", refCount);
#endif

        // 스왑체인 리사이즈
        JAM_CHECK_COM(pdxgiSwapchain->ResizeBuffers(0, _width, _height, DXGI_FORMAT_UNKNOWN, 0));

        // 백버퍼에 대한 텍스처 생성
        {
            // 렌더타겟 텍스처
            ID3D11Resource* pBackBufferRes = nullptr;
            JAM_CHECK_COM(pdxgiSwapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBufferRes)));

            DXGI_SWAP_CHAIN_DESC1 desc;
            JAM_CHECK_COM(pdxgiSwapchain->GetDesc1(&desc));

            TextureRef bbTexture;
            bbTexture.width     = NarrowCast<Int32>(desc.Width);
            bbTexture.height    = NarrowCast<Int32>(desc.Height);
            bbTexture.depth     = 1;
            bbTexture.type      = eTextureType::Texture2D;
            bbTexture.format    = eFormat::RGBA8_UNorm;
            bbTexture.mipsCount = 1;
            bbTexture.arraySize = 1;
            bbTexture.samples   = 1;
            bbTexture.bMutable  = false;
            bbTexture.refCount  = 1;
            bbTexture.bManaged  = false;
            bbTexture.pResource = pBackBufferRes;

            // 텍스처 풀에 등록
            TextureHandle texHandle = textures.Alloc();
            textures[texHandle]     = std::move(bbTexture);
            gfx::SetTextureName(texHandle);

            backBufferTextureHandle     = texHandle;
            backBufferFrameBufferHandle = gfx::CreateFrameBuffer(texHandle);
        }
    }
}

void GfxContext::OnWindowResizeEvent(const WindowResizeEvent& _event)
{
    OnResize(_event.GetWidth(), _event.GetHeight());
}

void GfxContext::OnWindowMaxizeEvent(const WindowMaximizeEvent& _event)
{
    OnResize(_event.GetWidth(), _event.GetHeight());
}

TextureRef GfxContext::CreateNativeTexture(
    const Int32                        _width,
    const Int32                        _height,
    const Int32                        _depth,
    const eFormat                      _format,
    const Int32                        _arraySize,
    const eTextureCreateFlags          _flags,
    const eTextureType                 _type,
    const std::span<const MemoryView>& _initData) const
{
    bool bGenMips       = (_flags & eTextureCreateFlags_GenerateMips) != 0;
    bool bSRGB          = (_flags & eTextureCreateFlags_SRGB) != 0;
    bool bIsMSAA        = (_flags & (eTextureCreateFlags_MultiSampled_Internal)) != 0;
    bool bWriteOnly     = (_flags & eTextureCreateFlags_WriteOnly) != 0;
    bool bRTVAttachable = (_flags & eTextureCreateFlags_RenderTarget) != 0;
    bool bDSVAttachable = (_flags & eTextureCreateFlags_DepthStencil) != 0;
    bool bMutable       = _initData.empty();
    bool bNeedInit      = !_initData.empty();
    bool bIsCubemap     = _type == eTextureType::TextureCube;

    const Int32 mipsCount = bGenMips ? CalcMipsCount(_width, _height, _depth) : 1;
    const Int32 samples   = _flags & eTextureCreateFlags_MSAAx4 != 0  ? 4
                          : _flags & eTextureCreateFlags_MSAAx8 != 0  ? 8
                          : _flags & eTextureCreateFlags_MSAAx16 != 0 ? 16
                                                                      : 1;

    // 검증
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "Texture dimensions must be greater than zero.");
    JAM_ASSERT(_arraySize > 0, "Texture array size must be greater than zero.");
    JAM_ASSERT(!(bIsMSAA && bGenMips), "Multisampled texture cannot have mipmaps.");

    // 포맷 검증 (dsv 쓰기전용 텍스처는 최적화된 포맷을 사용)
    const FormatInfo& formatInfo       = k_formatInfoTable[EnumToInt(_format)];
    const bool        bWriteOnlyDSVTex = bWriteOnly && bDSVAttachable && !bRTVAttachable;
    const DXGI_FORMAT dxgiFormat       = bWriteOnlyDSVTex ? formatInfo.dsvFormat
                                       : bSRGB            ? formatInfo.srgbFormat
                                                          : formatInfo.texFormat;

    // native 객체
    TextureRef texture;
    texture.width          = _width;
    texture.height         = _height;
    texture.depth          = _depth;
    texture.type           = _type;
    texture.format         = _format;
    texture.arraySize      = _arraySize;
    texture.samples        = samples;
    texture.mipsCount      = mipsCount;
    texture.bMutable       = bMutable;
    texture.refCount       = 1;
    texture.bManaged       = false;   // 소유권을 가진 쪽에서 해제해야함.
    texture.bRTVAttachable = bRTVAttachable;
    texture.bDSVAttachable = bDSVAttachable;

    switch (_type)
    {
        case eTextureType::Texture2D:
        case eTextureType::TextureCube:
        {
            D3D11_TEXTURE2D_DESC desc;
            desc.Width          = texture.width;
            desc.Height         = texture.height;
            desc.MipLevels      = texture.mipsCount;
            desc.ArraySize      = _arraySize;
            desc.Format         = dxgiFormat;
            desc.Usage          = bMutable ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
            desc.CPUAccessFlags = 0;

            // MSAA 설정
            desc.SampleDesc.Count   = samples;
            desc.SampleDesc.Quality = bIsMSAA ? QueryMaxMultisampleQuality(desc.Format, desc.SampleDesc.Count)
                                              : 0;

            // bind flags 설정
            desc.BindFlags = 0;
            if (!bWriteOnly)
                desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

            if (bRTVAttachable)
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;

            if (bDSVAttachable)
                desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

            // 기타 설정
            desc.MiscFlags = 0;
            if (bIsCubemap)
                desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

            if (bGenMips)
                desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

            if (bNeedInit)
            {
                JAM_CHECK_COM(pd3d11Device->CreateTexture2D(&desc, nullptr, &texture.pTexture2D));
            }
            else
            {
                JAM_ASSERT(_initData.size() == _arraySize * mipsCount, "Texture data count mismatch");

                std::vector<D3D11_SUBRESOURCE_DATA> initData;
                initData.reserve(_arraySize * mipsCount);
                for (Int32 arrayIdx = 0; arrayIdx < _arraySize; arrayIdx++)
                {
                    for (Int32 mipIdx = 0; mipIdx < mipsCount; mipIdx++)
                    {
                        auto [width, height, _]    = CalcMipSize(_width, _height, 0, mipIdx);
                        Int32             texIndex = CalcTextureIndex(mipIdx, arrayIdx, mipsCount);
                        const MemoryView& mem      = _initData[texIndex];

                        UINT rowPitch = width * GetPixelByte(_format);
                        JAM_ASSERT(mem.GetByteWidth() == rowPitch * height, "Texture data size mismatch");
                        initData.emplace_back(mem.GetDataPtr(), rowPitch, 0);
                    }
                }

                JAM_CHECK_COM(pd3d11Device->CreateTexture2D(&desc, initData.data(), &texture.pTexture2D));
            }
        }
        break;

        case eTextureType::Texture3D:
        {
            JAM_ASSERT((_flags & eTextureCreateFlags_MultiSampled_Internal) == 0, "3D texture cannot be multisampled");
            JAM_ASSERT(_arraySize == 1, "3D texture array size must be 1");
            JAM_ASSERT(!bIsCubemap, "3D texture cannot be a cubemap");
            JAM_ASSERT(!bDSVAttachable, "3D texture cannot be a Depth/Stencil view.");

            D3D11_TEXTURE3D_DESC desc;
            desc.Width          = texture.width;
            desc.Height         = texture.height;
            desc.Depth          = texture.depth;
            desc.MipLevels      = texture.mipsCount;
            desc.Format         = dxgiFormat;
            desc.Usage          = bMutable ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
            desc.CPUAccessFlags = 0;

            // bind flags 설정
            desc.BindFlags = 0;
            if (!bWriteOnly)
                desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            if (bRTVAttachable)
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            if (bDSVAttachable)
                desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

            // 기타 설정
            desc.MiscFlags = 0;
            if (bGenMips)
                desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

            if (bNeedInit)
            {
                JAM_ASSERT(_initData.size() == mipsCount, "Texture data count mismatch");

                std::vector<D3D11_SUBRESOURCE_DATA> initData;
                initData.reserve(mipsCount);

                for (Int32 mipIdx = 0; mipIdx < mipsCount; mipIdx++)
                {
                    auto [width, height, depth] = CalcMipSize(_width, _height, _depth, mipIdx);
                    const MemoryView& mem       = _initData[mipIdx];

                    UINT rowPitch   = width * GetPixelByte(_format);
                    UINT slicePitch = rowPitch * height;
                    JAM_ASSERT(mem.GetByteWidth() == slicePitch * depth, "Texture data size mismatch");
                    initData.emplace_back(mem.GetDataPtr(), rowPitch, slicePitch);
                }

                JAM_CHECK_COM(pd3d11Device->CreateTexture3D(&desc, initData.data(), &texture.pTexture3D));
            }
            else
            {
                JAM_CHECK_COM(pd3d11Device->CreateTexture3D(&desc, nullptr, &texture.pTexture3D));
            }
        }
        break;

        default:
            JAM_CRASH("Unsupported texture stage: {}", EnumToInt(_type));
    }

    // SRV 생성
    if (!bWriteOnly)
    {
        bool bIsArray = _arraySize > 1;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc {};
        srvDesc.Format = k_formatInfoTable[EnumToInt(_format)].srvFormat;

        switch (_type)
        {
            case eTextureType::Texture2D:
            case eTextureType::TextureCube:
            {
                if (bIsCubemap)
                {
                    if (_arraySize > 6)
                    {
                        srvDesc.ViewDimension                     = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                        srvDesc.TextureCubeArray.MostDetailedMip  = 0;
                        srvDesc.TextureCubeArray.MipLevels        = mipsCount;
                        srvDesc.TextureCubeArray.First2DArrayFace = 0;
                        srvDesc.TextureCubeArray.NumCubes         = _arraySize / 6;
                    }
                    else
                    {
                        srvDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
                        srvDesc.TextureCube.MostDetailedMip = 0;
                        srvDesc.TextureCube.MipLevels       = mipsCount;
                    }
                }
                else
                {
                    if (bIsArray)
                    {
                        if (bIsMSAA)
                        {
                            srvDesc.ViewDimension                    = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                            srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                            srvDesc.Texture2DMSArray.ArraySize       = _arraySize;
                        }
                        else
                        {
                            srvDesc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                            srvDesc.Texture2DArray.MostDetailedMip = 0;
                            srvDesc.Texture2DArray.MipLevels       = mipsCount;
                            srvDesc.Texture2DArray.FirstArraySlice = 0;
                            srvDesc.Texture2DArray.ArraySize       = _arraySize;
                        }
                    }
                    else
                    {
                        if (bIsMSAA)
                        {
                            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                        }
                        else
                        {
                            srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
                            srvDesc.Texture2D.MostDetailedMip = 0;
                            srvDesc.Texture2D.MipLevels       = mipsCount;
                        }
                    }
                }
            }
            break;

            case eTextureType::Texture3D:
            {
                srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MostDetailedMip = 0;
                srvDesc.Texture3D.MipLevels       = mipsCount;
            }
            break;
        }

        JAM_CHECK_COM(pd3d11Device->CreateShaderResourceView(texture.pResource, &srvDesc, &texture.pSRV));
    }

    return texture;
}

Buffer GfxContext::CreateNativeBuffer(
    const Int32                      _byteWidth,
    const eBufferType                _type,
    const std::optional<MemoryView>& _initData) const
{
    bool bDynamic   = (_initData == std::nullopt);
    bool bIsCbuffer = (_type == eBufferType::ConstantBuffer);
    JAM_ASSERT(_byteWidth > 0, "Buffer size must be greater than zero.");
    if (bIsCbuffer)
    {
        JAM_ASSERT(_byteWidth % 16 == 0, "Constant buffer size must be multiple of 16 bytes.");
        JAM_ASSERT(bDynamic, "Constant buffer must be dynamic.");
    }

    Buffer buffer;
    buffer.byteWidth = _byteWidth;
    buffer.bDynamic  = bDynamic;

    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = _byteWidth;
    desc.CPUAccessFlags      = bDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.StructureByteStride = 0;
    desc.Usage               = bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
    desc.BindFlags           = k_d3d11BufferbindFlagsTable[EnumToInt(_type)];
    desc.MiscFlags           = 0;

    if (_initData)
    {
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = _initData->GetDataPtr();
        JAM_CHECK_COM(pd3d11Device->CreateBuffer(&desc, &initData, &buffer.pBuffer));
    }
    else
    {
        JAM_CHECK_COM(pd3d11Device->CreateBuffer(&desc, nullptr, &buffer.pBuffer));
    }

    return buffer;
}

FrameBuffer GfxContext::CreateNativeFrameBuffer(
    std::span<const TextureHandle> _handles,
    const bool                     _bDestoryTexture)
{
    FrameBuffer frameBuffer;
    bool        bHasDepth = false;
    Int32       rtCount   = 0;

    for (const TextureHandle handle: _handles)
    {
        JAM_ASSERT(handle != k_invalidHandle, "Invalid texture handle in render target textures.");

        TextureRef& texture = textures[handle];
        JAM_ASSERT(texture.pResource, "Texture resource is null.");
        JAM_ASSERT(texture.bManaged == false, "Cannot attach a texture that is managed by a framebuffer.");

        // 텍스처 참조 관리
        if (_bDestoryTexture)
        {
            JAM_ASSERT(texture.refCount == 1, "Cannot transfer ownership of a texture that is referenced elsewhere.");
            texture.bManaged = true;   // 프레임버퍼가 소유권을 가짐.
        }
        else
        {
            ++texture.refCount;   // 참조 증가
        }

        // 렌더 타겟 / 뎁스 스텐실 객체 생성
        bool bIsArray   = texture.arraySize > 1;
        bool bIsMSAA    = texture.samples > 1;
        bool bIsCubemap = texture.type == eTextureType::TextureCube;

        if (IsDepthFormat(texture.format))
        {
            JAM_ASSERT(bHasDepth == false, "Only one depth stencil texture can be attached to a framebuffer.");
            JAM_ASSERT(texture.bDSVAttachable, "Depth stencil texture must be created with Depth/Stencil flag.");
            JAM_ASSERT(bIsCubemap == false, "Cubemap depth stencil view is not supported.");
            JAM_ASSERT(texture.type != eTextureType::Texture3D, "3D texture depth stencil view is not supported.");

            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc {};
            dsvDesc.Format = k_formatInfoTable[EnumToInt(texture.format)].dsvFormat;

            switch (texture.type)
            {
                case eTextureType::Texture2D:
                case eTextureType::TextureCube:
                {

                    if (bIsArray)
                    {
                        if (bIsMSAA)
                        {
                            dsvDesc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                            dsvDesc.Texture2DMSArray.FirstArraySlice = 0;
                            dsvDesc.Texture2DMSArray.ArraySize       = texture.arraySize;
                        }
                        else
                        {
                            dsvDesc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                            dsvDesc.Texture2DArray.ArraySize       = texture.arraySize;
                            dsvDesc.Texture2DArray.FirstArraySlice = 0;
                            dsvDesc.Texture2DArray.MipSlice        = 0;
                        }
                    }
                    else
                    {
                        if (bIsMSAA)
                        {
                            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                        }
                        else
                        {
                            dsvDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
                            dsvDesc.Texture2D.MipSlice = 0;
                        }
                    }
                }
                break;
            }

            JAM_CHECK_COM(pd3d11Device->CreateDepthStencilView(texture.pResource, &dsvDesc, &frameBuffer.pDSV));
            frameBuffer.depthStencilTextureHandle = handle;
            bHasDepth                             = true;
        }
        else   // 렌더 타겟
        {
            JAM_ASSERT(rtCount < k_renderTargetCountMax, "Exceeded maximum render target count.");
            JAM_ASSERT(texture.bRTVAttachable, "Render target texture must be created with RenderTarget flag.");
            JAM_ASSERT(bIsCubemap == false, "Cubemap render target view is not supported.");

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc {};
            rtvDesc.Format = k_formatInfoTable[EnumToInt(texture.type)].srvFormat;

            switch (texture.type)
            {
                case eTextureType::Texture2D:
                case eTextureType::TextureCube:
                {

                    if (bIsArray)
                    {
                        if (bIsMSAA)
                        {
                            rtvDesc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                            rtvDesc.Texture2DMSArray.FirstArraySlice = 0;
                            rtvDesc.Texture2DMSArray.ArraySize       = texture.arraySize;
                        }
                        else
                        {
                            rtvDesc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                            rtvDesc.Texture2DArray.ArraySize       = texture.arraySize;
                            rtvDesc.Texture2DArray.FirstArraySlice = 0;
                            rtvDesc.Texture2DArray.MipSlice        = 0;
                        }
                    }
                    else
                    {
                        if (bIsMSAA)
                        {
                            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                        }
                        else
                        {
                            rtvDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
                            rtvDesc.Texture2D.MipSlice = 0;
                        }
                    }
                }
                break;

                case eTextureType::Texture3D:
                {
                    rtvDesc.ViewDimension         = D3D11_RTV_DIMENSION_TEXTURE3D;
                    rtvDesc.Texture3D.MipSlice    = 0;
                    rtvDesc.Texture3D.FirstWSlice = 0;
                    rtvDesc.Texture3D.WSize       = -1;
                }
                break;
            }

            JAM_CHECK_COM(pd3d11Device->CreateRenderTargetView(texture.pResource, &rtvDesc, &frameBuffer.pRTV[rtCount]));
            frameBuffer.renderTargetTextureHandles[rtCount] = handle;
            ++rtCount;
        }
    }

    frameBuffer.renderTargetCount = rtCount;
    return frameBuffer;
}

void GfxContext::UploadBuffer(
    const Buffer&     _buffer,
    const MemoryView& _uploadData,
    const Int32       _offset,
    const bool        _bDiscard) const
{
    JAM_ASSERT(_buffer.bDynamic, "Only dynamic buffer can be updated");

    const Byte* pWriteData     = _uploadData.GetDataPtr();
    UInt64      writeByteWidth = Min(_uploadData.GetByteWidth(), _buffer.byteWidth - _offset);

    if (_bDiscard)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        JAM_CHECK_COM(pd3d11DeviceContext->Map(_buffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
        std::memcpy(static_cast<Byte*>(mappedResource.pData) + _offset, pWriteData, writeByteWidth);
        pd3d11DeviceContext->Unmap(_buffer.pBuffer, 0);
    }
    else
    {
        // 해당 코드는 ConstantBuffer에서 오류가 발생함
        // ConstantBuffer는 CopySubresourceRegion로 복사가 안됨.
        // https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-copysubresourceregion

        ComPtr<ID3D11Buffer> pStagingBuffer;

        D3D11_BUFFER_DESC desc;
        _buffer.pBuffer->GetDesc(&desc);
        desc.Usage          = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.BindFlags      = 0;
        desc.MiscFlags      = 0;
        desc.ByteWidth      = writeByteWidth;

        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem          = pWriteData;
        initData.SysMemPitch      = 0;
        initData.SysMemSlicePitch = 0;

        JAM_CHECK_COM(pd3d11Device->CreateBuffer(&desc, &initData, pStagingBuffer.ReleaseAndGetAddressOf()));

        D3D11_BOX box;
        box.left   = _offset;
        box.right  = _offset + writeByteWidth;
        box.top    = 0;
        box.bottom = 1;
        box.front  = 0;
        box.back   = 1;

        pd3d11DeviceContext->CopySubresourceRegion(_buffer.pBuffer, 0, _offset, 0, 0, pStagingBuffer.Get(), 0, &box);
    }
}

std::vector<UInt8> GfxContext::DownloadBuffer(
    const Buffer& _buffer) const
{
    ComPtr<ID3D11Buffer> pStagingBuffer;
    std::vector<UInt8>   readData(_buffer.byteWidth);

    // create staging buffer
    {
        D3D11_BUFFER_DESC desc;
        _buffer.pBuffer->GetDesc(&desc);
        desc.Usage          = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.BindFlags      = 0;
        desc.MiscFlags      = 0;

        JAM_CHECK_COM(pd3d11Device->CreateBuffer(&desc, nullptr, pStagingBuffer.ReleaseAndGetAddressOf()));
    }

    // copy to staging buffer
    pd3d11DeviceContext->CopyResource(pStagingBuffer.Get(), _buffer.pBuffer);

    // map staging buffer
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        JAM_CHECK_COM(pd3d11DeviceContext->Map(pStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedResource));
        std::memcpy(readData.data(), mappedResource.pData, _buffer.byteWidth);
        pd3d11DeviceContext->Unmap(pStagingBuffer.Get(), 0);
    }

    return readData;
}

void GfxContext::UploadTexture(
    const TextureRef& _texture,
    const MemoryView& _uploadTexData,
    const Int32       _uploadTexWidth,
    const Int32       _uploadTexHeight,
    const Int32       _uploadTexDepth,
    const Int32       _mipIndex,
    const Int32       _arrayIndex,
    const Int32       _xOffset,
    const Int32       _yOffset,
    const Int32       _zOffset) const
{
    JAM_ASSERT(_texture.bMutable, "Only mutable texture can be updated");
    JAM_ASSERT(_mipIndex < _texture.mipsCount, "Mip m_handle out of range");
    JAM_ASSERT(_arrayIndex < _texture.arraySize, "Array m_handle out of range");

    auto [width, height, depth] = CalcMipSize(_texture.width, _texture.height, _texture.depth, _mipIndex);

    D3D11_BOX box;
    box.right  = Min(_xOffset + _uploadTexWidth, width);
    box.bottom = Min(_yOffset + _uploadTexHeight, height);
    box.back   = Min(_zOffset + _uploadTexDepth, depth);
    box.left   = Min(_xOffset, box.right - 1);
    box.top    = Min(_yOffset, box.bottom - 1);
    box.front  = Min(_zOffset, box.back - 1);

    Int32 pixelByte     = GetPixelByte(_texture.format);
    Int32 texIndex      = CalcTextureIndex(_mipIndex, _arrayIndex, _texture.mipsCount);
    Int32 srcRowPitch   = _uploadTexWidth * pixelByte;
    Int32 srcSlicePitch = srcRowPitch * _uploadTexHeight;

    pd3d11DeviceContext->UpdateSubresource(
        _texture.pResource,
        texIndex,
        &box,
        _uploadTexData.GetDataPtr(),
        srcRowPitch,
        srcSlicePitch);
}

std::vector<Byte> GfxContext::DownloadTexture(
    const TextureRef& _texture,
    const Int32       _mipIndex,
    const Int32       _arrayIndex) const
{
    JAM_ASSERT(_mipIndex >= 0 && _arrayIndex >= 0, "Mip and array index must be non-negative");
    JAM_ASSERT(_mipIndex < _texture.mipsCount, "Mip handle out of range");
    JAM_ASSERT(_arrayIndex < _texture.arraySize, "Array handle out of range");

    auto [width, height, depth] = CalcMipSize(_texture.width, _texture.height, _texture.depth, _mipIndex);
    Int32 rowPitch              = width * GetPixelByte(_texture.format);
    Int32 slicePitch            = rowPitch * height;

    std::vector<Byte> downloadData;
    downloadData.resize(slicePitch * depth);

    switch (_texture.type)
    {
        case eTextureType::Texture2D:
        case eTextureType::TextureCube:
        {
            ComPtr<ID3D11Texture2D> pStagingTexture;

            // create staging texture
            {
                D3D11_TEXTURE2D_DESC desc;
                _texture.pTexture2D->GetDesc(&desc);
                desc.Usage          = D3D11_USAGE_STAGING;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                desc.BindFlags      = 0;
                desc.MiscFlags      = 0;
                desc.MipLevels      = 1;
                desc.ArraySize      = 1;

                JAM_CHECK_COM(pd3d11Device->CreateTexture2D(&desc, nullptr, pStagingTexture.ReleaseAndGetAddressOf()));
            }

            // copy to staging texture
            {
                UINT index = D3D11CalcSubresource(_mipIndex, _arrayIndex, _texture.mipsCount);
                pd3d11DeviceContext->CopySubresourceRegion(pStagingTexture.Get(), 0, 0, 0, 0, _texture.pTexture2D, index, nullptr);
            }

            // map staging texture
            {
                D3D11_MAPPED_SUBRESOURCE mapped;
                JAM_CHECK_COM(pd3d11DeviceContext->Map(pStagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped));

                Byte*       pDstHead = downloadData.data();
                const Byte* pSrcHead = static_cast<const Byte*>(mapped.pData);
                for (Int32 row = 0; row < height; ++row)
                {
                    Byte*       pDst = pDstHead + row * rowPitch;
                    const Byte* pSrc = pSrcHead + row * mapped.RowPitch;
                    std::memcpy(pDst, pSrc, rowPitch);
                }

                pd3d11DeviceContext->Unmap(pStagingTexture.Get(), 0);
            }
        }
        break;

        case eTextureType::Texture3D:
        {
            ComPtr<ID3D11Texture3D> pStagingTexture3D;

            // create staging texture
            {
                D3D11_TEXTURE3D_DESC desc;
                _texture.pTexture3D->GetDesc(&desc);
                desc.Usage          = D3D11_USAGE_STAGING;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
                desc.BindFlags      = 0;
                desc.MiscFlags      = 0;
                desc.MipLevels      = 1;

                JAM_CHECK_COM(pd3d11Device->CreateTexture3D(&desc, nullptr, pStagingTexture3D.ReleaseAndGetAddressOf()));
            }

            // copy to staging texture
            {
                UINT index = D3D11CalcSubresource(_mipIndex, 0, _texture.mipsCount);
                pd3d11DeviceContext->CopySubresourceRegion(pStagingTexture3D.Get(), 0, 0, 0, 0, _texture.pTexture3D, index, nullptr);
            }

            // map staging texture
            {
                D3D11_MAPPED_SUBRESOURCE mapped;
                JAM_CHECK_COM(pd3d11DeviceContext->Map(pStagingTexture3D.Get(), 0, D3D11_MAP_READ, 0, &mapped));

                Byte*       pDstHead = downloadData.data();
                const Byte* pSrcHead = static_cast<const Byte*>(mapped.pData);
                for (Int32 slice = 0; slice < depth; ++slice)
                {
                    for (Int32 row = 0; row < height; ++row)
                    {
                        Byte*       pDst = pDstHead + slice * slicePitch + row * rowPitch;
                        const Byte* pSrc = pSrcHead + slice * mapped.DepthPitch + row * mapped.RowPitch;
                        std::memcpy(pDst, pSrc, rowPitch);
                    }
                }
                pd3d11DeviceContext->Unmap(pStagingTexture3D.Get(), 0);
            }
        }
        break;
    }

    return downloadData;
}

void GfxContext::Blit(
    const TextureRef& _srcTexture,
    const TextureRef& _dstTexture,
    const Int32       _blitWidthOrZero,
    const Int32       _blitHeightOrZero,
    const Int32       _blitDepthOrZero,
    const Int32       _srcMipIndex,
    const Int32       _srcArrayIndex,
    const Int32       _srcX,
    const Int32       _srcY,
    const Int32       _srcZ,
    const Int32       _dstMipIndex,
    const Int32       _dstArrayIndex,
    const Int32       _dstX,
    const Int32       _dstY,
    const Int32       _dstZ) const
{
    JAM_ASSERT(_srcTexture.format == _dstTexture.format, "Source and destination texture formats must match for Blit operation.");
    JAM_ASSERT(_srcTexture.type == _dstTexture.type, "Source and destination texture types must match for Blit operation.");
    JAM_ASSERT(_srcTexture.mipsCount == _dstTexture.mipsCount, "Source and destination texture mip counts must match for Blit operation.");
    JAM_ASSERT(_blitDepthOrZero >= 0 && _blitHeightOrZero >= 0 && _blitWidthOrZero >= 0, "Blit dimensions must be non-negative.");
    JAM_ASSERT(_srcMipIndex < _srcTexture.mipsCount, "Source mip index out of range.");
    JAM_ASSERT(_dstMipIndex < _dstTexture.mipsCount, "Destination mip index out of range.");
    JAM_ASSERT(_srcArrayIndex < _srcTexture.arraySize, "Source array index out of range.");
    JAM_ASSERT(_dstArrayIndex < _dstTexture.arraySize, "Destination array index out of range.");
    JAM_ASSERT(_srcTexture.bMutable, "Source texture must be mutable for Blit operation.");
    JAM_ASSERT(_dstTexture.bMutable, "Destination texture must be mutable for Blit operation.");

    auto [srcWidth, srcHeight, srcDepth] = CalcMipSize(_srcTexture.width, _srcTexture.height, _srcTexture.depth, _srcMipIndex);
    auto [dstWidth, dstHeight, dstDepth] = CalcMipSize(_dstTexture.width, _dstTexture.height, _dstTexture.depth, _dstMipIndex);

    Int32 blitWidth  = _blitWidthOrZero > 0 ? _blitWidthOrZero : srcWidth - _srcX;
    Int32 blitHeight = _blitHeightOrZero > 0 ? _blitHeightOrZero : srcHeight - _srcY;
    Int32 blitDepth  = _blitDepthOrZero > 0 ? _blitDepthOrZero : srcDepth - _srcZ;

    blitWidth  = Min(blitWidth, srcWidth - _srcX, dstWidth - _dstX);
    blitHeight = Min(blitHeight, srcHeight - _srcY, dstHeight - _dstY);
    blitDepth  = Min(blitDepth, srcDepth - _srcZ, dstDepth - _dstZ);

    D3D11_BOX box;
    box.left   = _srcX;
    box.top    = _srcY;
    box.front  = _srcZ;
    box.right  = _srcX + blitWidth;
    box.bottom = _srcY + blitHeight;
    box.back   = _srcZ + blitDepth;
    JAM_ASSERT(box.right > box.left && box.bottom > box.top && box.back > box.front, "Invalid Blit dimensions.");

    UINT srcIndex = CalcTextureIndex(_srcMipIndex, _srcArrayIndex, _srcTexture.mipsCount);
    UINT dstIndex = CalcTextureIndex(_dstMipIndex, _dstArrayIndex, _dstTexture.mipsCount);

    pd3d11DeviceContext->CopySubresourceRegion(
        _dstTexture.pResource,
        dstIndex,
        _dstX,
        _dstY,
        _dstZ,
        _srcTexture.pResource,
        srcIndex,
        &box);
}

ID3D11RasterizerState* GfxContext::GetOrCreateD3D11RasterizerState(const RasterizerStateCacheKey _key)
{
    auto iter = rasterizerStateCache.find(_key);
    if (iter != rasterizerStateCache.end())
    {
        return iter->second;
    }

    D3D11_RASTERIZER_DESC desc;
    desc.FillMode = (_key & eStateFlags_Wireframe) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;

    Int32 cullMode = (_key & 0x0000'0000'0000'00C0);
    desc.CullMode  = cullMode == eStateFlags_CullNone  ? D3D11_CULL_NONE
                   : cullMode == eStateFlags_CullFront ? D3D11_CULL_FRONT
                                                       : D3D11_CULL_BACK;

    desc.FrontCounterClockwise = (_key & eStateFlags_CCWFront) != 0;
    desc.DepthBias             = 0.f;
    desc.DepthBiasClamp        = 0.f;
    desc.SlopeScaledDepthBias  = 0.f;
    desc.DepthClipEnable       = (_key & eStateFlags_DepthClamp) == 0;
    desc.ScissorEnable         = (_key & eStateFlags_Scissor) != 0;
    desc.MultisampleEnable     = (_key & eStateFlags_MultiSample) != 0;
    desc.AntialiasedLineEnable = (_key & eStateFlags_LineAA) != 0;

    JAM_CHECK_COM(pd3d11Device->CreateRasterizerState(&desc, &rasterizerStateCache[_key]));
    return rasterizerStateCache[_key];
}

ID3D11BlendState* GfxContext::GetOrCreateD3D11BlendState(const BlendStateCacheKey _key)
{
    auto iter = blendStateCache.find(_key);
    if (iter != blendStateCache.end())
    {
        return iter->second;
    }

    UInt8 writeMask = (_key & eStateFlags_BlendStateWriteMaskMask_Internal);

    D3D11_BLEND_DESC desc {};
    desc.IndependentBlendEnable = (_key & eStateFlags_IndependentBlend) != 0;
    desc.AlphaToCoverageEnable  = (_key & eStateFlags_AlphaToCoverage) != 0;

    if (desc.IndependentBlendEnable)
    {
        for (Int32 i = 0; i < 3; i++)
        {
            eStateFlags flags    = (_key >> (12ull * i + eStateFlags_BlendStateRT0Shift_Internal)) & 0xFFF;
            eBlend      srcBlend = static_cast<eBlend>(flags & 0x0F);
            eBlendOp    blendOp  = static_cast<eBlendOp>((flags >> 4) & 0x0F);
            eBlend      dstBlend = static_cast<eBlend>((flags >> 8) & 0x0f);

            desc.RenderTarget[i].BlendEnable           = (_key & (eStateFlags_BlendStateRT0Mask_Internal << (i * 12))) != 0;
            desc.RenderTarget[i].SrcBlend              = k_d3d11BlendTable[EnumToInt(srcBlend)];
            desc.RenderTarget[i].DestBlend             = k_d3d11BlendTable[EnumToInt(dstBlend)];
            desc.RenderTarget[i].BlendOp               = k_d3d11BlendOpTable[EnumToInt(blendOp)];
            desc.RenderTarget[i].SrcBlendAlpha         = k_d3d11BlendTable[EnumToInt(srcBlend)];
            desc.RenderTarget[i].DestBlendAlpha        = k_d3d11BlendTable[EnumToInt(dstBlend)];
            desc.RenderTarget[i].BlendOpAlpha          = k_d3d11BlendOpTable[EnumToInt(blendOp)];
            desc.RenderTarget[i].RenderTargetWriteMask = writeMask;
        }

        for (Int32 i = 3; i < 8; i++)
        {
            desc.RenderTarget[i] = desc.RenderTarget[0];
        }
    }
    else
    {
        eStateFlags flags    = (_key >> (eStateFlags_BlendStateRT0Shift_Internal)) & 0xFFF;
        eBlend      srcBlend = static_cast<eBlend>(flags & 0x0F);
        eBlendOp    blendOp  = static_cast<eBlendOp>((flags >> 4) & 0x0F);
        eBlend      dstBlend = static_cast<eBlend>((flags >> 8) & 0x0f);

        desc.RenderTarget[0].BlendEnable           = (_key & eStateFlags_BlendStateRT0Mask_Internal) != 0;
        desc.RenderTarget[0].SrcBlend              = k_d3d11BlendTable[EnumToInt(srcBlend)];
        desc.RenderTarget[0].DestBlend             = k_d3d11BlendTable[EnumToInt(dstBlend)];
        desc.RenderTarget[0].BlendOp               = k_d3d11BlendOpTable[EnumToInt(blendOp)];
        desc.RenderTarget[0].SrcBlendAlpha         = k_d3d11BlendTable[EnumToInt(srcBlend)];
        desc.RenderTarget[0].DestBlendAlpha        = k_d3d11BlendTable[EnumToInt(dstBlend)];
        desc.RenderTarget[0].BlendOpAlpha          = k_d3d11BlendOpTable[EnumToInt(blendOp)];
        desc.RenderTarget[0].RenderTargetWriteMask = writeMask;
    }

    JAM_CHECK_COM(pd3d11Device->CreateBlendState(&desc, &blendStateCache[_key]));
    return blendStateCache[_key];
}

ID3D11DepthStencilState* GfxContext::GetOrCreateD3D11DepthStencilState(const DepthStencilCacheKey _key)
{
    auto iter = depthStencilStateCache.find(_key);
    if (iter != depthStencilStateCache.end())
    {
        return iter->second;
    }

    eStateFlags   depthState = static_cast<eStateFlags>(((_key >> 32) & 0xFF) << eStateFlags_DepthStateShift_Internal);
    eStencilFlags fStencil   = static_cast<eStencilFlags>((_key >> 16) & 0xFFFF);
    eStencilFlags bStencil   = static_cast<eStencilFlags>(_key & 0xFFFF);

    D3D11_DEPTH_STENCIL_DESC desc;
    desc.DepthEnable    = (depthState & eStateFlags_DepthTestMask_Internal) != 0;
    desc.DepthWriteMask = (depthState & eStateFlags_DepthWrite) ? D3D11_DEPTH_WRITE_MASK_ALL
                                                                : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc      = k_d3d11ComparisonFuncTable[(depthState & eStateFlags_DepthTestMask_Internal) >> eStateFlags_DepthStateShift_Internal];
    desc.StencilEnable  = (fStencil & eStencilFlags_StencilTestMask_Internal) != 0
                      || (bStencil & eStencilFlags_StencilTestMask_Internal) != 0;

    desc.StencilReadMask  = 0xFF;
    desc.StencilWriteMask = 0xFF;

    // front face
    desc.FrontFace.StencilFailOp      = k_d3d11StencilOpTable[fStencil & 0xF];
    desc.FrontFace.StencilDepthFailOp = k_d3d11StencilOpTable[(fStencil >> 4) & 0xF];
    desc.FrontFace.StencilPassOp      = k_d3d11StencilOpTable[(fStencil >> 8) & 0xF];
    desc.FrontFace.StencilFunc        = k_d3d11ComparisonFuncTable[(fStencil >> 12) & 0xF];

    // back face
    desc.BackFace.StencilFailOp      = k_d3d11StencilOpTable[bStencil & 0xF];
    desc.BackFace.StencilDepthFailOp = k_d3d11StencilOpTable[(bStencil >> 4) & 0xF];
    desc.BackFace.StencilPassOp      = k_d3d11StencilOpTable[(bStencil >> 8) & 0xF];
    desc.BackFace.StencilFunc        = k_d3d11ComparisonFuncTable[(bStencil >> 12) & 0xF];

    JAM_CHECK_COM(pd3d11Device->CreateDepthStencilState(&desc, &depthStencilStateCache[_key]));
    return depthStencilStateCache[_key];
}

ID3D11SamplerState* GfxContext::GetOrCreateD3D11SamplerState(const SamplerStateCacheKey _key)
{
    auto iter = samplerStateCache.find(_key);
    if (iter != samplerStateCache.end())
    {
        return iter->second;
    }

    eSamplerFlags state          = static_cast<eSamplerFlags>(_key & UINT64_C(0x0000'0000'FFFF'FFFF));
    bool          bIsAnisotropic = (state & eSamplerFlags_AnisotropicMask_Internal) != 0;
    bool          bIsComparison  = (state & eSamplerFlags_CompareMask_Internal) != 0;

    RGBA borderColor;
    borderColor.color = (_key & UINT64_C(0xFFFF'FFFF'0000'0000)) >> 32;

    D3D11_SAMPLER_DESC desc {};
    Int32              filter = state & eSamplerFlags_FilterMask_Internal;
    if (bIsAnisotropic) filter = 0b00101'0101;
    if (bIsComparison) filter |= 0b0100'0000;
    desc.Filter        = static_cast<D3D11_FILTER>(filter);
    desc.AddressU      = k_d3d11TextureAddrModeTable[(state & eSamplerFlags_AddressUMask_Internal) >> eSamplerFlags_AddressUShift_Internal];
    desc.AddressV      = k_d3d11TextureAddrModeTable[(state & eSamplerFlags_AddressVMask_Internal) >> eSamplerFlags_AddressVShift_Internal];
    desc.AddressW      = k_d3d11TextureAddrModeTable[(state & eSamplerFlags_AddressWMask_Internal) >> eSamplerFlags_AddressWShift_Internal];
    desc.MipLODBias    = 0.f;
    desc.MaxAnisotropy = (state & eSamplerFlags_Anisotropic4)  ? 4
                       : (state & eSamplerFlags_Anisotropic8)  ? 8
                       : (state & eSamplerFlags_Anisotropic16) ? 16
                                                               : 1;

    if (bIsComparison)
    {
        desc.ComparisonFunc = k_d3d11ComparisonFuncTable[((state & eSamplerFlags_CompareMask_Internal) >> eSamplerFlags_CompareShift_Internal)];
    }

    desc.BorderColor[0] = static_cast<FLOAT>(borderColor.r) / 255.f;
    desc.BorderColor[1] = static_cast<FLOAT>(borderColor.g) / 255.f;
    desc.BorderColor[2] = static_cast<FLOAT>(borderColor.b) / 255.f;
    desc.BorderColor[3] = static_cast<FLOAT>(borderColor.a) / 255.f;
    desc.MinLOD         = -FLT_MAX;
    desc.MaxLOD         = FLT_MAX;

    JAM_CHECK_COM(pd3d11Device->CreateSamplerState(&desc, &samplerStateCache[_key]));
    return samplerStateCache[_key];
}

ShaderHandle GfxContext::GetOrCreateNativeShader(const MemoryView& _shaderByteCode)
{
    const void* pByteCode = _shaderByteCode.GetDataPtr();
    size_t      byteWidth = _shaderByteCode.GetByteWidth();

    // 해싱
    MurmurHash64 hasher;
    hasher.Begin();
    hasher.Add(pByteCode, byteWidth);
    ShaderCacheKey key = hasher.End();

    // 캐시 리턴
    auto iter = shaderCache.find(key);
    if (iter != shaderCache.end())
    {
        ShaderHandle handle = iter->second;
        ++shaders[handle].refCount;
        return handle;
    }

    // 쉐이더 생성
    ShaderRef shader;
    shader.refCount = 1;
    shader.cacheKey = key;

    // 리플렉션 생성
    D3D11_SHADER_DESC              desc;
    ComPtr<ID3D11ShaderReflection> pReflect = nullptr;
    JAM_CHECK_COM(D3DReflect(pByteCode, byteWidth, IID_PPV_ARGS(pReflect.ReleaseAndGetAddressOf())));
    JAM_CHECK_COM(pReflect->GetDesc(&desc));

    // 쉐이더 객체 생성
    D3D11_SHADER_VERSION_TYPE stage = static_cast<D3D11_SHADER_VERSION_TYPE>(D3D11_SHVER_GET_TYPE(desc.Version));
    switch (stage)
    {
        case D3D11_SHVER_PIXEL_SHADER:
        {
            shader.stage = eShader::PixelShader;
            JAM_CHECK_COM(pd3d11Device->CreatePixelShader(pByteCode, byteWidth, nullptr, &shader.pPixelShader));
        }
        break;

        case D3D11_SHVER_VERTEX_SHADER:
        {
            shader.stage = eShader::VertexShader;
            JAM_CHECK_COM(pd3d11Device->CreateVertexShader(pByteCode, byteWidth, nullptr, &shader.pVertexShader));
        }
        break;

        case D3D11_SHVER_GEOMETRY_SHADER:
        {
            shader.stage = eShader::GeometryShader;
            JAM_CHECK_COM(pd3d11Device->CreateGeometryShader(pByteCode, byteWidth, nullptr, &shader.pGeometryShader));
        }
        break;

        case D3D11_SHVER_HULL_SHADER:
        {
            shader.stage = eShader::HullShader;
            JAM_CHECK_COM(pd3d11Device->CreateHullShader(pByteCode, byteWidth, nullptr, &shader.pHullShader));
        }
        break;

        case D3D11_SHVER_DOMAIN_SHADER:
        {
            shader.stage = eShader::DomainShader;
            JAM_CHECK_COM(pd3d11Device->CreateDomainShader(pByteCode, byteWidth, nullptr, &shader.pDomainShader));
        }
        break;

        case D3D11_SHVER_COMPUTE_SHADER:
        {
            shader.stage = eShader::ComputeShader;
            JAM_CHECK_COM(pd3d11Device->CreateComputeShader(pByteCode, byteWidth, nullptr, &shader.pComputeShader));
        }
        break;

        default:
            JAM_ASSERT(0, "Unsupported shader stage");
    }

    ShaderHandle handle = shaders.Alloc();
    shaders[handle]     = std::move(shader);
    shaderCache[key]    = handle;
    gfx::SetShaderName(handle);
    return handle;
}

VertexLayoutHandle GfxContext::GetOrCreateNativeVertexLayout(
    const VertexLayout& _vertexLayout)
{

    // 해싱
    MurmurHash64 hasher;
    hasher.Begin();
    hasher.Add(&_vertexLayout, sizeof(_vertexLayout));
    VertexLayoutCacheKey hash = hasher.End();

    // 캐시 탐색
    auto iter = vertexLayoutCache.find(hash);
    if (iter != vertexLayoutCache.end())
    {
        VertexLayoutHandle handle = iter->second;
        ++vertexLayouts[handle].refCount;
        return handle;
    }

    // 엘레먼트 정보
    std::span<const VertexLayoutElement> elems = _vertexLayout.GetElements();

    // 더미 hlsl 생성 필요
    std::string dummyHLSL = "struct VSInput {";

    std::vector<D3D11_INPUT_ELEMENT_DESC> d3d11InputElementDescs;
    d3d11InputElementDescs.reserve(elems.size());
    for (const VertexLayoutElement& elem: elems)
    {
        JAM_ASSERT(elem.format != eAttributeFormat::Unknown && elem.attrib != eVertexAttribute::Unknown,
                   "Vertex layout contains unknown attribute or format");

        const eVertexAttribute attrib = elem.attrib;
        const eAttributeFormat format = elem.format;

        constexpr const char* const k_d3d11semanticTable[EnumCount<eVertexAttribute>()] = {
            "POSITION",
            "NORMAL",
            "TANGENT",
            "BITANGENT",
            "COLOR",
            "COLOR",
            "COLOR",
            "COLOR",
            "BONEINDEX",
            "BONEWEIGHT",
            "TEXCOORD",
            "TEXCOORD",
            "TEXCOORD",
            "TEXCOORD",
        };

        constexpr const Int32 k_d3d11IndexTable[EnumCount<eVertexAttribute>()] = {
            0,   // POSITION
            0,   // NORMAL
            0,   // TANGENT
            0,   // BITANGENT
            0,   // COLOR0
            1,   // COLOR1
            2,   // COLOR2
            3,   // COLOR3
            0,   // BONEINDEX
            0,   // BONEWEIGHT
            0,   // TEXCOORD0
            1,   // TEXCOORD1
            2,   // TEXCOORD2
            3,   // TEXCOORD3
        };
        //// hlsl 더미 생성
        const char* const      hlslDataType = k_hlslFormatTable[EnumToInt(format)];
        const std::string_view variableName = EnumToString(attrib);
        const char* const      semanticName = k_d3d11semanticTable[EnumToInt(attrib)];
        const Int32            index        = k_d3d11IndexTable[EnumToInt(attrib)];
        dummyHLSL += fmt::format("{} {} : {}{};", hlslDataType, variableName, semanticName, index);

        // 엘레먼트 설정
        D3D11_INPUT_ELEMENT_DESC desc = {};
        desc.SemanticName             = k_d3d11semanticTable[EnumToInt(attrib)];
        desc.SemanticIndex            = index;
        desc.Format                   = k_dxgiFormatAttributeTable[EnumToInt(format)];
        desc.InputSlot                = 0;
        desc.AlignedByteOffset        = D3D11_APPEND_ALIGNED_ELEMENT;
        desc.InputSlotClass           = D3D11_INPUT_PER_VERTEX_DATA;
        desc.InstanceDataStepRate     = 0;
        d3d11InputElementDescs.push_back(desc);
    }

    // hlsl 완성
    dummyHLSL += "}; void main(VSInput IN) { }";

    // 더미 쉐이더 생성
    ShaderCompiler compiler;
    eErrorCode     ec = compiler.LoadHLSL(dummyHLSL, eShader::VertexShader, "main", eShaderCompileFlags_Optimization);
    JAM_ASSERT(IsOk(ec), "Failed to compile dummy vertex shader for input layout creation");
    MemoryView byteCode = compiler.GetBytecode();

    // 인풋 레이아웃 생성
    ID3D11InputLayout* pInputLayout = nullptr;
    JAM_CHECK_COM(pd3d11Device->CreateInputLayout(d3d11InputElementDescs.data(), elems.size(), byteCode.GetDataPtr(), byteCode.GetByteWidth(), &pInputLayout));

    // 핸들 생성
    VertexLayoutHandle handle = vertexLayouts.Alloc();

    // 객체 생성
    VertexLayoutRef vertexLayout;
    vertexLayout.refCount     = 1;
    vertexLayout.layout       = _vertexLayout;
    vertexLayout.pInputLayout = pInputLayout;
    vertexLayout.hash         = hash;
    SetD3D11Name(pInputLayout, CreateNativeResourceName("InputLayout", handle));

    // 캐시에 등록
    vertexLayouts[handle]   = std::move(vertexLayout);
    vertexLayoutCache[hash] = handle;
    return handle;
}

std::string GfxContext::CreateNativeResourceName(std::string_view _tag, const AnyHandle _handle, std::string_view _name) const
{
    UInt32 index = _handle.index;
    UInt32 token = _handle.token;

    if (_name.empty())
    {
        return std::format("Tag: {} | Index: {} | Token: {}", _tag, index, token);
    }
    else
    {
        return std::format("Tag: {} | Index: {} | Token: {} | Name: {}", _tag, index, token, _name);
    }
}

void GfxContext::SetD3D11Name(ID3D11DeviceChild* _pDeviceChild, const std::string_view _name) const
{
    JAM_ASSERT(_pDeviceChild, "Resource is null");
    JAM_CHECK_COM(_pDeviceChild->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(_name.size()), _name.data()));
}

UInt32 GfxContext::QueryMaxMultisampleQuality(const DXGI_FORMAT _format, const Int32 _sampleCount) const
{
    JAM_ASSERT(_sampleCount > 0 && _sampleCount <= 32, "Sample vertexCount must be between 1 and 32");
    UINT qualityLevels = 0;
    JAM_CHECK_COM(pd3d11Device->CheckMultisampleQualityLevels(_format, _sampleCount, &qualityLevels));
    JAM_ASSERT(qualityLevels > 0, "The specified format does not support the given sample vertexCount.");
    return qualityLevels - 1;
}

eFormat GfxContext::GetNativeFormatFromDXGIFormat(
    const DXGI_FORMAT _dxgiFormat) const
{
    switch (_dxgiFormat)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM: return eFormat::RGBA8_UNorm;
        case DXGI_FORMAT_R8G8B8A8_UINT: return eFormat::RGBA8_UInt;
        case DXGI_FORMAT_R8G8B8A8_SNORM: return eFormat::RGBA8_SNorm;
        case DXGI_FORMAT_R8G8B8A8_SINT: return eFormat::RGBA8_Int;

        case DXGI_FORMAT_R16G16B16A16_UNORM: return eFormat::RGBA16_UNorm;
        case DXGI_FORMAT_R16G16B16A16_SNORM: return eFormat::RGBA16_SNorm;
        case DXGI_FORMAT_R16G16B16A16_UINT: return eFormat::RGBA16_UInt;
        case DXGI_FORMAT_R16G16B16A16_SINT: return eFormat::RGBA16_Int;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return eFormat::RGBA16_Float;

        case DXGI_FORMAT_R32G32B32A32_UINT: return eFormat::RGBA32_UInt;
        case DXGI_FORMAT_R32G32B32A32_SINT: return eFormat::RGBA32_Int;
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return eFormat::RGBA32_Float;

        case DXGI_FORMAT_R8_UNORM: return eFormat::R8_UNorm;
        case DXGI_FORMAT_R8_SNORM: return eFormat::R8_SNorm;
        case DXGI_FORMAT_R8_UINT: return eFormat::R8_UInt;
        case DXGI_FORMAT_R8_SINT: return eFormat::R8_Int;

        case DXGI_FORMAT_R16_UNORM: return eFormat::R16_UNorm;
        case DXGI_FORMAT_R16_SNORM: return eFormat::R16_SNorm;
        case DXGI_FORMAT_R16_UINT: return eFormat::R16_UInt;
        case DXGI_FORMAT_R16_SINT: return eFormat::R16_Int;
        case DXGI_FORMAT_R16_FLOAT: return eFormat::R16_Float;

        case DXGI_FORMAT_R32_UINT: return eFormat::R32_UInt;
        case DXGI_FORMAT_R32_SINT: return eFormat::R32_Int;
        case DXGI_FORMAT_R32_FLOAT: return eFormat::R32_Float;

        case DXGI_FORMAT_R8G8_UNORM: return eFormat::RG8_UNorm;
        case DXGI_FORMAT_R8G8_SNORM: return eFormat::RG8_SNorm;
        case DXGI_FORMAT_R8G8_UINT: return eFormat::RG8_UInt;
        case DXGI_FORMAT_R8G8_SINT: return eFormat::RG8_Int;

        case DXGI_FORMAT_R16G16_UNORM: return eFormat::RG16_UNorm;
        case DXGI_FORMAT_R16G16_SNORM: return eFormat::RG16_SNorm;
        case DXGI_FORMAT_R16G16_UINT: return eFormat::RG16_UInt;
        case DXGI_FORMAT_R16G16_SINT: return eFormat::RG16_Int;
        case DXGI_FORMAT_R16G16_FLOAT: return eFormat::RG16_Float;

        case DXGI_FORMAT_R32G32_UINT: return eFormat::RG32_UInt;
        case DXGI_FORMAT_R32G32_SINT: return eFormat::RG32_Int;
        case DXGI_FORMAT_R32G32_FLOAT: return eFormat::RG32_Float;

        case DXGI_FORMAT_D16_UNORM: return eFormat::D16_UNorm;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return eFormat::D24_UNorm_S8_UInt;
        case DXGI_FORMAT_D32_FLOAT: return eFormat::D32_Float;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return eFormat::D32_Float_S8_UInt;

        default:
            JAM_ASSERT(false, "Unsupported DXGI format: {}", static_cast<int>(_dxgiFormat));
            return eFormat::Unknown;
    }
}

}   // namespace

////////////////////////////////
///          PUBLIC          ///
////////////////////////////////

namespace jam::gfx
{

void Init()
{
    const Application& app = Application::GetInstance();
    const Window&      win = app.GetWindow();
    auto [width, height]   = win.GetWindowSize();
    HWND hwnd              = win.GetPlatformHandle();

    // d3d11 device 초기화
    {
        constexpr D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };

        Int32 d3dFlags = 0;

        d3dFlags |= D3D11_CREATE_DEVICE_DEBUG;

        JAM_CHECK_COM(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            d3dFlags,
            featureLevels,
            std::size(featureLevels),
            D3D11_SDK_VERSION,
            &g_ctx.pd3d11Device,
            nullptr,
            &g_ctx.pd3d11DeviceContext));
    }

    // 스왑체인 생성
    {
        ComPtr<IDXGIDevice2> dxgiDevice;
        JAM_CHECK_COM(g_ctx.pd3d11Device->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIAdapter1> dxgiAdapter;
        JAM_CHECK_COM(dxgiDevice->GetParent(IID_PPV_ARGS(dxgiAdapter.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIFactory2> dxgiFactory;
        JAM_CHECK_COM(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        swapChainDesc.Width              = width;
        swapChainDesc.Height             = height;
        swapChainDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo             = false;
        swapChainDesc.SampleDesc.Count   = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount        = 2;
        swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;

        JAM_CHECK_COM(dxgiFactory->CreateSwapChainForHwnd(g_ctx.pd3d11Device, hwnd, &swapChainDesc, nullptr, nullptr, &g_ctx.pdxgiSwapchain));
        g_ctx.OnResize(width, height);
    }

    // screen space quad
    {
        struct FScreenVertex
        {
            Vec2 position;
            Vec2 texCoord;
        };

        VertexLayout vertexLayout;
        vertexLayout
            .Add(eVertexAttribute::Position, eAttributeFormat::Float2)
            .Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);

        constexpr FScreenVertex k_vertices[4] = {
            { Vec2(-1.f, -1.f), Vec2(0.f, 1.f) },
            { Vec2(-1.f,  1.f), Vec2(0.f, 0.f) },
            {  Vec2(1.f, -1.f), Vec2(1.f, 1.f) },
            {  Vec2(1.f,  1.f), Vec2(1.f, 0.f) }
        };

        constexpr UInt16 k_indices[6] = {
            0, 1, 2, 2, 1, 3
        };

        g_ctx.fullScreenMesh.Load(
            MemoryView { k_vertices },
            vertexLayout,
            MemoryView { k_indices },
            eIndexFormat::U16);
    }
}

void Shutdown()
{
    // 내부 리소스 해제
    JAM_DESTROY_TEXTURE(g_ctx.backBufferTextureHandle);
    JAM_DESTROY_FRAME_BUFFER(g_ctx.backBufferFrameBufferHandle);
    g_ctx.fullScreenMesh.Unload();

    // d3d11 cache 해체
    for (ID3D11BlendState* pBlendState: std::views::values(g_ctx.blendStateCache))
        pBlendState->Release();

    for (ID3D11DepthStencilState* pDepthStencilState: std::views::values(g_ctx.depthStencilStateCache))
        pDepthStencilState->Release();

    for (ID3D11RasterizerState* pRasterizerState: std::views::values(g_ctx.rasterizerStateCache))
        pRasterizerState->Release();

    for (ID3D11SamplerState* pSamplerState: std::views::values(g_ctx.samplerStateCache))
        pSamplerState->Release();

    // d3d11 device 해제
    g_ctx.pdxgiSwapchain->Release();
    g_ctx.pd3d11DeviceContext->Release();
    g_ctx.pd3d11Device->Release();
}

bool DetectLeak()
{
    constexpr const char* const k_format             = "{} leak detected: Handle index = {}, Token = {}, Name = '{}'";
    constexpr const char* const k_formatRefCountable = "{} leak detected (ref count = {}): Handle index = {}, Token = {}, Name = '{}', RefCount = {}";

    bool bLeakDetected = false;

    for (const auto& [handle, value]: g_ctx.vertexBuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        log::Warn(k_format, "VertexBuffer", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_ctx.indexBuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        log::Warn(k_format, "IndexBuffer", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_ctx.textures)
    {
        UInt32 index  = handle.index;
        UInt32 token  = handle.token;
        log::Warn(k_formatRefCountable, "Texture", index, token, value.name, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_ctx.frameBuffers)
    {
        UInt32 index  = handle.index;
        UInt32 token  = handle.token;
        log::Warn(k_format, "FrameBuffer", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_ctx.shaders)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        log::Warn(k_formatRefCountable, "Shader", index, token, value.name, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_ctx.vertexLayouts)
    {
        UInt32 index  = handle.index;
        UInt32 token  = handle.token;
        log::Warn(k_formatRefCountable, "VertexLayout", index, token, "Cannot name", value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_ctx.cbuffers)
    {
        UInt32 index  = handle.index;
        UInt32 token = handle.token;
        log::Warn(k_format, "ConstantBuffer", index, token, value.name);
        bLeakDetected = true;
    }

    for (auto handle: g_ctx.programs | std::views::keys)
    {
        UInt32 index  = handle.index;
        UInt32 token  = handle.token;
        log::Warn(k_format, "Program", index, token, "Cannot name");
        bLeakDetected = true;
    }

    return bLeakDetected;
}

void OnEvent(Event& _event)
{
    HandleEvent<WindowResizeEvent>(_event, &g_ctx, &GfxContext::OnWindowResizeEvent);
    HandleEvent<WindowMaximizeEvent>(_event, &g_ctx, &GfxContext::OnWindowMaxizeEvent);
}

void Frame(const bool _bVSync)
{
    JAM_CHECK_COM(g_ctx.pdxgiSwapchain->Present(_bVSync ? 1 : 0, 0));

    // 렌더 타겟은 매 프레임마다 다시 설정해줘야 함
    g_ctx.frameBufferHandleBind = k_invalidHandle;
}

ID3D11Device* GetDevice()
{
    return g_ctx.pd3d11Device;
}

IDXGISwapChain1* GetSwapChain()
{
    return g_ctx.pdxgiSwapchain;
}

ID3D11DeviceContext* GetDeviceContext()
{
    return g_ctx.pd3d11DeviceContext;
}

VertexBufferHandle CreateVertexBuffer(
    const MemoryView&   _vertexData,
    const VertexLayout& _vertexLayout)
{
    VertexBuffer vertexBuf;
    vertexBuf.buffer       = g_ctx.CreateNativeBuffer(_vertexData.GetByteWidth(), eBufferType::VertexBuffer, _vertexData);
    vertexBuf.vertexStride = _vertexLayout.GetStride();
    vertexBuf.vertexCount  = _vertexData.GetByteWidth() / vertexBuf.vertexStride;

    // 버텍스 레이아웃 참조
    vertexBuf.layoutHandle = g_ctx.GetOrCreateNativeVertexLayout(_vertexLayout);

    // 이름 설정
    VertexBufferHandle handle   = g_ctx.vertexBuffers.Alloc();
    g_ctx.vertexBuffers[handle] = std::move(vertexBuf);
    SetVertexBufferName(handle);
    return handle;
}

VertexBufferHandle CreateDynamicVertexBuffer(
    const Int32         _vertexCount,
    const VertexLayout& _vertexLayout)
{
    JAM_ASSERT(_vertexCount > 0, "Vertex count must be greater than 0");

    VertexBuffer vertexBuffer;
    Int32        stride       = _vertexLayout.GetStride();
    vertexBuffer.buffer       = g_ctx.CreateNativeBuffer(_vertexCount * stride, eBufferType::VertexBuffer, std::nullopt);
    vertexBuffer.vertexStride = stride;
    vertexBuffer.vertexCount  = _vertexCount;

    // 버텍스 레이아웃 참조
    vertexBuffer.layoutHandle = g_ctx.GetOrCreateNativeVertexLayout(_vertexLayout);

    // 이름 설정
    VertexBufferHandle handle   = g_ctx.vertexBuffers.Alloc();
    g_ctx.vertexBuffers[handle] = std::move(vertexBuffer);
    SetVertexBufferName(handle);
    return handle;
}

IndexBufferHandle CreateIndexBuffer(
    const MemoryView&  _indexData,
    const eIndexFormat _format)
{
    Int32 stride = _format == eIndexFormat::U16 ? 2 : 4;

    IndexBuffer indexBuffer;
    indexBuffer.buffer      = g_ctx.CreateNativeBuffer(_indexData.GetByteWidth(), eBufferType::IndexBuffer, _indexData);
    indexBuffer.indexCount  = _indexData.GetByteWidth() / stride;
    indexBuffer.indexStride = stride;
    indexBuffer.format      = _format;

    // 이름 설정
    IndexBufferHandle handle   = g_ctx.indexBuffers.Alloc();
    g_ctx.indexBuffers[handle] = std::move(indexBuffer);
    SetIndexBufferName(handle);
    return handle;
}

IndexBufferHandle CreateDynamicIndexBuffer(
    const Int32        _indexCount,
    const eIndexFormat _format)
{
    JAM_ASSERT(_indexCount > 0, "Index count must be greater than 0");
    Int32 stride = _format == eIndexFormat::U16 ? 2 : 4;

    IndexBuffer indexBuffer;
    indexBuffer.buffer      = g_ctx.CreateNativeBuffer(_indexCount * stride, eBufferType::IndexBuffer, std::nullopt);
    indexBuffer.indexCount  = _indexCount;
    indexBuffer.indexStride = stride;
    indexBuffer.format      = _format;

    // 이름 설정
    IndexBufferHandle handle   = g_ctx.indexBuffers.Alloc();
    g_ctx.indexBuffers[handle] = std::move(indexBuffer);
    SetIndexBufferName(handle);
    return handle;
}

ConstantBufferHandle CreateConstantBuffer(const Int32 _byteWidth)
{
    JAM_ASSERT(_byteWidth > 0 && _byteWidth <= 65536, "Constant buffer size must be between 1 and 65536 bytes");
    JAM_ASSERT(_byteWidth % 16 == 0, "Constant buffer size must be a multiple of 16 bytes");

    ConstantBuffer cbuffer;
    cbuffer.buffer = g_ctx.CreateNativeBuffer(_byteWidth, eBufferType::ConstantBuffer, std::nullopt);

    // 이름 설정
    ConstantBufferHandle handle = g_ctx.cbuffers.Alloc();
    g_ctx.cbuffers[handle]      = std::move(cbuffer);
    SetConstantBufferName(handle);
    return handle;
}

TextureHandle CreateTexture2D(
    const Int32                        _width,
    const Int32                        _height,
    const eFormat                      _format,
    const Int32                        _arraySize,
    const eTextureCreateFlags          _flags,
    const std::span<const MemoryView>& _initData)
{
    TextureHandle handle   = g_ctx.textures.Alloc();
    g_ctx.textures[handle] = g_ctx.CreateNativeTexture(_width, _height, 1, _format, _arraySize, _flags, eTextureType::Texture2D, _initData);
    SetTextureName(handle);
    return handle;
}
TextureHandle CreateTextureCube(
    const Int32                        _width,
    const Int32                        _height,
    const eFormat                      _format,
    const Int32                        _cubeCount,
    const eTextureCreateFlags          _flags,
    const std::span<const MemoryView>& _initData)
{
    TextureHandle handle   = g_ctx.textures.Alloc();
    g_ctx.textures[handle] = g_ctx.CreateNativeTexture(_width, _height, 1, _format, _cubeCount * 6, _flags, eTextureType::TextureCube, _initData);
    SetTextureName(handle);
    return handle;
}

TextureHandle CreateTexture3D(
    const Int32                        _width,
    const Int32                        _height,
    const Int32                        _depth,
    const eFormat                      _format,
    const eTextureCreateFlags          _texFlags,
    const std::span<const MemoryView>& _initData)
{
    TextureHandle handle   = g_ctx.textures.Alloc();
    g_ctx.textures[handle] = g_ctx.CreateNativeTexture(_width, _height, _depth, _format, 1, _texFlags, eTextureType::Texture3D, _initData);
    SetTextureName(handle);
    return handle;
}

FrameBufferHandle CreateFrameBuffer(
    const std::span<const TextureHandle> _handles,
    const bool                           _bDestroyTexture)
{
    FrameBufferHandle handle   = g_ctx.frameBuffers.Alloc();
    g_ctx.frameBuffers[handle] = g_ctx.CreateNativeFrameBuffer(_handles, _bDestroyTexture);
    SetFrameBufferName(handle);
    return handle;
}

FrameBufferHandle CreateFrameBuffer(
    const TextureHandle _handle,
    const bool          _bDestroyTexture)
{
    TextureHandle handles[1] = { _handle };
    return CreateFrameBuffer(handles, _bDestroyTexture);
}

FrameBufferHandle CreateFrameBuffer(
    const Int32   _width,
    const Int32   _height,
    const eFormat _renderTargetFormat,
    const eFormat _depthFormat,
    const bool    _writeOnlyRenderTarget,
    const bool    _writeOnlyDepthStencil)
{
    JAM_ASSERT(_width > 0 && _height > 0, "Frame buffer width and height must be greater than 0");
    JAM_ASSERT(_renderTargetFormat != eFormat::Unknown, "Render target format must be specified");
    JAM_ASSERT(_depthFormat != eFormat::Unknown, "Depth stencil format must be specified");
    JAM_ASSERT(!IsDepthFormat(_renderTargetFormat), "Render target format must not be a depth format");
    JAM_ASSERT(IsDepthFormat(_depthFormat), "Depth format must be a depth format");

    eTextureCreateFlags rtvFlags = eTextureCreateFlags_RenderTarget;
    if (_writeOnlyRenderTarget)
    {
        rtvFlags |= eTextureCreateFlags_WriteOnly;
    }

    eTextureCreateFlags dsvFlags = eTextureCreateFlags_DepthStencil;
    if (_writeOnlyDepthStencil)
    {
        dsvFlags |= eTextureCreateFlags_WriteOnly;
    }

    TextureHandle handles[2] = { k_invalidHandle, k_invalidHandle };
    handles[0]               = CreateTexture2D(_width, _height, _renderTargetFormat, 1, rtvFlags);
    handles[1]               = CreateTexture2D(_width, _height, _depthFormat, 1, dsvFlags);
    return CreateFrameBuffer(handles, true);
}

FrameBufferHandle CreateFrameBuffer(
    const Int32   _width,
    const Int32   _height,
    const eFormat _format,
    const bool    _bWriteOnly)
{
    eTextureCreateFlags flags = 0;
    if (_bWriteOnly)
    {
        flags |= eTextureCreateFlags_WriteOnly;
    }

    if (IsDepthFormat(_format))
    {
        flags |= eTextureCreateFlags_DepthStencil;
    }
    else
    {
        flags |= eTextureCreateFlags_RenderTarget;
    }

    TextureHandle handle[1] = { k_invalidHandle };
    handle[0]               = CreateTexture2D(_width, _height, _format, 1, flags);
    return CreateFrameBuffer(handle, true);
}

ShaderHandle CreateShader(const MemoryView& _shaderByteCode)
{
    return g_ctx.GetOrCreateNativeShader(_shaderByteCode);
}

ProgramHandle CreateProgram(
    const ShaderHandle _vsHandle,
    const ShaderHandle _psHandle,
    const ShaderHandle _gsHandle,
    const ShaderHandle _dsHandle,
    const ShaderHandle _hsHandle,
    const bool         _bDestroyShader)
{
    JAM_ASSERT(_vsHandle != k_invalidHandle && _psHandle != k_invalidHandle, "At least one of vertex shader or pixel shader must be provided");

    Program program;

    const ShaderHandle handles[k_renderingPipelineShaderStageCount] = {
        _vsHandle,
        _psHandle,
        _gsHandle,
        _hsHandle,
        _dsHandle
    };

    // 셰이더 설정 및 소유권 관리
    for (Int32 i = 0; i < k_renderingPipelineShaderStageCount; ++i)
    {
        eShader      stage  = static_cast<eShader>(i);
        ShaderHandle handle = handles[i];

        if (handle == k_invalidHandle)
        {
            continue;
        }

        ShaderRef& shader = g_ctx.shaders[handle];
        JAM_ASSERT(shader.stage == stage, "Shader handle does not match the expected shader stage");

        if (_bDestroyShader == false)
        {
            // 텍스처 관리의 소유권은 공유될 수 없으나
            // 쉐이더 관리의 소유권은 공유될 수 있다.
            ++shader.refCount;   // 참조 카운트 증가
        }

        program.shaderHandles[i] = handle;
    }

    // d3d shader 객체 설정
    ProgramHandle handle   = g_ctx.programs.Alloc();
    g_ctx.programs[handle] = std::move(program);
    return handle;
}

ProgramHandle CreateProgram(
    const ShaderHandle _vsHandle,
    const ShaderHandle _psHandle,
    const bool         _bDestroyShader)
{
    return CreateProgram(
        _vsHandle,
        _psHandle,
        k_invalidHandle,
        k_invalidHandle,
        k_invalidHandle,
        _bDestroyShader);
}

void DestroyVertexBuffer(const VertexBufferHandle _handle)
{
    VertexBuffer& vertexBuffer = g_ctx.vertexBuffers[_handle];

    // d3d 버텍스 버퍼 파괴
    UINT d3dRefCount = vertexBuffer.buffer.pBuffer->Release();
    if (d3dRefCount > 0)
    {
        JAM_ASSERT(0, "Vertex buffer.pBuffer reference vertexCount is not zero. RefCount: {}", d3dRefCount);
    }

    VertexLayoutHandle layoutHandle = vertexBuffer.layoutHandle;
    VertexLayoutRef&   vertexLayout = g_ctx.vertexLayouts[layoutHandle];

    // 참조 해제
    Int32 refCount = --vertexLayout.refCount;
    JAM_ASSERT(refCount >= 0, "VertexLayoutRef reference vertexCount is already zero");

    // 객체 파괴
    if (refCount == 0)
    {
        refCount = vertexLayout.pInputLayout->Release();   // d3d 리소스 파괴
        if (refCount > 0)
        {
            JAM_ASSERT(0, "Vertex layout.pInputLayout reference vertexCount is not zero. RefCount: {}", refCount);
        }

        g_ctx.vertexLayouts.Free(layoutHandle);             // 레이아웃 핸들 해제
        g_ctx.vertexLayoutCache.erase(vertexLayout.hash);   // 캐시에서 레이아웃 핸들 제거
    }

    g_ctx.vertexBuffers.Free(_handle);   // 핸들 해제
}

void DestroyIndexBuffer(const IndexBufferHandle _handle)
{
    IndexBuffer& indexBuffer = g_ctx.indexBuffers[_handle];
    UINT         d3dRefCount = indexBuffer.buffer.pBuffer->Release();   // d3d 리소스 파괴
    if (d3dRefCount > 0)
    {
        JAM_ASSERT(0, "Index buffer.pBuffer reference vertexCount is not zero. RefCount: {}", d3dRefCount);
    }

    g_ctx.indexBuffers.Free(_handle);   // 핸들 해제
}

void DestroyTexture(const TextureHandle _handle)
{
    // 텍스처 참조 해제
    TextureRef& texture  = g_ctx.textures[_handle];
    Int32       refCount = --texture.refCount;
    JAM_ASSERT(refCount >= 0, "TextureRef reference vertexCount is already zero");

    // 텍스처 파괴
    if (texture.refCount == 0)
    {
        if (texture.pSRV)   // SRV 파괴
        {
            UINT d3dRefCount = texture.pSRV->Release();
            if (d3dRefCount > 0)
            {
                JAM_ASSERT(0, "TextureRef shader resource view reference vertexCount is not zero. RefCount: {}", d3dRefCount);
            }
        }

        UINT d3dRefCount = texture.pResource->Release();   // 리소스 파괴
        if (d3dRefCount > 0)
        {
            JAM_ASSERT(0, "TextureRef resource reference vertexCount is not zero. RefCount: {}", d3dRefCount);
        }

        g_ctx.textures.Free(_handle);   // 핸들 해제
    }
}

void DestroyFrameBuffer(const FrameBufferHandle _handle)
{
    FrameBuffer& frameBuffer = g_ctx.frameBuffers[_handle];

    // RTV 파괴
    for (Int32 i = 0; i < frameBuffer.renderTargetCount; ++i)
    {
        ID3D11RenderTargetView* pRTV     = frameBuffer.pRTV[i];
        UINT                    refCount = pRTV->Release();   // d3d 리소스 파괴
        if (refCount > 0)
        {
            JAM_ASSERT(0, "Render target view reference vertexCount is not zero. RefCount: {}", refCount);
        }

        DestroyTexture(frameBuffer.renderTargetTextureHandles[i]);   // TextureHandle 해제
    }

    // DSV 파괴
    if (frameBuffer.pDSV)
    {
        ID3D11DepthStencilView* pDSV     = frameBuffer.pDSV;
        UINT                    refCount = pDSV->Release();   // d3d 리소스 파괴
        if (refCount > 0)
        {
            JAM_ASSERT(0, "Depth stencil view reference vertexCount is not zero. RefCount: {}", refCount);
        }

        DestroyTexture(frameBuffer.depthStencilTextureHandle);   // TextureHandle 해제
    }

    g_ctx.frameBuffers.Free(_handle);   // FrameBuffer 핸들 해제
}

void DestroyProgram(const ProgramHandle _handle)
{
    Program& program = g_ctx.programs[_handle];

    for (ShaderHandle handle: program.shaderHandles)
    {
        if (handle != k_invalidHandle)
        {
            DestroyShader(handle);   // 셰이더 참조 해제
        }
    }

    g_ctx.programs.Free(_handle);   // Program 핸들 해제
}

void DestroyShader(const ShaderHandle _handle)
{
    ShaderRef& shader = g_ctx.shaders[_handle];
    JAM_ASSERT(shader.refCount > 0, "ShaderRef reference vertexCount is already zero");

    // 참조 해제
    Int32 refCount = --shader.refCount;
    JAM_ASSERT(refCount >= 0, "ShaderRef reference vertexCount is already zero");

    if (refCount == 0)
    {
        // d3d 쉐이더 객체 파괴
        switch (shader.stage)
        {
            case eShader::VertexShader: refCount = shader.pVertexShader->Release(); break;
            case eShader::PixelShader: refCount = shader.pPixelShader->Release(); break;
            case eShader::GeometryShader: refCount = shader.pGeometryShader->Release(); break;
            case eShader::HullShader: refCount = shader.pHullShader->Release(); break;
            case eShader::DomainShader: refCount = shader.pDomainShader->Release(); break;
            case eShader::ComputeShader: refCount = shader.pComputeShader->Release(); break;
            default:
                JAM_ASSERT(0, "Unsupported shader stage");
        }

        if (refCount > 0)
        {
            JAM_ASSERT(0, "Shader reference vertexCount is not zero. RefCount: {}", refCount);
        }

        g_ctx.shaders.Free(_handle);                // Shader 핸들 해제
        g_ctx.shaderCache.erase(shader.cacheKey);   // 캐시에서 제거
    }
}

void DestroyConstantBuffer(
    const ConstantBufferHandle _handle)
{
    ConstantBuffer& cbuffer  = g_ctx.cbuffers[_handle];
    UINT            refCount = cbuffer.buffer.pBuffer->Release();
    if (refCount > 0)
    {
        JAM_ASSERT(0, "Constant buffer.pBuffer reference vertexCount is not zero. RefCount: {}", refCount);
    }

    g_ctx.cbuffers.Free(_handle);   // 핸들 해제
}

void SetVertexBufferName(
    const VertexBufferHandle _handle,
    const std::string_view   _name)
{
    VertexBuffer& vertexBuffer = g_ctx.vertexBuffers[_handle];
    vertexBuffer.name          = g_ctx.CreateNativeResourceName("VertexBuffer", _handle, _name);
    g_ctx.SetD3D11Name(vertexBuffer.buffer.pBuffer, vertexBuffer.name);
}

void SetIndexBufferName(
    const IndexBufferHandle _handle,
    const std::string_view  _name)
{
    IndexBuffer& indexBuffer = g_ctx.indexBuffers[_handle];
    indexBuffer.name         = g_ctx.CreateNativeResourceName("IndexBuffer", _handle, _name);
    g_ctx.SetD3D11Name(indexBuffer.buffer.pBuffer, indexBuffer.name);
}

void SetConstantBufferName(
    const ConstantBufferHandle _handle,
    const std::string_view     _name)
{
    ConstantBuffer& cbuffer = g_ctx.cbuffers[_handle];
    cbuffer.name            = g_ctx.CreateNativeResourceName("ConstantBuffer", _handle, _name);
    g_ctx.SetD3D11Name(cbuffer.buffer.pBuffer, cbuffer.name);
}

void SetTextureName(
    const TextureHandle    _handle,
    const std::string_view _name)
{
    constexpr const char* const k_names[EnumCount<eTextureType>()] = {
        "Texture2D",
        "Texture3D",
        "TextureCube",
    };

    TextureRef& texture = g_ctx.textures[_handle];
    texture.name        = g_ctx.CreateNativeResourceName(k_names[EnumToInt(texture.type)], _handle, _name);
    g_ctx.SetD3D11Name(texture.pResource, texture.name);

    if (texture.pSRV)
    {
        constexpr const char* const k_srvNames[EnumCount<eTextureType>()] = {
            "Texture2D_SRV",
            "Texture3D_SRV",
            "TextureCube_SRV",
        };

        std::string name = g_ctx.CreateNativeResourceName(k_srvNames[EnumToInt(texture.type)], _handle, _name);
        g_ctx.SetD3D11Name(texture.pSRV, name);
    }
}

void SetFrameBufferName(
    const FrameBufferHandle _handle,
    const std::string_view  _name)
{
    constexpr const char* const k_names[k_renderTargetCountMax] = {
        "FrameBuffer_RTV0",
        "FrameBuffer_RTV1",
        "FrameBuffer_RTV2",
        "FrameBuffer_RTV3",
        "FrameBuffer_RTV4",
        "FrameBuffer_RTV5",
        "FrameBuffer_RTV6",
        "FrameBuffer_RTV7",
    };

    FrameBuffer& frameBuffer = g_ctx.frameBuffers[_handle];
    frameBuffer.name         = g_ctx.CreateNativeResourceName("FrameBuffer", _handle, _name);

    for (Int32 i = 0; i < frameBuffer.renderTargetCount; ++i)
    {
        if (frameBuffer.pRTV[i])
        {
            std::string name = g_ctx.CreateNativeResourceName(k_names[i], _handle, _name);
            g_ctx.SetD3D11Name(frameBuffer.pRTV[i], name);
        }
    }

    if (frameBuffer.pDSV)
    {
        std::string name = g_ctx.CreateNativeResourceName("FrameBuffer_DSV", _handle, _name);
        g_ctx.SetD3D11Name(frameBuffer.pDSV, name);
    }
}

void SetShaderName(
    const ShaderHandle     _handle,
    const std::string_view _name)
{
    constexpr const char* const k_names[EnumCount<eShader>()] = {
        "VertexShader",
        "PixelShader",
        "GeometryShader",
        "HullShader",
        "DomainShader",
        "ComputeShader"
    };

    ShaderRef& shader = g_ctx.shaders[_handle];
    shader.name       = g_ctx.CreateNativeResourceName(k_names[EnumToInt(shader.stage)], _handle, _name);

    switch (shader.stage)
    {
        case eShader::VertexShader:
            g_ctx.SetD3D11Name(shader.pVertexShader, shader.name);
            break;
        case eShader::PixelShader:
            g_ctx.SetD3D11Name(shader.pPixelShader, shader.name);
            break;
        case eShader::GeometryShader:
            g_ctx.SetD3D11Name(shader.pGeometryShader, shader.name);
            break;
        case eShader::HullShader:
            g_ctx.SetD3D11Name(shader.pHullShader, shader.name);
            break;
        case eShader::DomainShader:
            g_ctx.SetD3D11Name(shader.pDomainShader, shader.name);
            break;
        case eShader::ComputeShader:
            g_ctx.SetD3D11Name(shader.pComputeShader, shader.name);
            break;
        default:
            JAM_ASSERT(0, "Unsupported shader stage");
    }
}

TextureHandle LoadTexture(
    const DirectX::ScratchImage& _scratchImage)
{
    ID3D11Resource* pResource = nullptr;
    JAM_CHECK_COM(DirectX::CreateTexture(g_ctx.pd3d11Device,
                                         _scratchImage.GetImages(),
                                         _scratchImage.GetImageCount(),
                                         _scratchImage.GetMetadata(),
                                         &pResource));

    const DirectX::TexMetadata& metaData = _scratchImage.GetMetadata();

    TextureRef texture;
    texture.width     = metaData.width;
    texture.height    = metaData.height;
    texture.depth     = metaData.depth;
    texture.format    = g_ctx.GetNativeFormatFromDXGIFormat(metaData.format);
    texture.arraySize = metaData.arraySize;
    texture.samples   = 1;
    texture.mipsCount = metaData.mipLevels;
    texture.refCount  = 1;
    texture.bManaged  = false;
    texture.bMutable  = false;
    texture.pResource = pResource;

    if (metaData.IsCubemap())
    {
        texture.type = eTextureType::TextureCube;
    }
    else if (metaData.dimension == DirectX::TEX_DIMENSION_TEXTURE3D)
    {
        texture.type = eTextureType::Texture3D;
    }
    else if (metaData.dimension == DirectX::TEX_DIMENSION_TEXTURE2D)
    {
        texture.type = eTextureType::Texture2D;
    }
    else
    {
        JAM_ASSERT(0, "Unsupported texture dimension");
    }

    JAM_CHECK_COM(DirectX::CreateShaderResourceView(g_ctx.pd3d11Device,
                                                    _scratchImage.GetImages(),
                                                    _scratchImage.GetImageCount(),
                                                    _scratchImage.GetMetadata(),
                                                    &texture.pSRV));

    TextureHandle handle   = g_ctx.textures.Alloc();
    g_ctx.textures[handle] = std::move(texture);
    SetTextureName(handle);
    return handle;
}

Result<TextureHandle> LoadTexture(
    const fs::path& _filePath,
    const bool      _bSRGB,
    const bool      _bGenMips)
{
    ImageImporter importer;
    importer.SetPath(_filePath);
    if (_bSRGB)
    {
        importer.ForceSRGB();
    }

    if (_bGenMips)
    {
        importer.GenerateMips();
    }

    eErrorCode ec = importer.Import();
    if (IsError(ec))
    {
        return ec;
    }

    return LoadTexture(importer.GetScratchImage());
}

void UploadTexture2D(
    const TextureHandle _handle,
    const MemoryView&   _writeTexData,
    const Int32         _uploadTexWidth,
    const Int32         _uploadTexHeight,
    const Int32         _mipIndex,
    const Int32         _arrayIndex,
    const Int32         _xOffset,
    const Int32         _yOffset)
{
    const TextureRef& texture = g_ctx.textures[_handle];
    g_ctx.UploadTexture(
        texture,
        _writeTexData,
        _uploadTexWidth,
        _uploadTexHeight,
        1,
        _mipIndex,
        _arrayIndex,
        _xOffset,
        _yOffset,
        0);
}

void UploadTextureCube(
    const TextureHandle _handle,
    const MemoryView&   _writeTexData,
    const Int32         _uploadTexWidth,
    const Int32         _uploadTexHeight,
    const Int32         _mipIndex,
    const Int32         _cubeIndex,
    const eCubeFace     _cubeFace,
    const Int32         _xOffset,
    const Int32         _yOffset)
{
    const TextureRef& texture = g_ctx.textures[_handle];
    g_ctx.UploadTexture(
        texture,
        _writeTexData,
        _uploadTexWidth,
        _uploadTexHeight,
        1,
        _mipIndex,
        _cubeIndex * 6 + k_d3d11CubeFaceTable[EnumToInt(_cubeFace)],
        _xOffset,
        _yOffset,
        0);
}

void UploadTexture3D(
    const TextureHandle _handle,
    const MemoryView&   _writeTexData,
    const Int32         _uploadTexWidth,
    const Int32         _uploadTexHeight,
    const Int32         _uploadTexDepth,
    const Int32         _mipIndex,
    const Int32         _xOffset,
    const Int32         _yOffset,
    const Int32         _zOffset)
{
    const TextureRef& texture = g_ctx.textures[_handle];
    g_ctx.UploadTexture(
        texture,
        _writeTexData,
        _uploadTexWidth,
        _uploadTexHeight,
        _uploadTexDepth,
        _mipIndex,
        0,
        _xOffset,
        _yOffset,
        _zOffset);
}

std::vector<UInt8> DownloadTexture2D(
    const TextureHandle _handle,
    const Int32         _mipIndex,
    const Int32         _arrayIndex)
{
    const TextureRef& texture = g_ctx.textures[_handle];
    return g_ctx.DownloadTexture(
        texture,
        _mipIndex,
        _arrayIndex);
}

std::vector<UInt8> DownloadTextureCube(
    const TextureHandle _handle,
    const Int32         _mipIndex,
    const Int32         _cubeIndex,
    const eCubeFace     _cubeFace)
{
    const TextureRef& texture = g_ctx.textures[_handle];
    return g_ctx.DownloadTexture(
        texture,
        _mipIndex,
        _cubeIndex * 6 + k_d3d11CubeFaceTable[EnumToInt(_cubeFace)]);
}

std::vector<UInt8> DownloadTexture3D(
    const TextureHandle _handle,
    const Int32         _mipIndex)
{
    const TextureRef& texture = g_ctx.textures[_handle];
    return g_ctx.DownloadTexture(
        texture,
        _mipIndex,
        0);
}

DirectX::ScratchImage Capture(
    const TextureHandle _handle)
{
    const TextureRef&     texture = g_ctx.textures[_handle];
    DirectX::ScratchImage scratchImage;
    JAM_CHECK_COM(DirectX::CaptureTexture(g_ctx.pd3d11Device, g_ctx.pd3d11DeviceContext, texture.pResource, scratchImage));
    return scratchImage;
}

void Resolve(
    const TextureHandle _srcHandle,
    const Int32         _srcMipIndex,
    const Int32         _srcArrayIndex,
    const TextureHandle _dstHandle,
    const Int32         _dstMipIndex,
    const Int32         _dstArrayIndex)
{
    TextureRef& srcTexture = g_ctx.textures[_srcHandle];
    TextureRef& dstTexture = g_ctx.textures[_dstHandle];
    JAM_ASSERT(dstTexture.samples == 1, "Destination texture must not be multi-sampled.");

    Int32 srcIndex = srcTexture.mipsCount * _srcArrayIndex + _srcMipIndex;
    Int32 dstIndex = dstTexture.mipsCount * _dstArrayIndex + _dstMipIndex;
    g_ctx.pd3d11DeviceContext->ResolveSubresource(dstTexture.pResource,
                                                  dstIndex,
                                                  srcTexture.pResource,
                                                  srcIndex,
                                                  k_formatInfoTable[EnumToInt(dstTexture.format)].texFormat);
}

void Blit(
    const TextureHandle _srcHandle,
    const TextureHandle _dstHandle,
    const Int32         _blitWidthOrZero,
    const Int32         _blitHeightOrZero,
    const Int32         _blitDepthOrZero,
    const Int32         _srcMipIndex,
    const Int32         _srcArrayIndex,
    const Int32         _srcX,
    const Int32         _srcY,
    const Int32         _srcZ,
    const Int32         _dstMipIndex,
    const Int32         _dstArrayIndex,
    const Int32         _dstX,
    const Int32         _dstY,
    const Int32         _dstZ)
{
    const TextureRef& srcTexture = g_ctx.textures[_srcHandle];
    const TextureRef& dstTexture = g_ctx.textures[_dstHandle];
    g_ctx.Blit(srcTexture,
               dstTexture,
               _blitWidthOrZero,
               _blitHeightOrZero,
               _blitDepthOrZero,
               _srcMipIndex,
               _srcArrayIndex,
               _srcX,
               _srcY,
               _srcZ,
               _dstMipIndex,
               _dstArrayIndex,
               _dstX,
               _dstY,
               _dstZ);
}

void BlitToBackBuffer(
    const TextureHandle _srcHandle,
    const Int32         _blitWidthOrZero,
    const Int32         _blitHeightOrZero,
    const Int32         _srcMipIndex,
    const Int32         _srcArrayIndex,
    const Int32         _srcX,
    const Int32         _srcY,
    const Int32         _dstX,
    const Int32         _dstY)
{
    const TextureRef& srcTexture = g_ctx.textures[_srcHandle];
    const TextureRef& dstTexture = g_ctx.textures[g_ctx.backBufferTextureHandle];
    JAM_ASSERT(srcTexture.type == eTextureType::Texture2D, "Source texture must be 2D texture.");
    g_ctx.Blit(srcTexture, dstTexture, _blitWidthOrZero, _blitHeightOrZero, 1, _srcMipIndex, _srcArrayIndex, _srcX, _srcY, 0, 0, 0, _dstX, _dstY, 0);
}

void ClearRenderTarget(
    const FrameBufferHandle _handle,
    const RGBA              _color,
    const Int32             _renderTargetIndex)
{
    FrameBufferHandle  handle      = _handle == k_invalidHandle ? g_ctx.backBufferFrameBufferHandle : _handle;
    const FrameBuffer& frameBuffer = g_ctx.frameBuffers[handle];
    JAM_ASSERT(frameBuffer.pRTV[_renderTargetIndex], "Frame buffer does not have a render target view to clear.");

    const FLOAT clearColor[4] = {
        static_cast<FLOAT>(_color.r) / 255.0f,
        static_cast<FLOAT>(_color.g) / 255.0f,
        static_cast<FLOAT>(_color.b) / 255.0f,
        static_cast<FLOAT>(_color.a) / 255.0f,
    };

    g_ctx.pd3d11DeviceContext->ClearRenderTargetView(frameBuffer.pRTV[_renderTargetIndex], clearColor);
}

void ClearDepthStencil(
    const FrameBufferHandle _handle,
    const bool              _bClearDepth,
    const bool              _bClearStencil,
    const float             _depth,
    const UInt8             _stencil)
{
    FrameBufferHandle  handle      = _handle == k_invalidHandle ? g_ctx.backBufferFrameBufferHandle : _handle;
    const FrameBuffer& frameBuffer = g_ctx.frameBuffers[handle];
    JAM_ASSERT(frameBuffer.pDSV, "Frame buffer does not have a depth stencil view to clear.");

    Int32 clearFlags = 0;
    if (_bClearDepth) clearFlags |= D3D11_CLEAR_DEPTH;
    if (_bClearStencil) clearFlags |= D3D11_CLEAR_STENCIL;

    g_ctx.pd3d11DeviceContext->ClearDepthStencilView(frameBuffer.pDSV, clearFlags, _depth, _stencil);
}

const TextureHandle& GetRenderTargetTexture(
    const FrameBufferHandle _handle,
    const Int32             _renderTargetIndex)
{
    const FrameBuffer& frameBuffer = g_ctx.frameBuffers[_handle];
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < frameBuffer.renderTargetCount, "Render target m_index out of range.");
    return frameBuffer.renderTargetTextureHandles[_renderTargetIndex];
}

const TextureHandle& GetDepthStencilTexture(
    const FrameBufferHandle _handle)
{
    const FrameBuffer& frameBuffer = g_ctx.frameBuffers[_handle];
    JAM_ASSERT(frameBuffer.pDSV, "Frame buffer does not have a depth stencil view.");
    return frameBuffer.depthStencilTextureHandle;
}

void UploadBuffer(
    const VertexBufferHandle _handle,
    const MemoryView&        _writeData,
    const Int32              _offset,
    const bool               _bDiscard)
{
    const VertexBuffer& buffer = g_ctx.vertexBuffers[_handle];
    g_ctx.UploadBuffer(buffer.buffer, _writeData, _offset, _bDiscard);
}

void UploadBuffer(
    const IndexBufferHandle _handle,
    const MemoryView&       _writeData,
    const Int32             _offset,
    const bool              _bDiscard)
{
    const IndexBuffer& buffer = g_ctx.indexBuffers[_handle];
    g_ctx.UploadBuffer(buffer.buffer, _writeData, _offset, _bDiscard);
}

void UploadBuffer(
    const ConstantBufferHandle _handle,
    const MemoryView&          _writeData)
{
    ConstantBuffer& buffer = g_ctx.cbuffers[_handle];
    g_ctx.UploadBuffer(buffer.buffer, _writeData, 0, true);
}

std::vector<UInt8> DownloadBuffer(
    const VertexBufferHandle _handle)
{
    const VertexBuffer& buffer = g_ctx.vertexBuffers[_handle];
    return g_ctx.DownloadBuffer(buffer.buffer);
}

std::vector<UInt8> DownloadBuffer(
    const IndexBufferHandle _handle)
{
    const IndexBuffer& buffer = g_ctx.indexBuffers[_handle];
    return g_ctx.DownloadBuffer(buffer.buffer);
}

std::vector<Byte> DownloadBuffer(
    const ConstantBufferHandle _handle)
{
    const ConstantBuffer& buffer = g_ctx.cbuffers[_handle];
    return g_ctx.DownloadBuffer(buffer.buffer);
}

const VertexLayout& GetVertexLayout(
    const VertexBufferHandle _handle)
{
    const VertexBuffer&    buffer    = g_ctx.vertexBuffers[_handle];
    const VertexLayoutRef& layoutRef = g_ctx.vertexLayouts[buffer.layoutHandle];
    return layoutRef.layout;
}

void SetVertexBuffer(
    const VertexBufferHandle _handleOrInvalid)
{
    const VertexBuffer& buffer = g_ctx.vertexBuffers[_handleOrInvalid];
    SetVertexBuffer(_handleOrInvalid, buffer.vertexCount, 0);
}

void SetVertexBuffer(
    const VertexBufferHandle _handleOrInvalid,
    const Int32              _vertexCount,
    const Int32              _vertexOffset)
{
    if (g_ctx.vertexBufferHandleBind != _handleOrInvalid || g_ctx.vertexBufferOffsetBind != _vertexOffset)
    {
        g_ctx.vertexBufferHandleBind = _handleOrInvalid;
        g_ctx.vertexBufferOffsetBind = _vertexOffset;
        g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_VertexBuffer;
    }

    g_ctx.vertexCountBind = _vertexCount;
}

void SetIndexBuffer(const IndexBufferHandle _handleOrInvalid)
{
    const IndexBuffer& buffer = g_ctx.indexBuffers[_handleOrInvalid];
    SetIndexBuffer(_handleOrInvalid, buffer.indexCount, 0);
}

void SetIndexBuffer(
    const IndexBufferHandle _handleOrInvalid,
    const Int32             _indexCount,
    const Int32             _indexOffset)
{
    if (g_ctx.indexBufferHandleBind != _handleOrInvalid)
    {
        g_ctx.indexBufferHandleBind = _handleOrInvalid;
        g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_IndexBuffer;
    }

    g_ctx.indexBufferOffsetBind = _indexOffset;
    g_ctx.indexCountBind        = _indexCount;
}

void SetConstantBuffer(
    const ConstantBufferHandle _handleOrInvalid,
    const eShader              _shader,
    const Int32                _slot)
{
    const Int32          stage        = EnumToInt(_shader);
    ConstantBufferBinds& cbufferBinds = g_ctx.cbufferHandleBinds[stage];

    if (cbufferBinds.cbufferHandles[_slot] != _handleOrInvalid)
    {
        cbufferBinds.cbufferHandles[_slot] = _handleOrInvalid;
        cbufferBinds.dirtyStart            = Min(cbufferBinds.dirtyStart, _slot);
        cbufferBinds.dirtyEnd              = Max(cbufferBinds.dirtyEnd, _slot);
        g_ctx.pipelineDirtyFlags |= (ePipelineDirtyFlags_VSConstantBuffer << stage);
    }
}

void SetTexture(
    const TextureHandle _handleOrInvalid,
    const eShader       _shader,
    const Int32         _slot)
{
    const Int32   stage        = EnumToInt(_shader);
    TextureBinds& textureBinds = g_ctx.textureHandleBinds[stage];

    if (textureBinds.textureHandles[_slot] != _handleOrInvalid)
    {
        textureBinds.textureHandles[_slot] = _handleOrInvalid;
        textureBinds.dirtyStart            = Min(textureBinds.dirtyStart, _slot);
        textureBinds.dirtyEnd              = Max(textureBinds.dirtyEnd, _slot);
        g_ctx.pipelineDirtyFlags |= (ePipelineDirtyFlags_VSShaderResourceView << stage);
    }
}

void SetSampler(
    const eShader       _shader,
    const Int32         _slot,
    const eSamplerFlags _flags)
{
    const Int32        stage        = EnumToInt(_shader);
    SamplerFlagsBinds& samplerBinds = g_ctx.samplerFlagsBinds[stage];

    if (samplerBinds.samplerFlags[_slot] != _flags)
    {
        samplerBinds.samplerFlags[_slot] = _flags;
        samplerBinds.dirtyStart          = Min(samplerBinds.dirtyStart, _slot);
        samplerBinds.dirtyEnd            = Max(samplerBinds.dirtyEnd, _slot);
        g_ctx.pipelineDirtyFlags |= (ePipelineDirtyFlags_VSSamplerState << stage);
    }
}

void SetState(const eStateFlags _stateFlags)
{
    if (g_ctx.stateFlagsBind != _stateFlags)
    {
        // blend state
        BlendStateCacheKey blendStateKey = _stateFlags & eStateFlags_BlendStateMask_Internal;
        if (g_ctx.blendStateBind != blendStateKey)
        {
            g_ctx.blendStateBind = blendStateKey;
            g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_BlendState;
        }

        // depth state
        DepthStencilCacheKey depthStateKey              = ((_stateFlags & eStateFlags_DepthStateMask_Internal) >> eStateFlags_DepthStateShift_Internal) << 32;
        DepthStencilCacheKey depthStateKeyPipelineState = (g_ctx.depthStencilStateBind & UINT64_C(0x0000'00FF'0000'0000));

        if (depthStateKeyPipelineState != depthStateKey)
        {
            g_ctx.depthStencilStateBind = (g_ctx.depthStencilStateBind & UINT64_C(0xFFFF'FF00'FFFF'FFFF)) | depthStateKey;
            g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
        }

        // rasterizer state
        RasterizerStateCacheKey rasterizerStateKey = _stateFlags & eStateFlags_RasterizerStateMask_Internal;
        if (g_ctx.rasterizerStateBind != rasterizerStateKey)
        {
            g_ctx.rasterizerStateBind = rasterizerStateKey;
            g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_RasterizerState;
        }

        // primitive topology
        PrimitiveTopologyBind primitiveTopologyKey = (_stateFlags & eStateFlags_PrimitiveTopologyMask_Internal) >> eStateFlags_PrimitiveTopologyShift_Internal;
        if (g_ctx.primitiveTopologyBind != primitiveTopologyKey)
        {
            g_ctx.primitiveTopologyBind = primitiveTopologyKey;
            g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_PrimitiveTopology;
        }
    }
}

void SetBlendFactor(const RGBA _rgba)
{
    if (g_ctx.blendFactorBind.color != _rgba.color)
    {
        g_ctx.blendFactorBind = _rgba;
        g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_BlendState;
    }
}

void SetBorderColor(
    const eShader _shader,
    const Int32   _slot,
    const RGBA    _rgba)
{
    const Int32        shaderIndex  = EnumToInt(_shader);
    SamplerFlagsBinds& pipeCacheRef = g_ctx.samplerFlagsBinds[shaderIndex];
    if (pipeCacheRef.borderColors[_slot].color != _rgba.color)
    {
        pipeCacheRef.borderColors[_slot] = _rgba;
        pipeCacheRef.dirtyStart          = Min(pipeCacheRef.dirtyStart, _slot);
        pipeCacheRef.dirtyEnd            = Max(pipeCacheRef.dirtyEnd, _slot);
        g_ctx.pipelineDirtyFlags |= (ePipelineDirtyFlags_VSSamplerState << shaderIndex);
    }
}

void SetStencil(
    const eStencilFlags _fStencil,
    const eStencilFlags _bStencil,
    const UInt8         _stencilRef)
{

    if (g_ctx.fstencilFlagsBind != _fStencil)
    {
        g_ctx.fstencilFlagsBind     = _fStencil;
        g_ctx.depthStencilStateBind = (g_ctx.depthStencilStateBind & UINT64_C(0xFFFF'FFFF'0000'FFFF)) | (_fStencil << 16);
        g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
    }

    if (g_ctx.bstencilFlagsBind != _bStencil)
    {
        g_ctx.bstencilFlagsBind     = _bStencil;
        g_ctx.depthStencilStateBind = (g_ctx.depthStencilStateBind & UINT64_C(0xFFFF'FFFF'FFFF'0000)) | (_bStencil << 32);
        g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
    }

    if (g_ctx.stencilRefBind != _stencilRef)
    {
        g_ctx.stencilRefBind = _stencilRef;
        g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
    }
}

void SetProgram(const ProgramHandle _handleOrInvalid)
{
    if (g_ctx.programHandleBind != _handleOrInvalid)
    {
        g_ctx.programHandleBind = _handleOrInvalid;
        g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_Program;
    }
}

void SetFrameBuffer(const FrameBufferHandle _handleOrInvalid)
{
    FrameBufferHandle handle = _handleOrInvalid == k_invalidHandle ? g_ctx.backBufferFrameBufferHandle : _handleOrInvalid;
    if (g_ctx.frameBufferHandleBind != handle)
    {
        g_ctx.frameBufferHandleBind = handle;
        g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_FrameBuffer;
    }
}

void SetViewport(const Viewport _viewport)
{
    if (g_ctx.viewportBind.viewport != _viewport.viewport)
    {
        g_ctx.viewportBind = _viewport;
        g_ctx.pipelineDirtyFlags |= ePipelineDirtyFlags_Viewport;
    }
}

void SetViewport(const UInt16 _x, const UInt16 _y, const UInt16 _width, const UInt16 _height)
{
    SetViewport(Viewport { _x, _y, _width, _height });
}

ShaderHandle GetShader(
    const ProgramHandle _programHandle,
    const eShader       _shaderType)
{
    const Program&     program = g_ctx.programs[_programHandle];
    const ShaderHandle handle  = program.shaderHandles[EnumToInt(_shaderType)];
    JAM_ASSERT(handle != k_invalidHandle, "Program does not have the specified shader stage.");
    return handle;
}

void SubmitFullScreenQuad()
{
    g_ctx.fullScreenMesh.Submit();
}

void Submit()
{
    g_ctx.ApplyPipeline();
    if (g_ctx.indexBufferHandleBind == k_invalidHandle)
    {
        g_ctx.pd3d11DeviceContext->Draw(g_ctx.vertexCountBind, 0);
    }
    else
    {
        g_ctx.pd3d11DeviceContext->DrawIndexed(g_ctx.indexCountBind, g_ctx.indexBufferOffsetBind, 0);
    }
}

}   // namespace jam::gfx