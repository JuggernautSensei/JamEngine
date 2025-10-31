#include "pch.h"

#include "Graphics.h"

#include "BaseUtilities.h"

#include <d3d11shader.h>

#include "Event.h"
#include "Hash.h"
#include "ImageImporter.h"
#include "Memory.h"
#include "ResourcePool.h"
#include "ShaderCompiler.h"
#include "Window.h"

#include <DirectXTex.h>
#include <d3dcompiler.h>

namespace jam
{

///////////////////////////////
///          ENUMS          ///
///////////////////////////////

enum ePipelineDirtyFlags_ : UInt32
{
    ePipelineDirtyFlags_None = 0,

    ePipelineDirtyFlags_Primitive    = 1 << 0,
    ePipelineDirtyFlags_VertexBuffer = 1 << 1,
    ePipelineDirtyFlags_IndexBuffer  = 1 << 2,

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

    ePipelineDirtyFlags_All = 0xFFFFFFFF
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
    DXGI_FORMAT texFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT srvFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
};

constexpr FormatInfo k_formatInfoTable[EnumCount<eFormat>()] = {
    // eFormat::Unknown
    {             DXGI_FORMAT_UNKNOWN,                  DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R8_UNorm
    {            DXGI_FORMAT_R8_UNORM,                 DXGI_FORMAT_R8_UNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R8_SNorm
    {            DXGI_FORMAT_R8_SNORM,                 DXGI_FORMAT_R8_SNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R8_UInt
    {             DXGI_FORMAT_R8_UINT,                  DXGI_FORMAT_R8_UINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R8_Int
    {             DXGI_FORMAT_R8_SINT,                  DXGI_FORMAT_R8_SINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_UNorm
    {           DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_R16_UNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_SNorm
    {           DXGI_FORMAT_R16_SNORM,                DXGI_FORMAT_R16_SNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_UInt
    {            DXGI_FORMAT_R16_UINT,                 DXGI_FORMAT_R16_UINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_Int
    {            DXGI_FORMAT_R16_SINT,                 DXGI_FORMAT_R16_SINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R16_Float
    {           DXGI_FORMAT_R16_FLOAT,                DXGI_FORMAT_R16_FLOAT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R32_UInt
    {            DXGI_FORMAT_R32_UINT,                 DXGI_FORMAT_R32_UINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R32_Int
    {            DXGI_FORMAT_R32_SINT,                 DXGI_FORMAT_R32_SINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::R32_Float
    {           DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_R32_FLOAT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG8_UNorm
    {          DXGI_FORMAT_R8G8_UNORM,               DXGI_FORMAT_R8G8_UNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG8_SNorm
    {          DXGI_FORMAT_R8G8_SNORM,               DXGI_FORMAT_R8G8_SNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG8_UInt
    {           DXGI_FORMAT_R8G8_UINT,                DXGI_FORMAT_R8G8_UINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG8_Int
    {           DXGI_FORMAT_R8G8_SINT,                DXGI_FORMAT_R8G8_SINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_UNorm
    {        DXGI_FORMAT_R16G16_UNORM,             DXGI_FORMAT_R16G16_UNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_SNorm
    {        DXGI_FORMAT_R16G16_SNORM,             DXGI_FORMAT_R16G16_SNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_UInt
    {         DXGI_FORMAT_R16G16_UINT,              DXGI_FORMAT_R16G16_UINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_Int
    {         DXGI_FORMAT_R16G16_SINT,              DXGI_FORMAT_R16G16_SINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG16_Float
    {        DXGI_FORMAT_R16G16_FLOAT,             DXGI_FORMAT_R16G16_FLOAT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG32_UInt
    {         DXGI_FORMAT_R32G32_UINT,              DXGI_FORMAT_R32G32_UINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG32_Int
    {         DXGI_FORMAT_R32G32_SINT,              DXGI_FORMAT_R32G32_SINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RG32_Float
    {        DXGI_FORMAT_R32G32_FLOAT,             DXGI_FORMAT_R32G32_FLOAT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA8_UNorm
    {      DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA8_SNorm
    {      DXGI_FORMAT_R8G8B8A8_SNORM,           DXGI_FORMAT_R8G8B8A8_SNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA8_UInt
    {       DXGI_FORMAT_R8G8B8A8_UINT,            DXGI_FORMAT_R8G8B8A8_UINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA8_Int
    {       DXGI_FORMAT_R8G8B8A8_SINT,            DXGI_FORMAT_R8G8B8A8_SINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_UNorm
    {  DXGI_FORMAT_R16G16B16A16_UNORM,       DXGI_FORMAT_R16G16B16A16_UNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_SNorm
    {  DXGI_FORMAT_R16G16B16A16_SNORM,       DXGI_FORMAT_R16G16B16A16_SNORM,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_UInt
    {   DXGI_FORMAT_R16G16B16A16_UINT,        DXGI_FORMAT_R16G16B16A16_UINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_Int
    {   DXGI_FORMAT_R16G16B16A16_SINT,        DXGI_FORMAT_R16G16B16A16_SINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA16_Float
    {  DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA32_UInt
    {   DXGI_FORMAT_R32G32B32A32_UINT,        DXGI_FORMAT_R32G32B32A32_UINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA32_Int
    {   DXGI_FORMAT_R32G32B32A32_SINT,        DXGI_FORMAT_R32G32B32A32_SINT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA32_Float
    {  DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT,              DXGI_FORMAT_UNKNOWN },

    // eFormat::RGBA8_UNorm_sRGB
    { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,              DXGI_FORMAT_UNKNOWN },

    // eFormat::BGRA8_UNorm_sRGB
    { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,              DXGI_FORMAT_UNKNOWN },

    // eFormat::D16_UNorm
    {        DXGI_FORMAT_R16_TYPELESS,                DXGI_FORMAT_R16_UNORM,            DXGI_FORMAT_D16_UNORM },

    // eFormat::D24_UNorm_S8_UInt
    {      DXGI_FORMAT_R24G8_TYPELESS,    DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_D24_UNORM_S8_UINT },

    // eFormat::D32_Float
    {        DXGI_FORMAT_R32_TYPELESS,                DXGI_FORMAT_R32_FLOAT,            DXGI_FORMAT_D32_FLOAT },

    // eFormat::D32_Float_S8_UInt
    {   DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT },
};

struct SamplerInfo
{
    D3D11_FILTER filter;
    D3D11_FILTER comparisonFilter;
};

constexpr SamplerInfo k_samplerInfoTable[EnumCount<eTextureFilter>()] = {
    // 기본 필터링
    {               D3D11_FILTER_MIN_MAG_MIP_POINT,               D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT }, // MinPoint_MagPoint_MipPoint
    {        D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,        D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR }, // MinPoint_MagPoint_MipLinear
    {  D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,  D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT }, // MinPoint_MagLinear_MipPoint
    {        D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,        D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR }, // MinPoint_MagLinear_MipLinear
    {        D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,        D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT }, // MinLinear_MagPoint_MipPoint
    { D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR }, // MinLinear_MagPoint_MipLinear
    {        D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,        D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT }, // MinLinear_MagLinear_MipPoint
    {              D3D11_FILTER_MIN_MAG_MIP_LINEAR,              D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR }, // MinLinear_MagLinear_MipLinear

    // 이방성 필터링
    {                     D3D11_FILTER_ANISOTROPIC,                     D3D11_FILTER_COMPARISON_ANISOTROPIC }, // Anisotropic4
    {                     D3D11_FILTER_ANISOTROPIC,                     D3D11_FILTER_COMPARISON_ANISOTROPIC }, // Anisotropic8
    {                     D3D11_FILTER_ANISOTROPIC,                     D3D11_FILTER_COMPARISON_ANISOTROPIC }  // Anisotropic16
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

constexpr D3D11_COMPARISON_FUNC k_d3d11ComparisonFuncTable[EnumCount<eCompareFunc>()] = {
    D3D11_COMPARISON_LESS,            // eCompareFunc::None
    D3D11_COMPARISON_LESS,            // eCompareFunc::Less
    D3D11_COMPARISON_LESS_EQUAL,      // eCompareFunc::LessEqual
    D3D11_COMPARISON_GREATER,         // eCompareFunc::Greater
    D3D11_COMPARISON_GREATER_EQUAL,   // eCompareFunc::GreaterEqual
    D3D11_COMPARISON_EQUAL,           // eCompareFunc::Equal
    D3D11_COMPARISON_NOT_EQUAL,       // eCompareFunc::NotEqual
    D3D11_COMPARISON_ALWAYS,          // eCompareFunc::Always
    D3D11_COMPARISON_NEVER            // eCompareFunc::Never
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

constexpr D3D11_PRIMITIVE_TOPOLOGY k_d3d11PrimitiveTopologyTable[EnumCount<ePrimitive>()] = {
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,    // ePrimitive::TriangleList
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,   // ePrimitive::TriangleStrip
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST,        // ePrimitive::LineList
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,       // ePrimitive::LineStrip
    D3D11_PRIMITIVE_TOPOLOGY_POINTLIST        // ePrimitive::PointList
};

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
    SamplerState samplerStates[k_samplerStateCountMax] = {};
    RGBA         borderColors[k_samplerStateCountMax];

    Int32 dirtyStart = k_samplerStateCountMax;
    Int32 dirtyEnd   = 0;
};

struct RasterizerStateD3D11
{
    eCullMode cullMode : 2;
    bool      bWireframe : 1;
    bool      bDepthClamp : 1;
    bool      bMultiSample : 1;
    bool      bLineAA : 1;
    bool      bScissor : 1;
    bool      bCCWFront : 1;
};

struct BlendStateD3D11
{
    bool       bAlphaToCoverage : 1;
    bool       bIndependentBlend : 1;
    BlendState renderTarget[k_renderTargetCountMax] = {};
};

struct DepthStencilStateD3D11
{
    bool         bDepthWrite : 1;
    eCompareFunc depthTestFunc : 4;
    StencilState fStencil = {};
    StencilState bStencil = {};
};

//////////////////////////////////
///          INTERNAL          ///
//////////////////////////////////

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
        const std::span<const MemoryView>& _initDataOrEmpty) const;

    JAM_NODISCARD Buffer CreateNativeBuffer(
        Int32                            _byteWidth,
        eBufferType                      _type,
        const std::optional<MemoryView>& _initData) const;

    JAM_NODISCARD FrameBuffer CreateNativeFrameBuffer(
        std::span<const TextureHandle> _handles,
        bool                           _bDestoryTexture);

    ////////// DATA UPLOAD & DOWNLOAD //////////

    void UploadBuffer(
        const Buffer& _buffer,
        MemoryView    _uploadData,
        Int32         _offset,
        bool          _bDiscard) const;

    JAM_NODISCARD std::vector<UInt8> DownloadBuffer(
        const Buffer& _buffer) const;

    void UploadTexture(
        const TextureRef& _texture,
        MemoryView        _uploadTexData,
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

    JAM_NODISCARD ID3D11RasterizerState*   GetOrCreateD3D11RasterizerState(RasterizerStateD3D11 _state);
    JAM_NODISCARD ID3D11BlendState*        GetOrCreateD3D11BlendState(const BlendStateD3D11& _state);
    JAM_NODISCARD ID3D11DepthStencilState* GetOrCreateD3D11DepthStencilState(DepthStencilStateD3D11 _state);
    JAM_NODISCARD ID3D11SamplerState*      GetOrCreateD3D11SamplerState(SamplerState _state, RGBA _color);
    JAM_NODISCARD ShaderHandle             GetOrCreateNativeShader(MemoryView _shaderByteCode);
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

    HashMap<UInt32, ID3D11RasterizerState*>   rasterizerStateCache;
    HashMap<UInt64, ID3D11BlendState*>        blendStateCache;
    HashMap<UInt64, ID3D11DepthStencilState*> depthStencilStateCache;
    HashMap<UInt64, ID3D11SamplerState*>      samplerStateCache;

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

    GfxState stateBind       = {};
    RGBA     blendFactorBind = {};
    UInt8    stencilRefBind  = 0;

    ePrimitive             primitiveBind         = ePrimitive::TriangleList;
    DepthStencilStateD3D11 depthStencilStateBind = {};
    RasterizerStateD3D11   rasterizerStateBind   = {};
    BlendStateD3D11        blendStateBind        = {};

    ePipelineDirtyFlags pipelineDirtyFlags = ePipelineDirtyFlags_All;   // 처음에는 모두 더티 (기본 상태로 적용하기 위해)

    ////////// INTERNAL RESOURCE //////////

    TextureHandle     backBufferTextureHandle;
    FrameBufferHandle backBufferFrameBufferHandle;
} g_gfxContext;

ID3D11RasterizerState* GfxContext::GetOrCreateD3D11RasterizerState(
    const RasterizerStateD3D11 _state)
{
    MurmurHash32 hasher;
    hasher.Begin();
    hasher.Add(&_state, sizeof(RasterizerStateD3D11));
    UInt32 key = hasher.End();
    auto   it  = rasterizerStateCache.find(key);

    if (it != rasterizerStateCache.end())
        return it->second;

    D3D11_RASTERIZER_DESC desc;
    desc.FillMode              = _state.bWireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    desc.CullMode              = _state.cullMode == eCullMode::None  ? D3D11_CULL_NONE
                               : _state.cullMode == eCullMode::Front ? D3D11_CULL_FRONT
                                                                     : D3D11_CULL_BACK;
    desc.FrontCounterClockwise = _state.bCCWFront;
    desc.DepthBias             = 0;
    desc.DepthBiasClamp        = 0.f;
    desc.SlopeScaledDepthBias  = 0.f;
    desc.DepthClipEnable       = !_state.bDepthClamp;
    desc.MultisampleEnable     = _state.bMultiSample;
    desc.AntialiasedLineEnable = _state.bLineAA;
    desc.ScissorEnable         = _state.bScissor;

    ID3D11RasterizerState* pRasterizerState = nullptr;
    JAM_CHECK_COM(pd3d11Device->CreateRasterizerState(&desc, &pRasterizerState));
    rasterizerStateCache[key] = pRasterizerState;
    return pRasterizerState;
}

ID3D11BlendState* GfxContext::GetOrCreateD3D11BlendState(
    const BlendStateD3D11& _state)
{
    MurmurHash64 hasher;
    hasher.Begin();
    hasher.Add(&_state, sizeof(BlendStateD3D11));
    UInt64 key = hasher.End();

    auto it = blendStateCache.find(key);
    if (it != blendStateCache.end())
        return it->second;

    D3D11_BLEND_DESC desc {};
    desc.AlphaToCoverageEnable  = _state.bAlphaToCoverage;
    desc.IndependentBlendEnable = _state.bIndependentBlend;
    for (Int32 i = 0; i < k_renderTargetCountMax; ++i)
    {
        const BlendState&               rtBlend = _state.renderTarget[i];
        D3D11_RENDER_TARGET_BLEND_DESC& rtDesc  = desc.RenderTarget[i];

        rtDesc.BlendEnable    = rtBlend.bEnable;
        rtDesc.SrcBlend       = k_d3d11BlendTable[EnumToInt(rtBlend.srcBlend)];
        rtDesc.DestBlend      = k_d3d11BlendTable[EnumToInt(rtBlend.dstBlend)];
        rtDesc.BlendOp        = k_d3d11BlendOpTable[EnumToInt(rtBlend.blendOp)];
        rtDesc.SrcBlendAlpha  = k_d3d11BlendTable[EnumToInt(rtBlend.srcBlendAlpha)];
        rtDesc.DestBlendAlpha = k_d3d11BlendTable[EnumToInt(rtBlend.dstBlendAlpha)];
        rtDesc.BlendOpAlpha   = k_d3d11BlendOpTable[EnumToInt(rtBlend.blendOpAlpha)];

        rtDesc.RenderTargetWriteMask = 0;
        if (rtBlend.writeMask & eWriteFlags_Red)
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
        if (rtBlend.writeMask & eWriteFlags_Green)
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
        if (rtBlend.writeMask & eWriteFlags_Blue)
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
        if (rtBlend.writeMask & eWriteFlags_Alpha)
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    }

    ID3D11BlendState* pBlendState = nullptr;
    JAM_CHECK_COM(pd3d11Device->CreateBlendState(&desc, &pBlendState));
    blendStateCache[key] = pBlendState;
    return pBlendState;
}

ID3D11DepthStencilState* GfxContext::GetOrCreateD3D11DepthStencilState(
    const DepthStencilStateD3D11 _state)
{
    MurmurHash64 hasher;
    hasher.Begin();
    hasher.Add(&_state, sizeof(DepthStencilStateD3D11));
    UInt64 key = hasher.End();
    auto   it  = depthStencilStateCache.find(key);
    if (it != depthStencilStateCache.end())
        return it->second;

    D3D11_DEPTH_STENCIL_DESC desc;
    desc.DepthEnable      = _state.depthTestFunc != eCompareFunc::None;
    desc.DepthWriteMask   = _state.bDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc        = k_d3d11ComparisonFuncTable[EnumToInt(_state.depthTestFunc)];
    desc.StencilEnable    = _state.fStencil.compareFunc != eCompareFunc::None || _state.bStencil.compareFunc != eCompareFunc::None;
    desc.StencilReadMask  = 0xFF;
    desc.StencilWriteMask = 0xFF;

    // front
    desc.FrontFace.StencilFailOp      = k_d3d11StencilOpTable[EnumToInt(_state.fStencil.stencilFailOp)];
    desc.FrontFace.StencilDepthFailOp = k_d3d11StencilOpTable[EnumToInt(_state.fStencil.depthFailOp)];
    desc.FrontFace.StencilPassOp      = k_d3d11StencilOpTable[EnumToInt(_state.fStencil.passOp)];
    desc.FrontFace.StencilFunc        = k_d3d11ComparisonFuncTable[EnumToInt(_state.fStencil.compareFunc)];

    // back
    desc.BackFace.StencilFailOp      = k_d3d11StencilOpTable[EnumToInt(_state.bStencil.stencilFailOp)];
    desc.BackFace.StencilDepthFailOp = k_d3d11StencilOpTable[EnumToInt(_state.bStencil.depthFailOp)];
    desc.BackFace.StencilPassOp      = k_d3d11StencilOpTable[EnumToInt(_state.bStencil.passOp)];
    desc.BackFace.StencilFunc        = k_d3d11ComparisonFuncTable[EnumToInt(_state.bStencil.compareFunc)];

    ID3D11DepthStencilState* pDepthStencilState = nullptr;
    JAM_CHECK_COM(pd3d11Device->CreateDepthStencilState(&desc, &pDepthStencilState));
    depthStencilStateCache[key] = pDepthStencilState;
    return pDepthStencilState;
}

ID3D11SamplerState* GfxContext::GetOrCreateD3D11SamplerState(
    const SamplerState _state,
    const RGBA         _color)
{
    UInt64 key = _state.value;
    key        = (key << 32ull) | _color.color;

    auto it = samplerStateCache.find(key);
    if (it != samplerStateCache.end())
    {
        return it->second;
    }

    const SamplerInfo& filter          = k_samplerInfoTable[EnumToInt(_state.filter)];
    bool               bIsComparison   = _state.compareFunc != eCompareFunc::None;
    Int32              anisotopicLevel = _state.filter == eTextureFilter::Anisotropic4  ? 4
                                       : _state.filter == eTextureFilter::Anisotropic8  ? 8
                                       : _state.filter == eTextureFilter::Anisotropic16 ? 16
                                                                                        : 1;
    D3D11_SAMPLER_DESC desc;
    desc.Filter         = bIsComparison ? filter.comparisonFilter : filter.filter;
    desc.AddressU       = k_d3d11TextureAddrModeTable[EnumToInt(_state.addressU)];
    desc.AddressV       = k_d3d11TextureAddrModeTable[EnumToInt(_state.addressV)];
    desc.AddressW       = k_d3d11TextureAddrModeTable[EnumToInt(_state.addressW)];
    desc.MipLODBias     = 0.0f;
    desc.MaxAnisotropy  = anisotopicLevel;
    desc.ComparisonFunc = bIsComparison ? k_d3d11ComparisonFuncTable[EnumToInt(_state.compareFunc)] : D3D11_COMPARISON_ALWAYS;
    desc.BorderColor[0] = static_cast<FLOAT>(_color.r) / 255.0f;
    desc.BorderColor[1] = static_cast<FLOAT>(_color.g) / 255.0f;
    desc.BorderColor[2] = static_cast<FLOAT>(_color.b) / 255.0f;
    desc.BorderColor[3] = static_cast<FLOAT>(_color.a) / 255.0f;
    desc.MinLOD         = FloatLimits::lowest();
    desc.MaxLOD         = FloatLimits::max();

    ID3D11SamplerState* pSamplerState = nullptr;
    JAM_CHECK_COM(pd3d11Device->CreateSamplerState(&desc, &pSamplerState));
    samplerStateCache[key] = pSamplerState;
    return pSamplerState;
}

void GfxContext::ApplyConstantBuffers(const eShader _shader)
{

    static ID3D11Buffer* s_binder[k_constantBufferCountMax];
    ConstantBufferBinds& binds = cbufferHandleBinds[EnumToInt(_shader)];

    Int32 dirtyStart = binds.dirtyStart;
    if (binds.dirtyStart > binds.dirtyEnd)
        return;

    Int32 dirtyCount = binds.dirtyEnd - binds.dirtyStart + 1;
    for (Int32 index = 0; index < dirtyCount; ++index)
    {
        Handle<ConstantBuffer> handle = binds.cbufferHandles[dirtyStart + index];
        s_binder[index]               = JAM_IS_VALID_HANDLE(handle) ? cbuffers[handle].buffer.pBuffer : nullptr;
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
        default:
            JAM_ASSERT(0, "Unknown shader stage");
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
    if (bindRef.dirtyStart > bindRef.dirtyEnd)
        return;

    Int32 dirtyCount = bindRef.dirtyEnd - bindRef.dirtyStart + 1;
    for (Int32 i = 0; i < dirtyCount; ++i)
    {
        Handle<TextureRef> handle = bindRef.textureHandles[dirtyStart + i];
        s_binder[i]               = JAM_IS_VALID_HANDLE(handle) ? textures[handle].pSRV : nullptr;
    }

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
        default:
            JAM_ASSERT(0, "Unknown shader stage");
            break;
    }

    // reset dirty range
    bindRef.dirtyStart = k_shaderResourceCountMax;
    bindRef.dirtyEnd   = 0;
}

void GfxContext::ApplySamplerStates(const eShader _shader)
{
    static ID3D11SamplerState* s_binder[k_samplerStateCountMax];
    SamplerFlagsBinds&         bind = samplerFlagsBinds[EnumToInt(_shader)];

    Int32 dirtyStart = bind.dirtyStart;
    if (bind.dirtyStart > bind.dirtyEnd)
        return;

    Int32 dirtyCount = bind.dirtyEnd - bind.dirtyStart + 1;
    for (Int32 i = 0; i < dirtyCount; ++i)
    {
        s_binder[i] = g_gfxContext.GetOrCreateD3D11SamplerState(bind.samplerStates[dirtyStart + i], bind.borderColors[dirtyStart + i]);
    }

    switch (_shader)
    {
        case eShader::VertexShader:
            g_gfxContext.pd3d11DeviceContext->VSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::PixelShader:
            g_gfxContext.pd3d11DeviceContext->PSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::GeometryShader:
            g_gfxContext.pd3d11DeviceContext->GSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::HullShader:
            g_gfxContext.pd3d11DeviceContext->HSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::DomainShader:
            g_gfxContext.pd3d11DeviceContext->DSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        case eShader::ComputeShader:
            g_gfxContext.pd3d11DeviceContext->CSSetSamplers(dirtyStart, dirtyCount, s_binder);
            break;
        default:
            JAM_ASSERT(0, "Unknown shader stage");
            break;
    }

    // reset dirty range
    bind.dirtyStart = k_samplerStateCountMax;
    bind.dirtyEnd   = 0;
}

void GfxContext::ApplyPipeline()
{
    // apply pipeline state if dirty
    if (pipelineDirtyFlags != ePipelineDirtyFlags_None)
    {
        constexpr ID3D11Buffer* k_nullBuffer = nullptr;

        // vertex buffer
        if (pipelineDirtyFlags & ePipelineDirtyFlags_VertexBuffer)
        {
            if (JAM_IS_VALID_HANDLE(vertexBufferHandleBind))
            {
                const VertexBuffer& vertexBuffer = vertexBuffers[vertexBufferHandleBind];
                const UINT          stride       = vertexBuffer.vertexStride;
                const UINT          offset       = g_gfxContext.vertexBufferOffsetBind * stride;
                pd3d11DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer.buffer.pBuffer, &stride, &offset);
            }
            else
            {
                constexpr UINT k_stride = 0;
                constexpr UINT k_offset = 0;
                pd3d11DeviceContext->IASetVertexBuffers(0, 1, &k_nullBuffer, &k_stride, &k_offset);
            }
        }

        // index buffer
        if (pipelineDirtyFlags & ePipelineDirtyFlags_IndexBuffer)
        {
            if (JAM_IS_VALID_HANDLE(indexBufferHandleBind))
            {
                constexpr DXGI_FORMAT k_indexFormatTable[] = {
                    DXGI_FORMAT_R16_UINT,   // eIndexFormat::U16
                    DXGI_FORMAT_R32_UINT    // eIndexFormat::U32
                };

                const IndexBuffer& indexBuffer = indexBuffers[indexBufferHandleBind];
                pd3d11DeviceContext->IASetIndexBuffer(indexBuffer.buffer.pBuffer, k_indexFormatTable[EnumToInt(indexBuffer.format)], 0);
            }
            else
            {
                pd3d11DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
            }
        }

        // topology
        if (pipelineDirtyFlags & ePipelineDirtyFlags_Primitive)
        {
            pd3d11DeviceContext->IASetPrimitiveTopology(k_d3d11PrimitiveTopologyTable[EnumToInt(primitiveBind)]);
        }

        // input layout
        if (pipelineDirtyFlags & ePipelineDirtyFlags_InputLayout)
        {
            if (JAM_IS_VALID_HANDLE(vertexLayoutHandleBind))
            {
                const VertexLayoutRef& layout = vertexLayouts[vertexLayoutHandleBind];
                pd3d11DeviceContext->IASetInputLayout(layout.pInputLayout);
            }
            else
            {
                pd3d11DeviceContext->IASetInputLayout(nullptr);
            }
        }

        // shader program
        if (pipelineDirtyFlags & ePipelineDirtyFlags_Program)
        {
            if (JAM_IS_VALID_HANDLE(programHandleBind))
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
                            default:
                                JAM_ASSERT(0, "Unknown shader stage");
                                break;
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
                            default:
                                JAM_ASSERT(0, "Unknown shader stage");
                                break;
                        }
                    }
                }
            }
            else
            {
                pd3d11DeviceContext->VSSetShader(nullptr, nullptr, 0);
                pd3d11DeviceContext->PSSetShader(nullptr, nullptr, 0);
                pd3d11DeviceContext->GSSetShader(nullptr, nullptr, 0);
                pd3d11DeviceContext->HSSetShader(nullptr, nullptr, 0);
                pd3d11DeviceContext->DSSetShader(nullptr, nullptr, 0);
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
            FLOAT blendFactor[4] = {
                static_cast<float>(blendFactorBind.r) / 255.f,
                static_cast<float>(blendFactorBind.g) / 255.f,
                static_cast<float>(blendFactorBind.b) / 255.f,
                static_cast<float>(blendFactorBind.a) / 255.f
            };

            ID3D11BlendState* pBlendState = GetOrCreateD3D11BlendState(blendStateBind);
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
            JAM_ASSERT(JAM_IS_VALID_HANDLE(frameBufferHandleBind), "frameBufferHandleBind is invalid");
            const FrameBuffer& fb = frameBuffers[frameBufferHandleBind];
            pd3d11DeviceContext->OMSetRenderTargets(fb.renderTargetCount, fb.pRTV, fb.pDSV);
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
        JAM_DESTROY_TEXTURE(backBufferTextureHandle);
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
            bbTexture.width          = NarrowCast<Int32>(desc.Width);
            bbTexture.height         = NarrowCast<Int32>(desc.Height);
            bbTexture.depth          = 1;
            bbTexture.type           = eTextureType::Texture2D;
            bbTexture.format         = eFormat::RGBA8_UNorm;
            bbTexture.mipsCount      = 1;
            bbTexture.arraySize      = 1;
            bbTexture.samples        = 1;
            bbTexture.bMutable       = true;
            bbTexture.refCount       = 1;
            bbTexture.bManaged       = false;
            bbTexture.bRTVAttachable = true;
            bbTexture.bDSVAttachable = false;
            bbTexture.pResource      = pBackBufferRes;

            // 텍스처 풀에 등록
            TextureHandle texHandle = textures.Alloc();
            textures[texHandle]     = std::move(bbTexture);
            Graphics::SetTextureName(texHandle);

            backBufferTextureHandle     = texHandle;
            backBufferFrameBufferHandle = Graphics::CreateFrameBuffer(texHandle);
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
    const std::span<const MemoryView>& _initDataOrEmpty) const
{
    bool bGenMips       = (_flags & eTextureCreateFlags_GenerateMips) != 0;
    bool bIsMSAA        = (_flags & (eTextureCreateFlags_MultiSampled_Internal)) != 0;
    bool bWriteOnly     = (_flags & eTextureCreateFlags_WriteOnly) != 0;
    bool bRTVAttachable = (_flags & eTextureCreateFlags_RenderTarget) != 0;
    bool bDSVAttachable = (_flags & eTextureCreateFlags_DepthStencil) != 0;
    bool bMutable       = _initDataOrEmpty.empty();
    bool bNeedInit      = !_initDataOrEmpty.empty();
    bool bIsCubemap     = _type == eTextureType::TextureCube;

    const Int32 mipsCount = bGenMips ? CalcMipsCount(_width, _height, _depth) : 1;
    const Int32 samples   = (_flags & eTextureCreateFlags_MSAAx4) != 0  ? 4
                          : (_flags & eTextureCreateFlags_MSAAx8) != 0  ? 8
                          : (_flags & eTextureCreateFlags_MSAAx16) != 0 ? 16
                                                                        : 1;

    // 검증
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "Texture dimensions must be greater than zero.");
    JAM_ASSERT(_arraySize > 0, "Texture array size must be greater than zero.");
    JAM_ASSERT(!(bIsMSAA && bGenMips), "Multisampled texture cannot have mipmaps.");

    // 포맷 검증 (dsv 쓰기전용 텍스처는 최적화된 포맷을 사용)
    const FormatInfo& formatInfo       = k_formatInfoTable[EnumToInt(_format)];
    const bool        bWriteOnlyDSVTex = bWriteOnly && bDSVAttachable && !bRTVAttachable;
    const DXGI_FORMAT dxgiFormat       = bWriteOnlyDSVTex ? formatInfo.dsvFormat : formatInfo.texFormat;

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
                JAM_ASSERT(_initDataOrEmpty.size() == _arraySize * mipsCount, "Texture data count mismatch");

                std::vector<D3D11_SUBRESOURCE_DATA> initData;
                initData.reserve(_arraySize * mipsCount);
                for (Int32 arrayIdx = 0; arrayIdx < _arraySize; arrayIdx++)
                {
                    for (Int32 mipIdx = 0; mipIdx < mipsCount; mipIdx++)
                    {
                        auto [width, height, _] = CalcMipSize(_width, _height, 0, mipIdx);
                        Int32      texIndex     = CalcTextureIndex(mipIdx, arrayIdx, mipsCount);
                        MemoryView mem          = _initDataOrEmpty[texIndex];

                        UINT rowPitch = width * GetPixelByte(_format);
                        JAM_ASSERT(mem.GetByteWidth() == rowPitch * height, "Texture data size mismatch");
                        initData.emplace_back(mem.GetDataPtr(), rowPitch, 0);
                    }
                }

                JAM_CHECK_COM(pd3d11Device->CreateTexture2D(&desc, initData.data(), &texture.pTexture2D));
            }
            else
            {
                JAM_CHECK_COM(pd3d11Device->CreateTexture2D(&desc, nullptr, &texture.pTexture2D));
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
                JAM_ASSERT(_initDataOrEmpty.size() == mipsCount, "Texture data count mismatch");

                std::vector<D3D11_SUBRESOURCE_DATA> initData;
                initData.reserve(mipsCount);

                for (Int32 mipIdx = 0; mipIdx < mipsCount; mipIdx++)
                {
                    auto [width, height, depth] = CalcMipSize(_width, _height, _depth, mipIdx);
                    MemoryView mem              = _initDataOrEmpty[mipIdx];

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

            default:
                JAM_CRASH("Unsupported texture stage: {}", EnumToInt(_type));
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
        JAM_ASSERT(JAM_IS_VALID_HANDLE(handle), "Invalid texture handle in render target textures.");

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

                default:
                    JAM_CRASH("Unsupported texture stage: {}", EnumToInt(texture.type));
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
            rtvDesc.Format = k_formatInfoTable[EnumToInt(texture.format)].texFormat;

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

                default:
                    JAM_CRASH("Unsupported texture stage: {}", EnumToInt(texture.type));
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
    const Buffer&    _buffer,
    const MemoryView _uploadData,
    const Int32      _offset,
    const bool       _bDiscard) const
{
    JAM_ASSERT(_buffer.bDynamic, "Only dynamic buffer can be updated");

    const Byte* pUploadData     = _uploadData.GetDataPtr();
    Int32       uploadDataWidth = NarrowCast<Int32>(_uploadData.GetByteWidth());
    uploadDataWidth             = Min<Int32>(uploadDataWidth, _buffer.byteWidth - _offset);

    if (_bDiscard)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        JAM_CHECK_COM(pd3d11DeviceContext->Map(_buffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
        std::memcpy(static_cast<Byte*>(mappedResource.pData) + _offset, pUploadData, uploadDataWidth);
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
        desc.ByteWidth      = uploadDataWidth;

        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem          = pUploadData;
        initData.SysMemPitch      = 0;
        initData.SysMemSlicePitch = 0;

        JAM_CHECK_COM(pd3d11Device->CreateBuffer(&desc, &initData, pStagingBuffer.ReleaseAndGetAddressOf()));

        D3D11_BOX box;
        box.left   = 0;
        box.right  = uploadDataWidth;
        box.top    = 0;
        box.bottom = 1;
        box.front  = 0;
        box.back   = 1;

        pd3d11DeviceContext->CopySubresourceRegion(
            _buffer.pBuffer, 0, _offset, 0, 0, pStagingBuffer.Get(), 0, &box);
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
    const MemoryView  _uploadTexData,
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

        default:
            JAM_CRASH("Unsupported texture stage: {}", EnumToInt(_texture.type));
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
    JAM_ASSERT(_srcTexture.type == _dstTexture.type, "Source and destination texture types must match for Blit operation.");
    JAM_ASSERT(_srcTexture.mipsCount == _dstTexture.mipsCount, "Source and destination texture mip counts must match for Blit operation.");
    JAM_ASSERT(_blitDepthOrZero >= 0 && _blitHeightOrZero >= 0 && _blitWidthOrZero >= 0, "Blit dimensions must be non-negative.");
    JAM_ASSERT(_srcMipIndex < _srcTexture.mipsCount, "Source mip index out of range.");
    JAM_ASSERT(_dstMipIndex < _dstTexture.mipsCount, "Destination mip index out of range.");
    JAM_ASSERT(_srcArrayIndex < _srcTexture.arraySize, "Source array index out of range.");
    JAM_ASSERT(_dstArrayIndex < _dstTexture.arraySize, "Destination array index out of range.");
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

ShaderHandle GfxContext::GetOrCreateNativeShader(MemoryView _shaderByteCode)
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
    Graphics::SetShaderName(handle);
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

        constexpr Int32 k_d3d11IndexTable[EnumCount<eVertexAttribute>()] = {
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
    eErrorCode     ec = compiler.CompileFromSource(dummyHLSL, eShader::VertexShader, "main", eShaderCompileFlags_Optimization);
    JAM_ASSERT(IsOk(ec), "Failed to compile dummy vertex shader for input layout creation");
    MemoryView byteCode = compiler.GetBytecode();

    // 인풋 레이아웃 생성
    ID3D11InputLayout* pInputLayout = nullptr;
    UINT               elemCount    = NarrowCast<UINT>(elems.size());
    JAM_CHECK_COM(pd3d11Device->CreateInputLayout(d3d11InputElementDescs.data(), elemCount, byteCode.GetDataPtr(), byteCode.GetByteWidth(), &pInputLayout));

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

        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return eFormat::BGRA8_UNorm_sRGB;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return eFormat::RGBA8_UNorm_sRGB;

        default:
            JAM_ASSERT(0, "Unsupported DXGI format: {}", static_cast<int>(_dxgiFormat));
            return eFormat::Unknown;
    }
}

}   // namespace jam

namespace jam
{

void Graphics::Init(const HWND _hwnd, const Int32 _width, const Int32 _height)
{
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
            &g_gfxContext.pd3d11Device,
            nullptr,
            &g_gfxContext.pd3d11DeviceContext));
    }

    // 스왑체인 생성
    {
        ComPtr<IDXGIDevice2> dxgiDevice;
        JAM_CHECK_COM(g_gfxContext.pd3d11Device->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIAdapter1> dxgiAdapter;
        JAM_CHECK_COM(dxgiDevice->GetParent(IID_PPV_ARGS(dxgiAdapter.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIFactory2> dxgiFactory;
        JAM_CHECK_COM(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        swapChainDesc.Width              = _width;
        swapChainDesc.Height             = _height;
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

        JAM_CHECK_COM(dxgiFactory->CreateSwapChainForHwnd(g_gfxContext.pd3d11Device, _hwnd, &swapChainDesc, nullptr, nullptr, &g_gfxContext.pdxgiSwapchain));
        g_gfxContext.OnResize(_width, _height);
    }
}

void Graphics::Shutdown()
{
    // 내부 리소스 해제
    JAM_DESTROY_TEXTURE(g_gfxContext.backBufferTextureHandle);
    JAM_DESTROY_FRAME_BUFFER(g_gfxContext.backBufferFrameBufferHandle);

    // d3d11 cache 해체
    for (ID3D11BlendState* pBlendState: std::views::values(g_gfxContext.blendStateCache))
        pBlendState->Release();

    for (ID3D11DepthStencilState* pDepthStencilState: std::views::values(g_gfxContext.depthStencilStateCache))
        pDepthStencilState->Release();

    for (ID3D11RasterizerState* pRasterizerState: std::views::values(g_gfxContext.rasterizerStateCache))
        pRasterizerState->Release();

    for (ID3D11SamplerState* pSamplerState: std::views::values(g_gfxContext.samplerStateCache))
        pSamplerState->Release();

    // d3d11 device 해제
    g_gfxContext.pdxgiSwapchain->Release();
    g_gfxContext.pd3d11DeviceContext->Release();
    g_gfxContext.pd3d11Device->Release();
}

bool Graphics::DetectLeak()
{
    bool bLeakDetected = false;

    for (const auto& [handle, value]: g_gfxContext.vertexBuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("VertexBuffer leak detected: Handle index = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_gfxContext.indexBuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("IndexBuffer leak detected: Handle index = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_gfxContext.textures)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("Texture leak detected: Handle index = {}, Token = {}, Name = '{}', RefCount = {}", index, token, value.name, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_gfxContext.frameBuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("FrameBuffer leak detected: Handle index = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_gfxContext.shaders)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("Shader leak detected: Handle index = {}, Token = {}, Stage = {}, RefCount = {}", index, token, EnumToString(value.stage), value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_gfxContext.vertexLayouts)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("VertexLayout leak detected: Handle index = {}, Token = {}, RefCount = {}", index, token, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_gfxContext.cbuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("ConstantBuffer leak detected: Handle index = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (auto handle: g_gfxContext.programs | std::views::keys)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("Program leak detected: Handle index = {}, Token = {}", index, token);
        bLeakDetected = true;
    }

    return bLeakDetected;
}

void Graphics::OnEvent(Event& _event)
{
    HandleEvent<WindowResizeEvent>(_event, &g_gfxContext, &GfxContext::OnWindowResizeEvent);
    HandleEvent<WindowMaximizeEvent>(_event, &g_gfxContext, &GfxContext::OnWindowMaxizeEvent);
}

void Graphics::Frame(const bool _bVSync)
{
    JAM_CHECK_COM(g_gfxContext.pdxgiSwapchain->Present(_bVSync ? 1 : 0, 0));

    // 렌더 타겟은 매 프레임마다 다시 설정해줘야 함
    g_gfxContext.frameBufferHandleBind = k_invalidHandle;
}

ID3D11Device* Graphics::GetDevice()
{
    return g_gfxContext.pd3d11Device;
}

IDXGISwapChain1* Graphics::GetSwapChain()
{
    return g_gfxContext.pdxgiSwapchain;
}

ID3D11DeviceContext* Graphics::GetDeviceContext()
{
    return g_gfxContext.pd3d11DeviceContext;
}

VertexBufferHandle Graphics::CreateVertexBuffer(
    MemoryView          _vertexData,
    const VertexLayout& _vertexLayout)
{

    VertexBuffer vertexBuffer;
    Int32        bufferWidth  = NarrowCast<Int32>(_vertexData.GetByteWidth());
    vertexBuffer.buffer       = g_gfxContext.CreateNativeBuffer(bufferWidth, eBufferType::VertexBuffer, _vertexData);
    vertexBuffer.vertexStride = _vertexLayout.GetStride();
    vertexBuffer.vertexCount  = bufferWidth / vertexBuffer.vertexStride;

    // 버텍스 레이아웃 참조
    vertexBuffer.layoutHandle = g_gfxContext.GetOrCreateNativeVertexLayout(_vertexLayout);

    // 이름 설정
    VertexBufferHandle handle          = g_gfxContext.vertexBuffers.Alloc();
    g_gfxContext.vertexBuffers[handle] = std::move(vertexBuffer);
    SetVertexBufferName(handle);
    return handle;
}

VertexBufferHandle Graphics::CreateDynamicVertexBuffer(
    const Int32         _vertexCount,
    const VertexLayout& _vertexLayout)
{
    JAM_ASSERT(_vertexCount > 0, "Vertex count must be greater than 0");

    VertexBuffer vertexBuffer;
    Int32        stride       = _vertexLayout.GetStride();
    vertexBuffer.buffer       = g_gfxContext.CreateNativeBuffer(_vertexCount * stride, eBufferType::VertexBuffer, std::nullopt);
    vertexBuffer.vertexStride = stride;
    vertexBuffer.vertexCount  = _vertexCount;

    // 버텍스 레이아웃 참조
    vertexBuffer.layoutHandle = g_gfxContext.GetOrCreateNativeVertexLayout(_vertexLayout);

    // 이름 설정
    VertexBufferHandle handle          = g_gfxContext.vertexBuffers.Alloc();
    g_gfxContext.vertexBuffers[handle] = std::move(vertexBuffer);
    SetVertexBufferName(handle);
    return handle;
}

IndexBufferHandle Graphics::CreateIndexBuffer(
    MemoryView         _indexData,
    const eIndexFormat _format)
{
    Int32 stride      = _format == eIndexFormat::U16 ? 2 : 4;
    Int32 bufferWidth = NarrowCast<Int32>(_indexData.GetByteWidth());

    IndexBuffer indexBuffer;
    indexBuffer.buffer      = g_gfxContext.CreateNativeBuffer(bufferWidth, eBufferType::IndexBuffer, _indexData);
    indexBuffer.indexCount  = bufferWidth / stride;
    indexBuffer.indexStride = stride;
    indexBuffer.format      = _format;

    // 이름 설정
    IndexBufferHandle handle          = g_gfxContext.indexBuffers.Alloc();
    g_gfxContext.indexBuffers[handle] = std::move(indexBuffer);
    SetIndexBufferName(handle);
    return handle;
}

IndexBufferHandle Graphics::CreateDynamicIndexBuffer(
    const Int32        _indexCount,
    const eIndexFormat _format)
{
    JAM_ASSERT(_indexCount > 0, "Index count must be greater than 0");
    Int32 stride = GetIndexFormatByte(_format);

    IndexBuffer indexBuffer;
    indexBuffer.buffer      = g_gfxContext.CreateNativeBuffer(_indexCount * stride, eBufferType::IndexBuffer, std::nullopt);
    indexBuffer.indexCount  = _indexCount;
    indexBuffer.indexStride = stride;
    indexBuffer.format      = _format;

    // 이름 설정
    IndexBufferHandle handle          = g_gfxContext.indexBuffers.Alloc();
    g_gfxContext.indexBuffers[handle] = std::move(indexBuffer);
    SetIndexBufferName(handle);
    return handle;
}

ConstantBufferHandle Graphics::CreateConstantBuffer(const Int32 _byteWidth)
{
    JAM_ASSERT(_byteWidth > 0 && _byteWidth <= 65536, "Constant buffer size must be between 1 and 65536 bytes");
    JAM_ASSERT(_byteWidth % 16 == 0, "Constant buffer size must be a multiple of 16 bytes");

    ConstantBuffer cbuffer;
    cbuffer.buffer = g_gfxContext.CreateNativeBuffer(_byteWidth, eBufferType::ConstantBuffer, std::nullopt);

    // 이름 설정
    ConstantBufferHandle handle   = g_gfxContext.cbuffers.Alloc();
    g_gfxContext.cbuffers[handle] = std::move(cbuffer);
    SetConstantBufferName(handle);
    return handle;
}

TextureHandle Graphics::CreateTexture2D(
    const Int32                        _width,
    const Int32                        _height,
    const eFormat                      _format,
    const Int32                        _arraySize,
    const eTextureCreateFlags          _flags,
    const std::span<const MemoryView>& _initData)
{
    TextureHandle handle          = g_gfxContext.textures.Alloc();
    g_gfxContext.textures[handle] = g_gfxContext.CreateNativeTexture(_width, _height, 1, _format, _arraySize, _flags, eTextureType::Texture2D, _initData);
    SetTextureName(handle);
    return handle;
}
TextureHandle Graphics::CreateTextureCube(
    const Int32                        _width,
    const Int32                        _height,
    const eFormat                      _format,
    const Int32                        _cubeCount,
    const eTextureCreateFlags          _flags,
    const std::span<const MemoryView>& _initData)
{
    TextureHandle handle          = g_gfxContext.textures.Alloc();
    g_gfxContext.textures[handle] = g_gfxContext.CreateNativeTexture(_width, _height, 1, _format, _cubeCount * 6, _flags, eTextureType::TextureCube, _initData);
    SetTextureName(handle);
    return handle;
}

TextureHandle Graphics::CreateTexture3D(
    const Int32                        _width,
    const Int32                        _height,
    const Int32                        _depth,
    const eFormat                      _format,
    const eTextureCreateFlags          _texFlags,
    const std::span<const MemoryView>& _initData)
{
    TextureHandle handle          = g_gfxContext.textures.Alloc();
    g_gfxContext.textures[handle] = g_gfxContext.CreateNativeTexture(_width, _height, _depth, _format, 1, _texFlags, eTextureType::Texture3D, _initData);
    SetTextureName(handle);
    return handle;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const std::span<const TextureHandle> _handles,
    const bool                           _bDestroyTexture)
{
    FrameBufferHandle handle          = g_gfxContext.frameBuffers.Alloc();
    g_gfxContext.frameBuffers[handle] = g_gfxContext.CreateNativeFrameBuffer(_handles, _bDestroyTexture);
    SetFrameBufferName(handle);
    return handle;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const TextureHandle _handle,
    const bool          _bDestroyTexture)
{
    TextureHandle handles[1] = { _handle };
    return CreateFrameBuffer(handles, _bDestroyTexture);
}

FrameBufferHandle Graphics::CreateFrameBuffer(
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

FrameBufferHandle Graphics::CreateFrameBuffer(
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

ShaderHandle Graphics::CreateShader(
    const MemoryView _shaderByteCode)
{
    return g_gfxContext.GetOrCreateNativeShader(_shaderByteCode);
}

ProgramHandle Graphics::CreateProgram(
    const ShaderHandle _vsHandle,
    const ShaderHandle _psHandle,
    const ShaderHandle _gsHandle,
    const ShaderHandle _dsHandle,
    const ShaderHandle _hsHandle,
    const bool         _bDestroyShader)
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(_vsHandle) || JAM_IS_VALID_HANDLE(_psHandle), "At least one of VS or PS must be provided");

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

        if (!JAM_IS_VALID_HANDLE(handle))
        {
            continue;
        }

        ShaderRef& shader = g_gfxContext.shaders[handle];
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
    ProgramHandle handle          = g_gfxContext.programs.Alloc();
    g_gfxContext.programs[handle] = std::move(program);
    return handle;
}

ProgramHandle Graphics::CreateProgram(
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

void Graphics::DestroyVertexBuffer(const VertexBufferHandle _handle)
{
    VertexBuffer& vertexBuffer = g_gfxContext.vertexBuffers[_handle];

    // d3d 버텍스 버퍼 파괴
    UINT d3dRefCount = vertexBuffer.buffer.pBuffer->Release();
    if (d3dRefCount > 0)
    {
        JAM_ASSERT(0, "Vertex buffer reference count is not zero. RefCount: {}", d3dRefCount);
    }

    VertexLayoutHandle layoutHandle = vertexBuffer.layoutHandle;
    VertexLayoutRef&   vertexLayout = g_gfxContext.vertexLayouts[layoutHandle];

    // 참조 해제
    Int32 refCount = --vertexLayout.refCount;
    JAM_ASSERT(refCount >= 0, "VertexLayoutRef reference count is already zero");

    // 객체 파괴
    if (refCount == 0)
    {
        refCount = vertexLayout.pInputLayout->Release();   // d3d 리소스 파괴
        if (refCount > 0)
        {
            JAM_ASSERT(0, "Vertex layout.pInputLayout reference count is not zero. RefCount: {}", refCount);
        }

        g_gfxContext.vertexLayouts.Free(layoutHandle);             // 레이아웃 핸들 해제
        g_gfxContext.vertexLayoutCache.erase(vertexLayout.hash);   // 캐시에서 레이아웃 핸들 제거
    }

    g_gfxContext.vertexBuffers.Free(_handle);   // 핸들 해제
}

void Graphics::DestroyIndexBuffer(const IndexBufferHandle _handle)
{
    IndexBuffer& indexBuffer = g_gfxContext.indexBuffers[_handle];
    UINT         d3dRefCount = indexBuffer.buffer.pBuffer->Release();   // d3d 리소스 파괴
    if (d3dRefCount > 0)
    {
        JAM_ASSERT(0, "Index buffer.pBuffer reference count is not zero. RefCount: {}", d3dRefCount);
    }

    g_gfxContext.indexBuffers.Free(_handle);   // 핸들 해제
}

void Graphics::DestroyTexture(const TextureHandle _handle)
{
    // 텍스처 참조 해제
    TextureRef& texture  = g_gfxContext.textures[_handle];
    Int32       refCount = --texture.refCount;
    JAM_ASSERT(refCount >= 0, "TextureRef reference count is already zero");

    // 텍스처 파괴
    if (texture.refCount == 0)
    {
        if (texture.pSRV)   // SRV 파괴
        {
            UINT d3dRefCount = texture.pSRV->Release();
            if (d3dRefCount > 0)
            {
                JAM_ASSERT(0, "TextureRef shader resource view reference count is not zero. RefCount: {}", d3dRefCount);
            }
        }

        UINT d3dRefCount = texture.pResource->Release();   // 리소스 파괴
        if (d3dRefCount > 0)
        {
            JAM_ASSERT(0, "TextureRef resource reference count is not zero. RefCount: {}", d3dRefCount);
        }

        g_gfxContext.textures.Free(_handle);   // 핸들 해제
    }
}

void Graphics::DestroyFrameBuffer(const FrameBufferHandle _handle)
{
    FrameBuffer& frameBuffer = g_gfxContext.frameBuffers[_handle];

    // RTV 파괴
    for (Int32 i = 0; i < frameBuffer.renderTargetCount; ++i)
    {
        ID3D11RenderTargetView* pRTV     = frameBuffer.pRTV[i];
        UINT                    refCount = pRTV->Release();   // d3d 리소스 파괴
        if (refCount > 0)
        {
            JAM_ASSERT(0, "Render target view reference count is not zero. RefCount: {}", refCount);
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
            JAM_ASSERT(0, "Depth stencil view reference count is not zero. RefCount: {}", refCount);
        }

        DestroyTexture(frameBuffer.depthStencilTextureHandle);   // TextureHandle 해제
    }

    g_gfxContext.frameBuffers.Free(_handle);   // FrameBuffer 핸들 해제
}

void Graphics::DestroyProgram(const ProgramHandle _handle)
{
    Program& program = g_gfxContext.programs[_handle];

    for (ShaderHandle handle: program.shaderHandles)
    {
        if (JAM_IS_VALID_HANDLE(handle))
        {
            DestroyShader(handle);   // 셰이더 참조 해제
        }
    }

    g_gfxContext.programs.Free(_handle);   // Program 핸들 해제
}

void Graphics::DestroyShader(const ShaderHandle _handle)
{
    ShaderRef& shader = g_gfxContext.shaders[_handle];
    JAM_ASSERT(shader.refCount > 0, "ShaderRef reference count is already zero");

    // 참조 해제
    Int32 refCount = --shader.refCount;
    JAM_ASSERT(refCount >= 0, "ShaderRef reference count is already zero");

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
            JAM_ASSERT(0, "Shader reference count is not zero. RefCount: {}", refCount);
        }

        g_gfxContext.shaders.Free(_handle);                // Shader 핸들 해제
        g_gfxContext.shaderCache.erase(shader.cacheKey);   // 캐시에서 제거
    }
}

void Graphics::DestroyConstantBuffer(
    const ConstantBufferHandle _handle)
{
    ConstantBuffer& cbuffer  = g_gfxContext.cbuffers[_handle];
    UINT            refCount = cbuffer.buffer.pBuffer->Release();
    if (refCount > 0)
    {
        JAM_ASSERT(0, "Constant buffer.pBuffer reference count is not zero. RefCount: {}", refCount);
    }

    g_gfxContext.cbuffers.Free(_handle);   // 핸들 해제
}

void Graphics::SetVertexBufferName(
    const VertexBufferHandle _handle,
    const std::string_view   _name)
{
    VertexBuffer& vertexBuffer = g_gfxContext.vertexBuffers[_handle];
    vertexBuffer.name          = g_gfxContext.CreateNativeResourceName("VertexBuffer", _handle, _name);
    g_gfxContext.SetD3D11Name(vertexBuffer.buffer.pBuffer, vertexBuffer.name);
}

void Graphics::SetIndexBufferName(
    const IndexBufferHandle _handle,
    const std::string_view  _name)
{
    IndexBuffer& indexBuffer = g_gfxContext.indexBuffers[_handle];
    indexBuffer.name         = g_gfxContext.CreateNativeResourceName("IndexBuffer", _handle, _name);
    g_gfxContext.SetD3D11Name(indexBuffer.buffer.pBuffer, indexBuffer.name);
}

void Graphics::SetConstantBufferName(
    const ConstantBufferHandle _handle,
    const std::string_view     _name)
{
    ConstantBuffer& cbuffer = g_gfxContext.cbuffers[_handle];
    cbuffer.name            = g_gfxContext.CreateNativeResourceName("ConstantBuffer", _handle, _name);
    g_gfxContext.SetD3D11Name(cbuffer.buffer.pBuffer, cbuffer.name);
}

void Graphics::SetTextureName(
    const TextureHandle    _handle,
    const std::string_view _name)
{
    constexpr const char* const k_names[EnumCount<eTextureType>()] = {
        "Texture2D",
        "Texture3D",
        "TextureCube",
    };

    TextureRef& texture = g_gfxContext.textures[_handle];
    texture.name        = g_gfxContext.CreateNativeResourceName(k_names[EnumToInt(texture.type)], _handle, _name);
    g_gfxContext.SetD3D11Name(texture.pResource, texture.name);

    if (texture.pSRV)
    {
        constexpr const char* const k_srvNames[EnumCount<eTextureType>()] = {
            "Texture2D_SRV",
            "Texture3D_SRV",
            "TextureCube_SRV",
        };

        std::string name = g_gfxContext.CreateNativeResourceName(k_srvNames[EnumToInt(texture.type)], _handle, _name);
        g_gfxContext.SetD3D11Name(texture.pSRV, name);
    }
}

void Graphics::SetFrameBufferName(
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

    FrameBuffer& frameBuffer = g_gfxContext.frameBuffers[_handle];
    frameBuffer.name         = g_gfxContext.CreateNativeResourceName("FrameBuffer", _handle, _name);

    for (Int32 i = 0; i < frameBuffer.renderTargetCount; ++i)
    {
        if (frameBuffer.pRTV[i])
        {
            std::string name = g_gfxContext.CreateNativeResourceName(k_names[i], _handle, _name);
            g_gfxContext.SetD3D11Name(frameBuffer.pRTV[i], name);
        }
    }

    if (frameBuffer.pDSV)
    {
        std::string name = g_gfxContext.CreateNativeResourceName("FrameBuffer_DSV", _handle, _name);
        g_gfxContext.SetD3D11Name(frameBuffer.pDSV, name);
    }
}

void Graphics::SetShaderName(
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

    ShaderRef& shader = g_gfxContext.shaders[_handle];
    shader.name       = g_gfxContext.CreateNativeResourceName(k_names[EnumToInt(shader.stage)], _handle, _name);

    switch (shader.stage)
    {
        case eShader::VertexShader:
            g_gfxContext.SetD3D11Name(shader.pVertexShader, shader.name);
            break;
        case eShader::PixelShader:
            g_gfxContext.SetD3D11Name(shader.pPixelShader, shader.name);
            break;
        case eShader::GeometryShader:
            g_gfxContext.SetD3D11Name(shader.pGeometryShader, shader.name);
            break;
        case eShader::HullShader:
            g_gfxContext.SetD3D11Name(shader.pHullShader, shader.name);
            break;
        case eShader::DomainShader:
            g_gfxContext.SetD3D11Name(shader.pDomainShader, shader.name);
            break;
        case eShader::ComputeShader:
            g_gfxContext.SetD3D11Name(shader.pComputeShader, shader.name);
            break;
        default:
            JAM_ASSERT(0, "Unsupported shader stage");
    }
}

TextureHandle Graphics::LoadTexture(
    const DirectX::ScratchImage& _scratchImage)
{
    ID3D11Resource* pResource = nullptr;
    JAM_CHECK_COM(DirectX::CreateTexture(g_gfxContext.pd3d11Device,
                                         _scratchImage.GetImages(),
                                         _scratchImage.GetImageCount(),
                                         _scratchImage.GetMetadata(),
                                         &pResource));

    const DirectX::TexMetadata& metaData = _scratchImage.GetMetadata();

    TextureRef texture;
    texture.width     = NarrowCast<Int32>(metaData.width);
    texture.height    = NarrowCast<Int32>(metaData.height);
    texture.depth     = NarrowCast<Int32>(metaData.depth);
    texture.format    = g_gfxContext.GetNativeFormatFromDXGIFormat(metaData.format);
    texture.arraySize = NarrowCast<Int32>(metaData.arraySize);
    texture.samples   = 1;
    texture.mipsCount = NarrowCast<Int32>(metaData.mipLevels);
    texture.refCount  = 1;
    texture.bManaged  = false;
    texture.bMutable  = false;
    texture.pResource = pResource;

    if (metaData.IsCubemap())
        texture.type = eTextureType::TextureCube;
    else if (metaData.dimension == DirectX::TEX_DIMENSION_TEXTURE3D)
        texture.type = eTextureType::Texture3D;
    else if (metaData.dimension == DirectX::TEX_DIMENSION_TEXTURE2D)
        texture.type = eTextureType::Texture2D;
    else
        JAM_ASSERT(0, "Unsupported texture dimension");

    JAM_CHECK_COM(DirectX::CreateShaderResourceView(g_gfxContext.pd3d11Device,
                                                    _scratchImage.GetImages(),
                                                    _scratchImage.GetImageCount(),
                                                    _scratchImage.GetMetadata(),
                                                    &texture.pSRV));

    TextureHandle handle          = g_gfxContext.textures.Alloc();
    g_gfxContext.textures[handle] = std::move(texture);
    SetTextureName(handle);
    return handle;
}

Result<TextureHandle> Graphics::LoadTexture(
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

void Graphics::UploadTexture2D(
    const TextureHandle _handle,
    const MemoryView    _uploadTexData,
    const Int32         _uploadTexWidth,
    const Int32         _uploadTexHeight,
    const Int32         _mipIndex,
    const Int32         _arrayIndex,
    const Int32         _xOffset,
    const Int32         _yOffset)
{
    const TextureRef& texture = g_gfxContext.textures[_handle];
    g_gfxContext.UploadTexture(
        texture,
        _uploadTexData,
        _uploadTexWidth,
        _uploadTexHeight,
        1,
        _mipIndex,
        _arrayIndex,
        _xOffset,
        _yOffset,
        0);
}

void Graphics::UploadTextureCube(
    const TextureHandle _handle,
    const MemoryView    _uploateTexData,
    const Int32         _uploadTexWidth,
    const Int32         _uploadTexHeight,
    const Int32         _mipIndex,
    const Int32         _cubeIndex,
    const eCubeFace     _cubeFace,
    const Int32         _xOffset,
    const Int32         _yOffset)
{
    const TextureRef& texture = g_gfxContext.textures[_handle];
    g_gfxContext.UploadTexture(
        texture,
        _uploateTexData,
        _uploadTexWidth,
        _uploadTexHeight,
        1,
        _mipIndex,
        _cubeIndex * 6 + k_d3d11CubeFaceTable[EnumToInt(_cubeFace)],
        _xOffset,
        _yOffset,
        0);
}

void Graphics::UploadTexture3D(
    const TextureHandle _handle,
    const MemoryView    _uplodeTexData,
    const Int32         _uploadTexWidth,
    const Int32         _uploadTexHeight,
    const Int32         _uploadTexDepth,
    const Int32         _mipIndex,
    const Int32         _xOffset,
    const Int32         _yOffset,
    const Int32         _zOffset)
{
    const TextureRef& texture = g_gfxContext.textures[_handle];
    g_gfxContext.UploadTexture(
        texture,
        _uplodeTexData,
        _uploadTexWidth,
        _uploadTexHeight,
        _uploadTexDepth,
        _mipIndex,
        0,
        _xOffset,
        _yOffset,
        _zOffset);
}

std::vector<UInt8> Graphics::DownloadTexture2D(
    const TextureHandle _handle,
    const Int32         _mipIndex,
    const Int32         _arrayIndex)
{
    const TextureRef& texture = g_gfxContext.textures[_handle];
    return g_gfxContext.DownloadTexture(
        texture,
        _mipIndex,
        _arrayIndex);
}

std::vector<UInt8> Graphics::DownloadTextureCube(
    const TextureHandle _handle,
    const Int32         _mipIndex,
    const Int32         _cubeIndex,
    const eCubeFace     _cubeFace)
{
    const TextureRef& texture = g_gfxContext.textures[_handle];
    return g_gfxContext.DownloadTexture(
        texture,
        _mipIndex,
        _cubeIndex * 6 + k_d3d11CubeFaceTable[EnumToInt(_cubeFace)]);
}

std::vector<UInt8> Graphics::DownloadTexture3D(
    const TextureHandle _handle,
    const Int32         _mipIndex)
{
    const TextureRef& texture = g_gfxContext.textures[_handle];
    return g_gfxContext.DownloadTexture(
        texture,
        _mipIndex,
        0);
}

DirectX::ScratchImage Graphics::Capture(
    const TextureHandle _handle)
{
    const TextureRef&     texture = g_gfxContext.textures[_handle];
    DirectX::ScratchImage scratchImage;
    JAM_CHECK_COM(DirectX::CaptureTexture(g_gfxContext.pd3d11Device, g_gfxContext.pd3d11DeviceContext, texture.pResource, scratchImage));
    return scratchImage;
}

void Graphics::Resolve(
    const TextureHandle _srcHandle,
    const Int32         _srcArrayIndex,
    const TextureHandle _dstHandle,
    const Int32         _dstArrayIndex)
{
    TextureRef& srcTexture = g_gfxContext.textures[_srcHandle];
    TextureRef& dstTexture = g_gfxContext.textures[_dstHandle];
    JAM_ASSERT(dstTexture.samples == 1, "Destination texture must not be multi-sampled.");

    Int32 srcIndex = srcTexture.mipsCount * _srcArrayIndex;
    Int32 dstIndex = dstTexture.mipsCount * _dstArrayIndex;
    g_gfxContext.pd3d11DeviceContext->ResolveSubresource(dstTexture.pResource,
                                                         dstIndex,
                                                         srcTexture.pResource,
                                                         srcIndex,
                                                         k_formatInfoTable[EnumToInt(dstTexture.format)].texFormat);
}

void Graphics::Blit(
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
    const TextureRef& srcTexture = g_gfxContext.textures[_srcHandle];
    const TextureRef& dstTexture = g_gfxContext.textures[_dstHandle];
    g_gfxContext.Blit(srcTexture,
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

void Graphics::BlitToBackBuffer(
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
    const TextureRef& srcTexture = g_gfxContext.textures[_srcHandle];
    const TextureRef& dstTexture = g_gfxContext.textures[g_gfxContext.backBufferTextureHandle];
    JAM_ASSERT(srcTexture.type == eTextureType::Texture2D, "Source texture must be 2D texture.");
    g_gfxContext.Blit(srcTexture, dstTexture, _blitWidthOrZero, _blitHeightOrZero, 1, _srcMipIndex, _srcArrayIndex, _srcX, _srcY, 0, 0, 0, _dstX, _dstY, 0);
}

TextureResolution Graphics::GetTextureResolution(const TextureHandle _handle, const Int32 _mipIndex)
{
    const TextureRef& texture = g_gfxContext.textures[_handle];
    TextureResolution res;
    res.width  = Max(1, texture.width >> _mipIndex);
    res.height = Max(1, texture.height >> _mipIndex);
    res.depth  = Max(1, texture.depth >> _mipIndex);
    return res;
}

void Graphics::ClearRenderTarget(const FrameBufferHandle _handle, const RGBA _color, const Int32 _renderTargetIndex)
{
    FrameBufferHandle handle      = JAM_IS_VALID_HANDLE(_handle) ? _handle : g_gfxContext.frameBufferHandleBind;
    FrameBuffer&      frameBuffer = g_gfxContext.frameBuffers[handle];
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < frameBuffer.renderTargetCount, "Render target m_index out of range.");
    FLOAT colors[4] = {
        static_cast<float>(_color.r) / 255.0f,
        static_cast<float>(_color.g) / 255.0f,
        static_cast<float>(_color.b) / 255.0f,
        static_cast<float>(_color.a) / 255.0f
    };
    g_gfxContext.pd3d11DeviceContext->ClearRenderTargetView(frameBuffer.pRTV[_renderTargetIndex], colors);
}

void Graphics::ClearDepthStencil(
    const FrameBufferHandle _handle,
    const bool              _bClearDepth,
    const bool              _bClearStencil,
    const float             _depth,
    const UInt8             _stencil)
{
    FrameBufferHandle handle      = JAM_IS_VALID_HANDLE(_handle) ? _handle : g_gfxContext.frameBufferHandleBind;
    FrameBuffer&      frameBuffer = g_gfxContext.frameBuffers[handle];
    JAM_ASSERT(frameBuffer.pDSV, "Frame buffer does not have a depth stencil view.");

    UINT clearFlags = 0;
    if (_bClearDepth)
        clearFlags |= D3D11_CLEAR_DEPTH;
    if (_bClearStencil)
        clearFlags |= D3D11_CLEAR_STENCIL;

    g_gfxContext.pd3d11DeviceContext->ClearDepthStencilView(frameBuffer.pDSV, clearFlags, _depth, _stencil);
}

const TextureHandle& Graphics::GetRenderTargetTexture(
    const FrameBufferHandle _handle,
    const Int32             _renderTargetIndex)
{
    const FrameBuffer& frameBuffer = g_gfxContext.frameBuffers[_handle];
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < frameBuffer.renderTargetCount, "Render target m_index out of range.");
    return frameBuffer.renderTargetTextureHandles[_renderTargetIndex];
}

const TextureHandle& Graphics::GetDepthStencilTexture(
    const FrameBufferHandle _handle)
{
    const FrameBuffer& frameBuffer = g_gfxContext.frameBuffers[_handle];
    JAM_ASSERT(frameBuffer.pDSV, "Frame buffer does not have a depth stencil view.");
    return frameBuffer.depthStencilTextureHandle;
}

const VertexLayout& Graphics::GetVertexLayout(const VertexLayoutHandle _handle)
{
    const VertexLayoutRef& layoutRef = g_gfxContext.vertexLayouts[_handle];
    return layoutRef.layout;
}

VertexBufferInfo Graphics::GetVertexBufferInfo(const VertexBufferHandle _handle)
{
    const VertexBuffer& vertexBuffer = g_gfxContext.vertexBuffers[_handle];

    VertexBufferInfo info;
    info.byteWidth    = vertexBuffer.buffer.byteWidth;
    info.vertexCount  = vertexBuffer.vertexCount;
    info.vertexStride = vertexBuffer.vertexStride;
    info.bDynamic     = vertexBuffer.buffer.bDynamic;
    info.layoutHandle = vertexBuffer.layoutHandle;
    return info;
}

IndexBufferInfo Graphics::GetIndexBufferInfo(const IndexBufferHandle _handle)
{
    const IndexBuffer& indexBuffer = g_gfxContext.indexBuffers[_handle];

    IndexBufferInfo info;
    info.byteWidth  = indexBuffer.buffer.byteWidth;
    info.indexCount = indexBuffer.indexCount;
    info.format     = indexBuffer.format;
    info.bDynamic   = indexBuffer.buffer.bDynamic;
    return info;
}

ConstantBufferInfo Graphics::GetConstantBufferInfo(const ConstantBufferHandle _handle)
{
    const ConstantBuffer& cbuffer = g_gfxContext.cbuffers[_handle];

    ConstantBufferInfo info;
    info.byteWidth = cbuffer.buffer.byteWidth;
    return info;
}

TextureInfo Graphics::GetTextureInfo(const TextureHandle _handle)
{
    const TextureRef& texture = g_gfxContext.textures[_handle];

    TextureInfo info;
    info.width                   = texture.width;
    info.height                  = texture.height;
    info.depth                   = texture.depth;
    info.type                    = texture.type;
    info.format                  = texture.format;
    info.arraySize               = texture.arraySize;
    info.samples                 = texture.samples;
    info.mipsCount               = texture.mipsCount;
    info.bMutable                = texture.bMutable;
    info.bRenderTargetAttachable = texture.bRTVAttachable;
    info.bDepthStencilAttachable = texture.bDSVAttachable;
    return info;
}

FrameBufferInfo Graphics::GetFrameBufferInfo(const FrameBufferHandle _handle)
{
    const FrameBuffer& frameBuffer = g_gfxContext.frameBuffers[_handle];

    FrameBufferInfo info;
    info.renderTargetCount = frameBuffer.renderTargetCount;
    info.bHasDepthStencil  = JAM_IS_VALID_HANDLE(frameBuffer.depthStencilTextureHandle);
    std::memcpy(&info.renderTargetTextureHandles, frameBuffer.renderTargetTextureHandles, sizeof(frameBuffer.renderTargetTextureHandles));
    info.depthStencilTextureHandle = frameBuffer.depthStencilTextureHandle;
    return info;
}

ShaderInfo Graphics::GetShaderInfo(const ShaderHandle _handle)
{
    const ShaderRef& shader = g_gfxContext.shaders[_handle];

    ShaderInfo info;
    info.stage = shader.stage;
    return info;
}

ProgramInfo Graphics::GetProgramInfo(const ProgramHandle _handle)
{
    const Program& program = g_gfxContext.programs[_handle];
    ProgramInfo    info;
    std::memcpy(info.shaderHandles, program.shaderHandles, sizeof(program.shaderHandles));
    return info;
}

void Graphics::UploadBuffer(
    const VertexBufferHandle _handle,
    const MemoryView         _writeData,
    const Int32              _offset,
    const bool               _bDiscard)
{
    const VertexBuffer& buffer = g_gfxContext.vertexBuffers[_handle];
    g_gfxContext.UploadBuffer(buffer.buffer, _writeData, _offset, _bDiscard);
}

void Graphics::UploadBuffer(
    const IndexBufferHandle _handle,
    const MemoryView        _writeData,
    const Int32             _offset,
    const bool              _bDiscard)
{
    const IndexBuffer& buffer = g_gfxContext.indexBuffers[_handle];
    g_gfxContext.UploadBuffer(buffer.buffer, _writeData, _offset, _bDiscard);
}

void Graphics::UploadBuffer(
    const ConstantBufferHandle _handle,
    const MemoryView           _writeData)
{
    ConstantBuffer& buffer = g_gfxContext.cbuffers[_handle];
    g_gfxContext.UploadBuffer(buffer.buffer, _writeData, 0, true);
}

std::vector<UInt8> Graphics::DownloadBuffer(
    const VertexBufferHandle _handle)
{
    const VertexBuffer& buffer = g_gfxContext.vertexBuffers[_handle];
    return g_gfxContext.DownloadBuffer(buffer.buffer);
}

std::vector<UInt8> Graphics::DownloadBuffer(
    const IndexBufferHandle _handle)
{
    const IndexBuffer& buffer = g_gfxContext.indexBuffers[_handle];
    return g_gfxContext.DownloadBuffer(buffer.buffer);
}

std::vector<Byte> Graphics::DownloadBuffer(
    const ConstantBufferHandle _handle)
{
    const ConstantBuffer& buffer = g_gfxContext.cbuffers[_handle];
    return g_gfxContext.DownloadBuffer(buffer.buffer);
}

const VertexLayout& Graphics::GetVertexLayout(
    const VertexBufferHandle _handle)
{
    const VertexBuffer&    buffer    = g_gfxContext.vertexBuffers[_handle];
    const VertexLayoutRef& layoutRef = g_gfxContext.vertexLayouts[buffer.layoutHandle];
    return layoutRef.layout;
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _handleOrInvalid)
{
    const VertexBuffer& buffer = g_gfxContext.vertexBuffers[_handleOrInvalid];
    SetVertexBuffer(_handleOrInvalid, buffer.vertexCount, 0);
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _handleOrInvalid,
    const Int32              _vertexCount,
    const Int32              _vertexOffset)
{
    if (g_gfxContext.vertexBufferHandleBind != _handleOrInvalid || g_gfxContext.vertexBufferOffsetBind != _vertexOffset)
    {
        const VertexBuffer& buffer = g_gfxContext.vertexBuffers[_handleOrInvalid];
        if (g_gfxContext.vertexLayoutHandleBind != buffer.layoutHandle)
        {
            g_gfxContext.vertexLayoutHandleBind = buffer.layoutHandle;
            g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_InputLayout;
        }

        g_gfxContext.vertexBufferHandleBind = _handleOrInvalid;
        g_gfxContext.vertexBufferOffsetBind = _vertexOffset;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_VertexBuffer;
    }
    g_gfxContext.vertexCountBind = _vertexCount;
}

void Graphics::SetIndexBuffer(const IndexBufferHandle _handleOrInvalid)
{
    const IndexBuffer& buffer = g_gfxContext.indexBuffers[_handleOrInvalid];
    SetIndexBuffer(_handleOrInvalid, buffer.indexCount, 0);
}

void Graphics::SetIndexBuffer(
    const IndexBufferHandle _handleOrInvalid,
    const Int32             _indexCount,
    const Int32             _indexOffset)
{
    if (g_gfxContext.indexBufferHandleBind != _handleOrInvalid)
    {
        g_gfxContext.indexBufferHandleBind = _handleOrInvalid;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_IndexBuffer;
    }

    g_gfxContext.indexBufferOffsetBind = _indexOffset;
    g_gfxContext.indexCountBind        = _indexCount;
}

void Graphics::SetConstantBuffer(
    const ConstantBufferHandle _handleOrInvalid,
    const eShader              _shader,
    const Int32                _slot)
{
    const Int32          stage = EnumToInt(_shader);
    ConstantBufferBinds& binds = g_gfxContext.cbufferHandleBinds[stage];

    if (binds.cbufferHandles[_slot] != _handleOrInvalid)
    {
        binds.cbufferHandles[_slot] = _handleOrInvalid;
        binds.dirtyStart            = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd              = Max(binds.dirtyEnd, _slot);
        g_gfxContext.pipelineDirtyFlags |= (ePipelineDirtyFlags_VSConstantBuffer << stage);
    }
}

void Graphics::SetTexture(
    const TextureHandle _handleOrInvalid,
    const eShader       _shader,
    const Int32         _slot)
{
    const Int32   stage = EnumToInt(_shader);
    TextureBinds& binds = g_gfxContext.textureHandleBinds[stage];

    if (binds.textureHandles[_slot] != _handleOrInvalid)
    {
        binds.textureHandles[_slot] = _handleOrInvalid;
        binds.dirtyStart            = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd              = Max(binds.dirtyEnd, _slot);
        g_gfxContext.pipelineDirtyFlags |= (ePipelineDirtyFlags_VSShaderResourceView << stage);
    }
}

void Graphics::SetSampler(
    const SamplerState _state,
    const eShader      _shader,
    const Int32        _slot)
{
    const Int32        shaderIdx = EnumToInt(_shader);
    SamplerFlagsBinds& binds     = g_gfxContext.samplerFlagsBinds[shaderIdx];

    if (!MemEqual(binds.samplerStates[_slot], _state))
    {
        binds.samplerStates[_slot] = _state;
        binds.dirtyStart           = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd             = Max(binds.dirtyEnd, _slot);
        g_gfxContext.pipelineDirtyFlags |= (ePipelineDirtyFlags_VSSamplerState << shaderIdx);
    }
}

void Graphics::SetBlendFactor(const RGBA _rgba)
{
    if (g_gfxContext.blendFactorBind.color != _rgba.color)
    {
        g_gfxContext.blendFactorBind = _rgba;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_BlendState;
    }
}

void Graphics::SetStencil(
    const StencilState _fStencil,
    const StencilState _bStencil,
    const UInt8        _stencilRef)
{
    if (!MemEqual(g_gfxContext.depthStencilStateBind.fStencil, _fStencil))
    {
        g_gfxContext.depthStencilStateBind.fStencil = _fStencil;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
    }

    if (!MemEqual(g_gfxContext.depthStencilStateBind.bStencil, _bStencil))
    {
        g_gfxContext.depthStencilStateBind.bStencil = _bStencil;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
    }

    if (g_gfxContext.stencilRefBind != _stencilRef)
    {
        g_gfxContext.stencilRefBind = _stencilRef;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
    }
}

void Graphics::SetBorderColor(
    const eShader _shader,
    const Int32   _slot,
    const RGBA    _rgba)
{
    const Int32        shaderIndex = EnumToInt(_shader);
    SamplerFlagsBinds& binds       = g_gfxContext.samplerFlagsBinds[shaderIndex];
    if (binds.borderColors[_slot].color != _rgba.color)
    {
        binds.borderColors[_slot] = _rgba;
        binds.dirtyStart          = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd            = Max(binds.dirtyEnd, _slot);
        g_gfxContext.pipelineDirtyFlags |= (ePipelineDirtyFlags_VSSamplerState << shaderIndex);
    }
}

void Graphics::SetGfxState(const GfxState _state)
{
    if (!MemEqual(g_gfxContext.stateBind, _state))
    {
        g_gfxContext.stateBind = _state;

        // depth stencil state
        DepthStencilStateD3D11 dss = g_gfxContext.depthStencilStateBind;
        dss.bDepthWrite            = _state.bDepthWrite;
        dss.depthTestFunc          = _state.depthTestFunc;
        if (!MemEqual(g_gfxContext.depthStencilStateBind, dss))
        {
            g_gfxContext.depthStencilStateBind = dss;
            g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
        }

        // rasterizer state
        RasterizerStateD3D11 rs;
        rs.cullMode     = _state.cullMode;
        rs.bWireframe   = _state.bWireframe;
        rs.bDepthClamp  = _state.bDepthClamp;
        rs.bMultiSample = _state.bMultiSample;
        rs.bLineAA      = _state.bLineAA;
        rs.bScissor     = _state.bScissor;
        rs.bCCWFront    = _state.bCCWFront;
        if (!MemEqual(g_gfxContext.rasterizerStateBind, rs))
        {
            g_gfxContext.rasterizerStateBind = rs;
            g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_RasterizerState;
        }

        // blend state
        BlendStateD3D11 bs   = g_gfxContext.blendStateBind;
        bs.bAlphaToCoverage  = _state.bAlphaToCoverage;
        bs.bIndependentBlend = _state.bIndependentBlend;
        if (!MemEqual(g_gfxContext.blendStateBind, bs))
        {
            g_gfxContext.blendStateBind = bs;
            g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_BlendState;
        }

        // primitive
        if (g_gfxContext.primitiveBind != _state.primitive)
        {
            g_gfxContext.primitiveBind = _state.primitive;
            g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_Primitive;
        }
    }
}

void Graphics::SetBlendState(
    const BlendState _state,
    const Int32      _renderTargetIndex)
{
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < k_renderTargetCountMax, "Render target index out of range.");
    BlendState bs = g_gfxContext.blendStateBind.renderTarget[_renderTargetIndex];
    if (!MemEqual(bs, _state))
    {
        g_gfxContext.blendStateBind.renderTarget[_renderTargetIndex] = _state;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_BlendState;
    }
}

void Graphics::SetProgram(const ProgramHandle _handleOrInvalid)
{
    if (g_gfxContext.programHandleBind != _handleOrInvalid)
    {
        g_gfxContext.programHandleBind = _handleOrInvalid;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_Program;
    }
}

void Graphics::SetFrameBuffer(const FrameBufferHandle _handleOrInvalid)
{
    FrameBufferHandle handle = JAM_IS_VALID_HANDLE(_handleOrInvalid) ? _handleOrInvalid : g_gfxContext.backBufferFrameBufferHandle;
    if (g_gfxContext.frameBufferHandleBind != handle)
    {
        g_gfxContext.frameBufferHandleBind = handle;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_FrameBuffer;
    }
}

void Graphics::SetViewport(const Viewport _viewport)
{
    if (g_gfxContext.viewportBind.viewport != _viewport.viewport)
    {
        g_gfxContext.viewportBind = _viewport;
        g_gfxContext.pipelineDirtyFlags |= ePipelineDirtyFlags_Viewport;
    }
}

void Graphics::SetViewport(const UInt16 _x, const UInt16 _y, const UInt16 _width, const UInt16 _height)
{
    SetViewport(Viewport { _x, _y, _width, _height });
}

void Graphics::Flush()
{
    g_gfxContext.ApplyPipeline();
}

ShaderHandle Graphics::GetShader(
    const ProgramHandle _programHandle,
    const eShader       _shaderType)
{
    const Program&     program = g_gfxContext.programs[_programHandle];
    const ShaderHandle handle  = program.shaderHandles[EnumToInt(_shaderType)];
    JAM_ASSERT(JAM_IS_VALID_HANDLE(handle), "Program does not have the specified shader stage.");
    return handle;
}

void Graphics::Submit()
{
    g_gfxContext.ApplyPipeline();
    if (JAM_IS_VALID_HANDLE(g_gfxContext.indexBufferHandleBind))
    {
        g_gfxContext.pd3d11DeviceContext->DrawIndexed(g_gfxContext.indexCountBind, g_gfxContext.indexBufferOffsetBind, 0);
    }
    else
    {
        g_gfxContext.pd3d11DeviceContext->Draw(g_gfxContext.vertexCountBind, 0);
    }
}

}   // namespace jam