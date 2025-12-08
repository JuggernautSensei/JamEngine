#include "pch.h"

#include "Graphics.h"

#include "BaseUtilities.h"
#include "Debug.h"

#include <d3d11shader.h>

#include "Event.h"
#include "Hash.h"
#include "ImageImporter.h"
#include "ResourcePool.h"
#include "ShaderCompiler.h"
#include "WindowsPlatform.h"

#include <d3dcompiler.h>

namespace jam
{

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
    ePipelineDirtyFlags_ScissorRect       = 1 << 26,

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
using ePipelineDirtyFlags = EnumIntType<ePipelineDirtyFlags_>;

enum class eBufferType
{
    VertexBuffer = 0,
    IndexBuffer,
    ConstantBuffer
};

struct SamplerInfo
{
    D3D11_FILTER filter           = {};
    D3D11_FILTER comparisonFilter = {};
};

constexpr SamplerInfo k_samplerInfoTable[EnumCount<eTextureFilter>()] = {

    {               D3D11_FILTER_MIN_MAG_MIP_POINT,               D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT },
    {        D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,        D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR },
    {  D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,  D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT },
    {        D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,        D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR },
    {        D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,        D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT },
    { D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR },
    {        D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,        D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT },
    {              D3D11_FILTER_MIN_MAG_MIP_LINEAR,              D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR },
    {                     D3D11_FILTER_ANISOTROPIC,                     D3D11_FILTER_COMPARISON_ANISOTROPIC },
    {                     D3D11_FILTER_ANISOTROPIC,                     D3D11_FILTER_COMPARISON_ANISOTROPIC },
    {                     D3D11_FILTER_ANISOTROPIC,                     D3D11_FILTER_COMPARISON_ANISOTROPIC }
};

constexpr D3D11_BLEND k_d3d11BlendTable[EnumCount<eBlend>()] = {
    D3D11_BLEND_ZERO,
    D3D11_BLEND_ONE,
    D3D11_BLEND_SRC_COLOR,
    D3D11_BLEND_INV_SRC_COLOR,
    D3D11_BLEND_SRC_ALPHA,
    D3D11_BLEND_INV_SRC_ALPHA,
    D3D11_BLEND_DEST_ALPHA,
    D3D11_BLEND_INV_DEST_ALPHA,
    D3D11_BLEND_DEST_COLOR,
    D3D11_BLEND_INV_DEST_COLOR,
    D3D11_BLEND_SRC_ALPHA_SAT,
    D3D11_BLEND_BLEND_FACTOR,
    D3D11_BLEND_INV_BLEND_FACTOR
};

constexpr D3D11_BLEND_OP k_d3d11BlendOpTable[EnumCount<eBlendOp>()] = {
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_SUBTRACT,
    D3D11_BLEND_OP_REV_SUBTRACT,
    D3D11_BLEND_OP_MIN,
    D3D11_BLEND_OP_MAX
};

constexpr D3D11_COMPARISON_FUNC k_d3d11ComparisonFuncTable[EnumCount<eCompareFunc>()] = {
    static_cast<D3D11_COMPARISON_FUNC>(0),
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

constexpr D3D11_TEXTURECUBE_FACE k_d3d11CubeFaceTable[k_numCubeFaces] = {
    D3D11_TEXTURECUBE_FACE_POSITIVE_X,
    D3D11_TEXTURECUBE_FACE_NEGATIVE_X,
    D3D11_TEXTURECUBE_FACE_POSITIVE_Y,
    D3D11_TEXTURECUBE_FACE_NEGATIVE_Y,
    D3D11_TEXTURECUBE_FACE_POSITIVE_Z,
    D3D11_TEXTURECUBE_FACE_NEGATIVE_Z
};

constexpr DXGI_FORMAT k_dxgiFormatAttributeTable[EnumCount<eAttributeFormat>()] = {
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UINT,
};

constexpr D3D11_PRIMITIVE_TOPOLOGY k_d3d11PrimitiveTopologyTable[EnumCount<ePrimitive>()] = {
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
    D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
};

static constexpr const char* k_hlslFormatNames[EnumCount<eAttributeFormat>()] = {
    "float",
    "float2",
    "float3",
    "float4",
    "float4",
    "uint4"
};

static constexpr const char* k_hlslSemanticTable[EnumCount<eVertexAttribute>()] = {
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
    "TEXCOORD"
};

static constexpr Int32 k_d3d11IndexTable[EnumCount<eVertexAttribute>()] = {
    0,
    0,
    0,
    0,
    0,
    1,
    2,
    3,
    0,
    0,
    0,
    1,
    2,
    3
};

struct ConstantBufferBinds
{
    ConstantBufferBinds()
    {
        std::ranges::fill(cbhs, k_invalidHandle);
    }

    ConstantBufferHandle cbhs[k_numConstantBufferLimit] = {};
    Int32                dirtyStart                     = k_int32Max;
    Int32                dirtyEnd                       = 0;
};

struct TextureBinds
{
    TextureBinds()
    {
        std::ranges::fill(texhs, k_invalidHandle);
    }

    TextureHandle texhs[k_numShaderResourceLimit] = {};
    Int32         dirtyStart                      = k_int32Max;
    Int32         dirtyEnd                        = 0;
};

struct SamplerBinds
{
    SamplerBinds()
    {
        SamplerState invalid;
        invalid.value = 0xFFFFFFFF;
        std::ranges::fill(samplers, invalid);
        std::ranges::fill(borderColors, RGBA::k_black);
    }

    SamplerState samplers[k_numSamplerLimit]     = {};
    RGBA         borderColors[k_numSamplerLimit] = {};
    Int32        dirtyStart                      = k_int32Max;
    Int32        dirtyEnd                        = 0;
};

struct RasterizerStateD3D11
{
    RasterizerStateD3D11()
    {
        cullMode     = static_cast<eCullMode>(0xFF);   // invalid
        bWireframe   = false;
        bDepthClamp  = false;
        bMultiSample = false;
        bLineAA      = false;
        bScissor     = false;
        bCCWFront    = false;
    }

    eCullMode cullMode;
    bool      bWireframe : 1;
    bool      bDepthClamp : 1;
    bool      bMultiSample : 1;
    bool      bLineAA : 1;
    bool      bScissor : 1;
    bool      bCCWFront : 1;
};

struct BlendStateD3D11
{
    BlendStateD3D11()
    {
        BlendState invalid;
        invalid.value = 0xFFFFFFFF;
        std::ranges::fill(blends, invalid);

        bIndependentBlend = false;
        bAlphaToCoverage  = false;
    }

    bool       bIndependentBlend : 1;
    bool       bAlphaToCoverage : 1;
    BlendState blends[k_numRenderTargetLimit];
};

struct DepthStencilStateD3D11
{
    DepthStencilStateD3D11()
    {
        StencilState invalid;
        invalid.value = 0xFFFF;

        bDepthWrite   = false;
        depthTestFunc = eCompareFunc::Less;
        fStencil      = invalid;
        bStencil      = invalid;
    }

    bool         bDepthWrite : 1;
    eCompareFunc depthTestFunc : 4;
    StencilState fStencil;
    StencilState bStencil;
};

struct GfxContext
{
    ////////// PIPELINE //////////

    void ApplyConstantBuffers(eShader _stage);
    void ApplyShaderResourceViews(eShader _shader);
    void ApplySamplerStates(eShader _shader);
    void ApplyPipeline();

    ////////// EVENT HANDLERS //////////

    void OnResize(Int32 _width, Int32 _height);
    void OnWindowResizedEvent(const WindowResizedEvent& _event);
    void OnWindowMaxizeEvent(const WindowMaximizedEvent& _event);

    ////////// NATIVE RESOURCE FACTORIES //////////

    [[nodiscard]] TextureRef CreateTexture(
        Int32                                _width,
        Int32                                _height,
        Int32                                _depth,
        eTextureFormat                       _format,
        Int32                                _arraySize,
        eTextureCreateFlags                  _flags,
        eTextureType                         _type,
        const std::span<const ConstMemView>& _initOrEmpty) const;

    [[nodiscard]] HardwareBuffer CreateBuffer(
        Int32                              _byteWidth,
        eBufferType                        _type,
        const std::optional<ConstMemView>& _optInit) const;

    [[nodiscard]] FrameBuffer CreateFrameBuffer(
        std::span<const TextureHandle> _attachments,
        bool                           _bDestoryTexture);

    [[nodiscard]] ShaderHandle GetOrCreateShader(
        ConstMemView _shaderByteCode);

    [[nodiscard]] VertexLayoutHandle GetOrCreateVertexLayout(
        const VertexLayout& _vl);

    ////////// DATA UPLOAD & DOWNLOAD //////////

    [[nodiscard]] MemView BeginUploadBuffer(
        const HardwareBuffer& _buffer) const;

    void EndUploadBuffer(
        const HardwareBuffer& _buffer) const;

    void UploadBuffer(
        const HardwareBuffer& _buffer,
        ConstMemView          _uploadData,
        Int32                 _offset,
        bool                  _bDiscard) const;

    [[nodiscard]] std::vector<Byte> DownloadBuffer(
        const HardwareBuffer& _buffer) const;

    [[nodiscard]] std::vector<Byte> DownloadTexture(
        const TextureRef& _texture,
        Int32             _mipIndex,
        Int32             _arrayIndex) const;

    void Blit(
        const TextureRef& _blitDstTexture,
        Int32             _blitDstMipIndex,
        Int32             _blitDstArrayIndex,
        Int32             _blitDstX,
        Int32             _blitDstY,
        Int32             _blitDstZ,
        const TextureRef& _blitSrcTexture,
        Int32             _blitSrcMipIndex,
        Int32             _blitSrcArrayIndex,
        Int32             _blitSrcX,
        Int32             _blitSrcY,
        Int32             _blitSrcZ,
        Int32             _blitSrcWidthOrZero,
        Int32             _blitSrcHeightOrZero,
        Int32             _blitSrcDepthOrZero) const;

    void UploadTexture(
        const TextureRef& _dstTexture,
        Int32             _dstMipIndex,
        Int32             _dstArrayIndex,
        Int32             _dstX,
        Int32             _dstY,
        Int32             _dstZ,
        ConstMemView      _uploadData,
        Int32             _uploadWidth,
        Int32             _uploadHeight,
        Int32             _uploadDepth,
        Int32             _uploadDataRowPitch,
        Int32             _uploadDataDepthPitch) const;

    ////////// D3D11 CACHE ///////////

    [[nodiscard]] ID3D11RasterizerState* GetOrCreateD3D11RasterizerState(
        RasterizerStateD3D11 _state);

    [[nodiscard]] ID3D11BlendState* GetOrCreateD3D11BlendState(
        const BlendStateD3D11& _state);

    [[nodiscard]] ID3D11DepthStencilState* GetOrCreateD3D11DepthStencilState(
        DepthStencilStateD3D11 _state);

    [[nodiscard]] ID3D11SamplerState* GetOrCreateD3D11SamplerState(
        SamplerState _state,
        RGBA         _color);

    ////////// MISC //////////

    /// create debug name
    [[nodiscard]] std::string CreateName(
        std::string_view _tag,
        AnyHandle        _handle,
        std::string_view _userName) const;

    [[nodiscard]] std::string SourceLocToString(
        const SourceLoc& _loc) const;

    /// set debug name
    void SetD3D11Name(
        ID3D11DeviceChild* _pDeviceChild,
        std::string_view   _name) const;

    /// query highest multisample quality levels
    [[nodiscard]] UInt32 QueryMaxMultisampleQuality(
        DXGI_FORMAT _format,
        Int32       _sampleCount) const;

    ////////// D3D DEVICE //////////

    ID3D11Device*        pd3d11Device        = nullptr;
    ID3D11DeviceContext* pd3d11DeviceContext = nullptr;
    IDXGISwapChain1*     pdxgiSwapchain      = nullptr;

    ////////// RESOURCE POOL //////////

    ResourcePool<VertexBuffer>    vertexBuffers = {};
    ResourcePool<IndexBuffer>     indexBuffers  = {};
    ResourcePool<TextureRef>      textures      = {};
    ResourcePool<FrameBuffer>     frameBuffers  = {};
    ResourcePool<ShaderRef>       shaders       = {};
    ResourcePool<VertexLayoutRef> vertexLayouts = {};
    ResourcePool<ConstantBuffer>  cbuffers      = {};
    ResourcePool<Program>         programs      = {};

    ////////// HANDLE CACHE //////////

    HashMap<ShaderCacheKey, ShaderHandle>             shaderCache         = {};
    HashMap<VertexLayoutCacheKey, VertexLayoutHandle> vertexLayoutHandles = {};

    ////////// D3D11 STATE CACHE //////////

    HashMap<UInt32, ID3D11RasterizerState*>   d3d11RasterizerStateCache   = {};
    HashMap<UInt64, ID3D11BlendState*>        d3d11BlendStateCache        = {};
    HashMap<UInt64, ID3D11DepthStencilState*> d3d11DepthStencilStateCache = {};
    HashMap<UInt64, ID3D11SamplerState*>      d3d11SamplerStateCache      = {};

    ////////// PIPELINE BIND //////////

    VertexLayoutHandle vlhBind      = k_invalidHandle;
    VertexBufferHandle vbhBind      = k_invalidHandle;
    Int32              vbOffsetBind = 0;
    Int32              numVertsBind = 0;

    IndexBufferHandle ibhBind        = k_invalidHandle;
    Int32             ibOffsetBind   = 0;
    Int32             numIndicesBind = 0;

    FrameBufferHandle fbhBind = k_invalidHandle;
    ProgramHandle     phBind  = k_invalidHandle;

    ConstantBufferBinds cbufferBinds[EnumCount<eShader>()] = {};
    TextureBinds        textureBinds[EnumCount<eShader>()] = {};
    SamplerBinds        samplerBinds[EnumCount<eShader>()] = {};

    Viewport vpBind          = {};
    Rect     scissorRectBind = {};

    RenderState renderStateBind = {};
    RGBA        blendFactorBind = {};
    UInt8       stencilRefBind  = 0;

    ePrimitive             primBind              = static_cast<ePrimitive>(0xFF);   // invalid at start
    DepthStencilStateD3D11 depthStencilStateBind = {};                              // invalid at start
    RasterizerStateD3D11   rasterizerStateBind   = {};                              // invalid at start
    BlendStateD3D11        blendStateBind        = {};                              // invalid at start

    ePipelineDirtyFlags pipelineDirtyFlags = ePipelineDirtyFlags_All;

    Int32 submitParameters[EnumCount<eSubmitParameterType>()];

    ////////// BACK BUFFER RESOURCE //////////

    Resolution        backBufferRes  = {};
    TextureHandle     backBufferTexh = k_invalidHandle;
    FrameBufferHandle backBufferFbh  = k_invalidHandle;

}* g_pGfxCtx;

void GfxContext::ApplyConstantBuffers(const eShader _stage)
{
    static ID3D11Buffer* s_binder[k_numConstantBufferLimit];
    ConstantBufferBinds& binds = cbufferBinds[EnumToInt(_stage)];

    const Int32 dirtyStart = binds.dirtyStart;
    const Int32 dirtyEnd   = binds.dirtyEnd;
    if (dirtyStart > dirtyEnd)
    {
        return;
    }

    const Int32 numDirties = dirtyEnd - dirtyStart + 1;
    for (Int32 i = 0; i < numDirties; ++i)
    {
        const ConstantBufferHandle cbh = binds.cbhs[dirtyStart + i];
        s_binder[i]                    = cbuffers.IsValid(cbh) ? cbuffers[cbh].buffer.pBuffer : nullptr;
    }

    // bind
    switch (_stage)
    {
        case eShader::VertexShader:
            pd3d11DeviceContext->VSSetConstantBuffers(dirtyStart, numDirties, s_binder);
            break;
        case eShader::PixelShader:
            pd3d11DeviceContext->PSSetConstantBuffers(dirtyStart, numDirties, s_binder);
            break;
        case eShader::GeometryShader:
            pd3d11DeviceContext->GSSetConstantBuffers(dirtyStart, numDirties, s_binder);
            break;
        case eShader::ComputeShader:
            pd3d11DeviceContext->CSSetConstantBuffers(dirtyStart, numDirties, s_binder);
            break;
        case eShader::HullShader:
            pd3d11DeviceContext->HSSetConstantBuffers(dirtyStart, numDirties, s_binder);
            break;
        case eShader::DomainShader:
            pd3d11DeviceContext->DSSetConstantBuffers(dirtyStart, numDirties, s_binder);
            break;
        default:
            JAM_ASSERT(0, "Unknown shader stage");
            break;
    }

    // reset dirty range
    binds.dirtyStart = k_numConstantBufferLimit;
    binds.dirtyEnd   = 0;
}

void GfxContext::ApplyShaderResourceViews(
    const eShader _shader)
{
    static ID3D11ShaderResourceView* s_binder[k_numShaderResourceLimit];
    TextureBinds&                    binds = textureBinds[EnumToInt(_shader)];

    const Int32 dirtyStart = binds.dirtyStart;
    const Int32 dirtyEnd   = binds.dirtyEnd;
    if (dirtyStart > dirtyEnd)
    {
        return;
    }

    const Int32 numDirtySlots = dirtyEnd - dirtyStart + 1;
    for (Int32 i = 0; i < numDirtySlots; ++i)
    {
        const TextureHandle texh = binds.texhs[dirtyStart + i];
        s_binder[i]              = textures.IsValid(texh) ? textures[texh].pSRV : nullptr;
    }

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            pd3d11DeviceContext->VSSetShaderResources(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::PixelShader:
            pd3d11DeviceContext->PSSetShaderResources(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::GeometryShader:
            pd3d11DeviceContext->GSSetShaderResources(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::ComputeShader:
            pd3d11DeviceContext->CSSetShaderResources(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::HullShader:
            pd3d11DeviceContext->HSSetShaderResources(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::DomainShader:
            pd3d11DeviceContext->DSSetShaderResources(dirtyStart, numDirtySlots, s_binder);
            break;
        default:
            JAM_ASSERT(0, "Unknown shader stage");
            break;
    }

    // reset dirty range
    binds.dirtyStart = k_numShaderResourceLimit;
    binds.dirtyEnd   = 0;
}

void GfxContext::ApplySamplerStates(const eShader _shader)
{
    static ID3D11SamplerState* s_binder[k_numSamplerLimit];
    SamplerBinds&              binds = samplerBinds[EnumToInt(_shader)];

    const Int32 dirtyStart = binds.dirtyStart;
    const Int32 dirtyEnd   = binds.dirtyEnd;
    if (dirtyStart > dirtyEnd)
    {
        return;
    }

    const Int32 numDirtySlots = dirtyEnd - dirtyStart + 1;
    for (Int32 i = 0; i < numDirtySlots; ++i)
    {
        const SamplerState sampler = binds.samplers[dirtyStart + i];
        const RGBA         color   = binds.borderColors[dirtyStart + i];
        s_binder[i]                = GetOrCreateD3D11SamplerState(sampler, color);
    }

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            pd3d11DeviceContext->VSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::PixelShader:
            pd3d11DeviceContext->PSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::GeometryShader:
            pd3d11DeviceContext->GSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::ComputeShader:
            pd3d11DeviceContext->CSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::HullShader:
            pd3d11DeviceContext->HSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::DomainShader:
            pd3d11DeviceContext->DSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        default:
            JAM_ASSERT(0, "Unknown shader stage");
            break;
    }

    // reset dirty range
    binds.dirtyStart = k_numSamplerLimit;
    binds.dirtyEnd   = 0;
}

void GfxContext::ApplyPipeline()
{
    // apply pipeline state if dirty
    if (pipelineDirtyFlags != ePipelineDirtyFlags_None)
    {
        // vertex buffer
        if (pipelineDirtyFlags & ePipelineDirtyFlags_VertexBuffer)
        {
            if (JAM_IS_VALID_HANDLE(vbhBind))
            {
                const VertexBuffer& vb     = vertexBuffers[vbhBind];
                const UINT          stride = vb.stride;
                const UINT          offset = vbOffsetBind * stride;
                pd3d11DeviceContext->IASetVertexBuffers(0, 1, &vb.buffer.pBuffer, &stride, &offset);
            }
            else
            {
                constexpr ID3D11Buffer* k_pNullBuffer = nullptr;
                constexpr UINT          k_stride      = 0;
                constexpr UINT          k_offset      = 0;
                pd3d11DeviceContext->IASetVertexBuffers(0, 1, &k_pNullBuffer, &k_stride, &k_offset);
            }
        }

        // index buffer
        if (pipelineDirtyFlags & ePipelineDirtyFlags_IndexBuffer)
        {
            if (JAM_IS_VALID_HANDLE(ibhBind))
            {
                const IndexBuffer& ib     = indexBuffers[ibhBind];
                const DXGI_FORMAT  format = ib.bUseU32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
                pd3d11DeviceContext->IASetIndexBuffer(ib.buffer.pBuffer, format, ibOffsetBind);
            }
            else
            {
                pd3d11DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
            }
        }

        // topology
        if (pipelineDirtyFlags & ePipelineDirtyFlags_Primitive)
        {
            pd3d11DeviceContext->IASetPrimitiveTopology(k_d3d11PrimitiveTopologyTable[EnumToInt(primBind)]);
        }

        // input vertexLayout
        if (pipelineDirtyFlags & ePipelineDirtyFlags_InputLayout)
        {
            if (JAM_IS_VALID_HANDLE(vlhBind))
            {
                const VertexLayoutRef& vl = vertexLayouts[vlhBind];
                pd3d11DeviceContext->IASetInputLayout(vl.pInputLayout);
            }
            else
            {
                pd3d11DeviceContext->IASetInputLayout(nullptr);
            }
        }

        // shader program
        if (pipelineDirtyFlags & ePipelineDirtyFlags_Program)
        {
            if (JAM_IS_VALID_HANDLE(phBind))
            {
                const Program& program = programs[phBind];
                for (Int32 i = 0; i < k_numGraphicsShaderStage; ++i)
                {
                    const ShaderHandle sh    = program.shs[i];
                    const eShader      stage = static_cast<eShader>(i);
                    if (JAM_IS_VALID_HANDLE(sh))
                    {
                        const ShaderRef& shader = shaders[sh];
                        switch (stage)
                        {
                            case eShader::VertexShader:
                                pd3d11DeviceContext->VSSetShader(shader.pVertexShader, nullptr, 0);
                                break;
                            case eShader::PixelShader:
                                pd3d11DeviceContext->PSSetShader(shader.pPixelShader, nullptr, 0);
                                break;
                            case eShader::GeometryShader:
                                pd3d11DeviceContext->GSSetShader(shader.pGeometryShader, nullptr, 0);
                                break;
                            case eShader::HullShader:
                                pd3d11DeviceContext->HSSetShader(shader.pHullShader, nullptr, 0);
                                break;
                            case eShader::DomainShader:
                                pd3d11DeviceContext->DSSetShader(shader.pDomainShader, nullptr, 0);
                                break;
                            default:
                                JAM_ASSERT(0, "Unknown shader stage");
                                break;
                        }
                    }
                    else
                    {
                        switch (stage)
                        {
                            case eShader::VertexShader:
                                pd3d11DeviceContext->VSSetShader(nullptr, nullptr, 0);
                                break;
                            case eShader::PixelShader:
                                pd3d11DeviceContext->PSSetShader(nullptr, nullptr, 0);
                                break;
                            case eShader::GeometryShader:
                                pd3d11DeviceContext->GSSetShader(nullptr, nullptr, 0);
                                break;
                            case eShader::HullShader:
                                pd3d11DeviceContext->HSSetShader(nullptr, nullptr, 0);
                                break;
                            case eShader::DomainShader:
                                pd3d11DeviceContext->DSSetShader(nullptr, nullptr, 0);
                                break;
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
            if (pipelineDirtyFlags & ePipelineDirtyFlags_VSConstantBuffer)
                ApplyConstantBuffers(eShader::VertexShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_PSConstantBuffer)
                ApplyConstantBuffers(eShader::PixelShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_GSConstantBuffer)
                ApplyConstantBuffers(eShader::GeometryShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_HSConstantBuffer)
                ApplyConstantBuffers(eShader::HullShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_DSConstantBuffer)
                ApplyConstantBuffers(eShader::DomainShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_CSConstantBuffer)
                ApplyConstantBuffers(eShader::ComputeShader);
        }

        // shader resource view
        if (pipelineDirtyFlags & ePipelineDirtyFlags_ShaderResourceViewMask)
        {
            if (pipelineDirtyFlags & ePipelineDirtyFlags_VSShaderResourceView)
                ApplyShaderResourceViews(eShader::VertexShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_PSShaderResourceView)
                ApplyShaderResourceViews(eShader::PixelShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_GSShaderResourceView)
                ApplyShaderResourceViews(eShader::GeometryShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_HSShaderResourceView)
                ApplyShaderResourceViews(eShader::HullShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_DSShaderResourceView)
                ApplyShaderResourceViews(eShader::DomainShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_CSShaderResourceView)
                ApplyShaderResourceViews(eShader::ComputeShader);
        }

        // sampler state
        if (pipelineDirtyFlags & ePipelineDirtyFlags_SamplerStateMask)
        {
            if (pipelineDirtyFlags & ePipelineDirtyFlags_VSSamplerState)
                ApplySamplerStates(eShader::VertexShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_PSSamplerState)
                ApplySamplerStates(eShader::PixelShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_GSSamplerState)
                ApplySamplerStates(eShader::GeometryShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_HSSamplerState)
                ApplySamplerStates(eShader::HullShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_DSSamplerState)
                ApplySamplerStates(eShader::DomainShader);
            if (pipelineDirtyFlags & ePipelineDirtyFlags_CSSamplerState)
                ApplySamplerStates(eShader::ComputeShader);
        }

        // blend state
        if (pipelineDirtyFlags & ePipelineDirtyFlags_BlendState)
        {
            const FLOAT blendFactor[4] = {
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
            const FrameBuffer& fb = frameBuffers[fbhBind];
            pd3d11DeviceContext->OMSetRenderTargets(fb.numRenderTarget, fb.pRTV, fb.pDSV);
        }

        // viewport
        if (pipelineDirtyFlags & ePipelineDirtyFlags_Viewport)
        {
            const D3D11_VIEWPORT vp {
                static_cast<FLOAT>(vpBind.x),
                static_cast<FLOAT>(vpBind.y),
                static_cast<FLOAT>(vpBind.w),
                static_cast<FLOAT>(vpBind.h),
                0.f,
                1.f
            };
            pd3d11DeviceContext->RSSetViewports(1, &vp);
        }

        // scissor rect
        if (pipelineDirtyFlags & ePipelineDirtyFlags_ScissorRect)
        {
            const D3D11_RECT rect {
                scissorRectBind.left,
                scissorRectBind.top,
                scissorRectBind.right,
                scissorRectBind.bottom
            };
            pd3d11DeviceContext->RSSetScissorRects(1, &rect);
        }

        // clear dirty flags
        pipelineDirtyFlags = ePipelineDirtyFlags_None;
    }
}

void GfxContext::OnResize(const Int32 _width, const Int32 _height)
{
    backBufferRes.width  = _width;
    backBufferRes.height = _height;

    // resize back buffer
    if (pdxgiSwapchain)
    {
        // destroy back buffer dependencies
        JAM_DESTROY_TEXTURE(backBufferTexh);
        JAM_DESTROY_FRAME_BUFFER(backBufferFbh);

#ifdef JAM_DEBUG
        {
            // check back buffer dependencies destroyed
            ID3D11Resource* pRes;
            JAM_CHECK_COM(pdxgiSwapchain->GetBuffer(0, IID_PPV_ARGS(&pRes)));
            const UINT refCount = pRes->Release();
            JAM_ASSERT(refCount == 0, "Back buffer.pBuffer reference numVerts is not zero after cleanup. RefCount: {}", refCount);
        }
#endif

        // resize
        JAM_CHECK_COM(pdxgiSwapchain->ResizeBuffers(0, _width, _height, DXGI_FORMAT_UNKNOWN, 0));

        // create dependencies
        {
            // render target texture
            ID3D11Resource* pRes = nullptr;
            JAM_CHECK_COM(pdxgiSwapchain->GetBuffer(0, IID_PPV_ARGS(&pRes)));

            DXGI_SWAP_CHAIN_DESC1 desc;
            JAM_CHECK_COM(pdxgiSwapchain->GetDesc1(&desc));

            TextureRef bbTexRef;
            bbTexRef.width          = NarrowCast<Int32>(desc.Width);
            bbTexRef.height         = NarrowCast<Int32>(desc.Height);
            bbTexRef.depth          = 1;
            bbTexRef.type           = eTextureType::Texture2D;
            bbTexRef.format         = eTextureFormat::RGBA8_UNorm;
            bbTexRef.numMips        = 1;
            bbTexRef.arraySize      = 1;
            bbTexRef.samples        = 1;
            bbTexRef.bMutable       = true;
            bbTexRef.refCount       = 1;
            bbTexRef.bManaged       = false;
            bbTexRef.bRtvAttachable = true;
            bbTexRef.bDsvAttachable = false;
            bbTexRef.pResource      = pRes;

            // register to texture resource pool
            const TextureHandle texh = textures.Alloc();
            textures[texh]           = std::move(bbTexRef);
            Graphics::SetTextureName(texh, "BackBuffer");

            backBufferTexh = texh;
            backBufferFbh  = Graphics::CreateFrameBuffer(texh);
        }
    }
}

void GfxContext::OnWindowResizedEvent(const WindowResizedEvent& _event)
{
    OnResize(_event.GetWidth(), _event.GetHeight());
}

void GfxContext::OnWindowMaxizeEvent(const WindowMaximizedEvent& _event)
{
    OnResize(_event.GetWidth(), _event.GetHeight());
}

TextureRef GfxContext::CreateTexture(
    const Int32                          _width,
    const Int32                          _height,
    const Int32                          _depth,
    const eTextureFormat                 _format,
    const Int32                          _arraySize,
    const eTextureCreateFlags            _flags,
    const eTextureType                   _type,
    const std::span<const ConstMemView>& _initOrEmpty) const
{
    const bool  bGenMips       = (_flags & eTextureCreateFlags_GenerateMips) != 0;
    const bool  bIsMSAA        = (_flags & (eTextureCreateFlags_MultiSampled_Internal)) != 0;
    const bool  bWriteOnly     = (_flags & eTextureCreateFlags_WriteOnly) != 0;
    const bool  bRtvAttachable = (_flags & eTextureCreateFlags_RenderTarget) != 0;
    const bool  bDsvAttachable = (_flags & eTextureCreateFlags_DepthStencil) != 0;
    const bool  bMutable       = _initOrEmpty.empty() || (_flags & eTextureCreateFlags_Mutable) != 0;
    const bool  bIsCubemap     = _type == eTextureType::TextureCube;
    const Int32 numMips        = bGenMips ? CalcNumMips(_width, _height, _depth) : 1;
    const Int32 samples        = (_flags & eTextureCreateFlags_MSAAx4) != 0  ? 4
                               : (_flags & eTextureCreateFlags_MSAAx8) != 0  ? 8
                               : (_flags & eTextureCreateFlags_MSAAx16) != 0 ? 16
                                                                             : 1;

    // validation
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "Texture dimensions must be greater than zero.");
    JAM_ASSERT(_arraySize > 0, "Texture array size must be greater than zero.");
    JAM_ASSERT(!(bIsMSAA && bGenMips), "Multisampled texture cannot have mipmaps.");

    // format
    const TextureFormatInfo formatInfo       = GetTextureFormatInfo(_format);
    const bool              bWriteOnlyDsvTex = bWriteOnly && bDsvAttachable && !bRtvAttachable;
    const DXGI_FORMAT       dxgiFormat       = bWriteOnlyDsvTex ? formatInfo.depthFormat : formatInfo.texFormat;

    // native
    TextureRef tex;
    tex.width          = _width;
    tex.height         = _height;
    tex.depth          = _depth;
    tex.type           = _type;
    tex.format         = _format;
    tex.arraySize      = _arraySize;
    tex.samples        = samples;
    tex.numMips        = numMips;
    tex.bMutable       = bMutable;
    tex.refCount       = 1;
    tex.bManaged       = false;
    tex.bRtvAttachable = bRtvAttachable;
    tex.bDsvAttachable = bDsvAttachable;

    switch (_type)
    {
        case eTextureType::Texture2D:
        case eTextureType::TextureCube:
        {
            D3D11_TEXTURE2D_DESC desc;
            desc.Width          = tex.width;
            desc.Height         = tex.height;
            desc.MipLevels      = tex.numMips;
            desc.ArraySize      = _arraySize;
            desc.Format         = dxgiFormat;
            desc.Usage          = bMutable ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
            desc.CPUAccessFlags = 0;

            // MSAA 설정
            desc.SampleDesc.Count   = samples;
            desc.SampleDesc.Quality = bIsMSAA ? QueryMaxMultisampleQuality(desc.Format, samples) : 0;

            // bind flags 설정
            desc.BindFlags = 0;
            if (!bWriteOnly)
                desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            if (bRtvAttachable)
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            if (bDsvAttachable)
                desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

            // misc
            desc.MiscFlags = 0;
            if (bIsCubemap)
                desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
            if (bGenMips)
                desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

            if (_initOrEmpty.empty())
            {
                JAM_CHECK_COM(pd3d11Device->CreateTexture2D(&desc, nullptr, &tex.pTexture2D));
            }
            else
            {
                JAM_ASSERT(_initOrEmpty.size() == _arraySize * numMips, "Texture data count mismatch");

                std::vector<D3D11_SUBRESOURCE_DATA> initData;
                initData.reserve(_arraySize * numMips);
                for (Int32 arrayIdx = 0; arrayIdx < _arraySize; arrayIdx++)
                {
                    for (Int32 mipIdx = 0; mipIdx < numMips; mipIdx++)
                    {
                        const auto [width, height, _] = CalcTextureResolution(_width, _height, 0, mipIdx);
                        const Int32        texIndex   = CalcTextureIndex(mipIdx, arrayIdx, numMips);
                        const ConstMemView mem        = _initOrEmpty[texIndex];
                        const UINT         rowPitch   = width * GetBytePerPixel(_format);
                        JAM_ASSERT(mem.GetByteWidth() == rowPitch * height, "Texture data size mismatch");
                        initData.emplace_back(mem.GetDataPtr(), rowPitch, 0);
                    }
                }

                JAM_CHECK_COM(pd3d11Device->CreateTexture2D(&desc, initData.data(), &tex.pTexture2D));
            }
        }
        break;

        case eTextureType::Texture3D:
        {
            JAM_ASSERT((_flags & eTextureCreateFlags_MultiSampled_Internal) == 0, "3D texture cannot be multisampled");
            JAM_ASSERT(_arraySize == 1, "3D texture array size must be 1");
            JAM_ASSERT(!bIsCubemap, "3D texture cannot be a cubemap");
            JAM_ASSERT(!bDsvAttachable, "3D texture cannot be a Depth/Stencil view.");

            D3D11_TEXTURE3D_DESC desc;
            desc.Width          = tex.width;
            desc.Height         = tex.height;
            desc.Depth          = tex.depth;
            desc.MipLevels      = tex.numMips;
            desc.Format         = dxgiFormat;
            desc.Usage          = bMutable ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
            desc.CPUAccessFlags = 0;

            // bind flags 설정
            desc.BindFlags = 0;
            if (!bWriteOnly)
                desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            if (bRtvAttachable)
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            if (bDsvAttachable)
                desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;

            // 기타 설정
            desc.MiscFlags = 0;
            if (bGenMips)
                desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

            if (_initOrEmpty.empty())
            {
                JAM_CHECK_COM(pd3d11Device->CreateTexture3D(&desc, nullptr, &tex.pTexture3D));
            }
            else
            {
                JAM_ASSERT(_initOrEmpty.size() == numMips, "Texture data count mismatch");

                std::vector<D3D11_SUBRESOURCE_DATA> initData;
                initData.reserve(numMips);

                for (Int32 mipIdx = 0; mipIdx < numMips; mipIdx++)
                {
                    const auto [width, height, depth] = CalcTextureResolution(_width, _height, _depth, mipIdx);
                    const ConstMemView mem            = _initOrEmpty[mipIdx];
                    const UINT         rowPitch       = width * GetBytePerPixel(_format);
                    const UINT         slicePitch     = rowPitch * height;
                    JAM_ASSERT(mem.GetByteWidth() == slicePitch * depth, "Texture data size mismatch");
                    initData.emplace_back(mem.GetDataPtr(), rowPitch, slicePitch);
                }

                JAM_CHECK_COM(pd3d11Device->CreateTexture3D(&desc, initData.data(), &tex.pTexture3D));
            }
        }
        break;

        default:
            JAM_CRASH("Unsupported texture stage: {}", EnumToInt(_type));
    }

    // SRV 생성
    if (!bWriteOnly)
    {
        const bool bIsArray = _arraySize > 1;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc {};
        srvDesc.Format = GetTextureFormatInfo(_format).colorFormat;
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
                        srvDesc.TextureCubeArray.MipLevels        = numMips;
                        srvDesc.TextureCubeArray.First2DArrayFace = 0;
                        srvDesc.TextureCubeArray.NumCubes         = _arraySize / 6;
                    }
                    else
                    {
                        srvDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
                        srvDesc.TextureCube.MostDetailedMip = 0;
                        srvDesc.TextureCube.MipLevels       = numMips;
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
                            srvDesc.Texture2DArray.MipLevels       = numMips;
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
                            srvDesc.Texture2D.MipLevels       = numMips;
                        }
                    }
                }
            }
            break;

            case eTextureType::Texture3D:
            {
                srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MostDetailedMip = 0;
                srvDesc.Texture3D.MipLevels       = numMips;
            }
            break;

            default:
                JAM_CRASH("Unsupported texture stage: {}", EnumToInt(_type));
        }

        JAM_CHECK_COM(pd3d11Device->CreateShaderResourceView(tex.pResource, &srvDesc, &tex.pSRV));
    }

    return tex;
}

HardwareBuffer GfxContext::CreateBuffer(
    const Int32                        _byteWidth,
    const eBufferType                  _type,
    const std::optional<ConstMemView>& _optInit) const
{
    JAM_ASSERT(_byteWidth > 0, "Buffer size must be greater than zero.");

    // validation
    const bool bDynamic = !_optInit;
    if (_type == eBufferType::ConstantBuffer)
    {
        JAM_ASSERT(_byteWidth % 16 == 0, "Constant buffer size must be multiple of 16 bytes.");
        JAM_ASSERT(bDynamic, "Constant buffer must be dynamic.");
    }

    // native buffer
    HardwareBuffer buffer;
    buffer.byteWidth = _byteWidth;
    buffer.bDynamic  = bDynamic;

    // d3d11 buffer
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = _byteWidth;
    desc.CPUAccessFlags      = bDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.StructureByteStride = 0;
    desc.Usage               = bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
    desc.BindFlags           = _type == eBufferType::VertexBuffer   ? D3D11_BIND_VERTEX_BUFFER
                             : _type == eBufferType::IndexBuffer    ? D3D11_BIND_INDEX_BUFFER
                             : _type == eBufferType::ConstantBuffer ? D3D11_BIND_CONSTANT_BUFFER
                                                                    : 0;
    desc.MiscFlags           = 0;

    // create
    if (_optInit)
    {
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = _optInit->GetDataPtr();
        JAM_CHECK_COM(pd3d11Device->CreateBuffer(&desc, &initData, &buffer.pBuffer));
    }
    else
    {
        JAM_CHECK_COM(pd3d11Device->CreateBuffer(&desc, nullptr, &buffer.pBuffer));
    }

    return buffer;
}

FrameBuffer GfxContext::CreateFrameBuffer(
    std::span<const TextureHandle> _attachments,
    const bool                     _bDestoryTexture)
{
    FrameBuffer fb              = {};
    bool        bHasDepth       = false;
    Int32       numRenderTarget = 0;

    for (const TextureHandle texh: _attachments)
    {
        JAM_ASSERT(JAM_IS_VALID_HANDLE(texh), "Invalid texture handle in render target textures.");

        TextureRef& tex = textures[texh];
        JAM_ASSERT(tex.pResource, "Texture resource is null.");
        JAM_ASSERT(tex.bManaged == false, "Cannot attach a texture that is managed by a framebuffer.");

        // 렌더 타겟 / 뎁스 스텐실 객체 생성
        const bool bIsArray   = tex.arraySize > 1;
        const bool bIsMSAA    = tex.samples > 1;
        const bool bIsCubemap = tex.type == eTextureType::TextureCube;

        // 텍스처 참조 관리
        if (_bDestoryTexture)
        {
            JAM_ASSERT(tex.refCount == 1, "Cannot transfer ownership of a texture that is referenced elsewhere.");
            tex.bManaged = true;   // 프레임버퍼가 소유권을 가짐.
        }
        else
        {
            ++tex.refCount;   // 참조 증가
        }

        if (IsDepthFormat(tex.format))
        {
            JAM_ASSERT(bHasDepth == false, "Only one depth stencil texture can be attached to a framebuffer.");
            JAM_ASSERT(tex.bDsvAttachable, "Depth stencil texture must be created with Depth/Stencil flag.");
            JAM_ASSERT(bIsCubemap == false, "Cubemap depth stencil view is not supported.");
            JAM_ASSERT(tex.type != eTextureType::Texture3D, "3D texture depth stencil view is not supported.");

            D3D11_DEPTH_STENCIL_VIEW_DESC desc {};
            desc.Format = GetTextureFormatInfo(tex.format).depthFormat;

            switch (tex.type)
            {
                case eTextureType::Texture2D:
                case eTextureType::TextureCube:
                {

                    if (bIsArray)
                    {
                        if (bIsMSAA)
                        {
                            desc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                            desc.Texture2DMSArray.FirstArraySlice = 0;
                            desc.Texture2DMSArray.ArraySize       = tex.arraySize;
                        }
                        else
                        {
                            desc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                            desc.Texture2DArray.ArraySize       = tex.arraySize;
                            desc.Texture2DArray.FirstArraySlice = 0;
                            desc.Texture2DArray.MipSlice        = 0;
                        }
                    }
                    else
                    {
                        if (bIsMSAA)
                        {
                            desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                        }
                        else
                        {
                            desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
                            desc.Texture2D.MipSlice = 0;
                        }
                    }
                }
                break;

                default:
                    JAM_CRASH("Unsupported texture stage: {}", EnumToInt(tex.type));
            }

            JAM_CHECK_COM(pd3d11Device->CreateDepthStencilView(tex.pResource, &desc, &fb.pDSV));
            fb.depthStencilTexh = texh;
            bHasDepth           = true;
        }
        else   // 렌더 타겟
        {
            JAM_ASSERT(numRenderTarget < k_numRenderTargetLimit, "Exceeded maximum render target count.");
            JAM_ASSERT(tex.bRtvAttachable, "Render target texture must be created with RenderTarget flag.");
            JAM_ASSERT(bIsCubemap == false, "Cubemap render target view is not supported.");

            D3D11_RENDER_TARGET_VIEW_DESC desc {};
            desc.Format = GetTextureFormatInfo(tex.format).texFormat;

            switch (tex.type)
            {
                case eTextureType::Texture2D:
                case eTextureType::TextureCube:
                {

                    if (bIsArray)
                    {
                        if (bIsMSAA)
                        {
                            desc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                            desc.Texture2DMSArray.FirstArraySlice = 0;
                            desc.Texture2DMSArray.ArraySize       = tex.arraySize;
                        }
                        else
                        {
                            desc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                            desc.Texture2DArray.ArraySize       = tex.arraySize;
                            desc.Texture2DArray.FirstArraySlice = 0;
                            desc.Texture2DArray.MipSlice        = 0;
                        }
                    }
                    else
                    {
                        if (bIsMSAA)
                        {
                            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                        }
                        else
                        {
                            desc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
                            desc.Texture2D.MipSlice = 0;
                        }
                    }
                }
                break;

                case eTextureType::Texture3D:
                {
                    desc.ViewDimension         = D3D11_RTV_DIMENSION_TEXTURE3D;
                    desc.Texture3D.MipSlice    = 0;
                    desc.Texture3D.FirstWSlice = 0;
                    desc.Texture3D.WSize       = -1;
                }
                break;

                default:
                    JAM_CRASH("Unsupported texture stage: {}", EnumToInt(tex.type));
            }

            JAM_CHECK_COM(pd3d11Device->CreateRenderTargetView(tex.pResource, &desc, &fb.pRTV[numRenderTarget]));
            fb.renderTargetTexhs[numRenderTarget] = texh;
            ++numRenderTarget;
        }
    }

    fb.numRenderTarget = numRenderTarget;
    return fb;
}

MemView GfxContext::BeginUploadBuffer(const HardwareBuffer& _buffer) const
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    JAM_CHECK_COM(pd3d11DeviceContext->Map(_buffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
    return { mappedResource.pData, static_cast<size_t>(_buffer.byteWidth) };
}

void GfxContext::EndUploadBuffer(const HardwareBuffer& _buffer) const
{
    pd3d11DeviceContext->Unmap(_buffer.pBuffer, 0);
}

void GfxContext::UploadBuffer(
    const HardwareBuffer& _buffer,
    const ConstMemView    _uploadData,
    const Int32           _offset,
    const bool            _bDiscard) const
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

        pd3d11DeviceContext->CopySubresourceRegion(_buffer.pBuffer, 0, _offset, 0, 0, pStagingBuffer.Get(), 0, &box);
    }
}

std::vector<Byte> GfxContext::DownloadBuffer(
    const HardwareBuffer& _buffer) const
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

std::vector<Byte> GfxContext::DownloadTexture(
    const TextureRef& _texture,
    const Int32       _mipIndex,
    const Int32       _arrayIndex) const
{
    JAM_ASSERT(_mipIndex >= 0 && _arrayIndex >= 0, "Mip and array index must be non-negative");
    JAM_ASSERT(_mipIndex < _texture.numMips, "Mip handle out of range");
    JAM_ASSERT(_arrayIndex < _texture.arraySize, "Array handle out of range");

    const auto [width, height, depth] = CalcTextureResolution(_texture.width, _texture.height, _texture.depth, _mipIndex);
    const Int32 rowPitch              = width * GetBytePerPixel(_texture.format);
    const Int32 slicePitch            = rowPitch * height;

    std::vector<Byte> downloadData;
    downloadData.resize(slicePitch * depth);

    switch (_texture.type)
    {
        case eTextureType::Texture2D:
        case eTextureType::TextureCube:
        {
            ComPtr<ID3D11Texture2D> pReadback;

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

                JAM_CHECK_COM(pd3d11Device->CreateTexture2D(&desc, nullptr, pReadback.ReleaseAndGetAddressOf()));
            }

            // copy to staging texture
            {
                const UINT index = D3D11CalcSubresource(_mipIndex, _arrayIndex, _texture.numMips);
                pd3d11DeviceContext->CopySubresourceRegion(pReadback.Get(), 0, 0, 0, 0, _texture.pTexture2D, index, nullptr);
            }

            // map staging texture
            {
                D3D11_MAPPED_SUBRESOURCE mapped;
                JAM_CHECK_COM(pd3d11DeviceContext->Map(pReadback.Get(), 0, D3D11_MAP_READ, 0, &mapped));

                Byte*       pDstHead = downloadData.data();
                const Byte* pSrcHead = static_cast<const Byte*>(mapped.pData);
                for (Int32 row = 0; row < height; ++row)
                {
                    Byte*       pDst = pDstHead + row * rowPitch;
                    const Byte* pSrc = pSrcHead + row * mapped.RowPitch;
                    std::memcpy(pDst, pSrc, rowPitch);
                }

                pd3d11DeviceContext->Unmap(pReadback.Get(), 0);
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
                const UINT index = D3D11CalcSubresource(_mipIndex, 0, _texture.numMips);
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
    const TextureRef& _blitDstTexture,
    const Int32       _blitDstMipIndex,
    const Int32       _blitDstArrayIndex,
    const Int32       _blitDstX,
    const Int32       _blitDstY,
    const Int32       _blitDstZ,
    const TextureRef& _blitSrcTexture,
    const Int32       _blitSrcMipIndex,
    const Int32       _blitSrcArrayIndex,
    const Int32       _blitSrcX,
    const Int32       _blitSrcY,
    const Int32       _blitSrcZ,
    const Int32       _blitSrcWidthOrZero,
    const Int32       _blitSrcHeightOrZero,
    const Int32       _blitSrcDepthOrZero) const
{
    const auto [srcWidth, srcHeight, srcDepth] = CalcTextureResolution(_blitSrcTexture.width, _blitSrcTexture.height, _blitSrcTexture.depth, _blitSrcMipIndex);
    const auto [dstWidth, dstHeight, dstDepth] = CalcTextureResolution(_blitDstTexture.width, _blitDstTexture.height, _blitDstTexture.depth, _blitDstMipIndex);

    Int32 blitWidth  = _blitSrcWidthOrZero > 0 ? _blitSrcWidthOrZero : srcWidth - _blitSrcX;
    Int32 blitHeight = _blitSrcHeightOrZero > 0 ? _blitSrcHeightOrZero : srcHeight - _blitSrcY;
    Int32 blitDepth  = _blitSrcDepthOrZero > 0 ? _blitSrcDepthOrZero : srcDepth - _blitSrcZ;
    blitWidth        = Min(blitWidth, srcWidth - _blitSrcX, dstWidth - _blitDstX);
    blitHeight       = Min(blitHeight, srcHeight - _blitSrcY, dstHeight - _blitDstY);
    blitDepth        = Min(blitDepth, srcDepth - _blitSrcZ, dstDepth - _blitDstZ);

    D3D11_BOX box;
    box.left   = _blitSrcX;
    box.top    = _blitSrcY;
    box.front  = _blitSrcZ;
    box.right  = _blitSrcX + blitWidth;
    box.bottom = _blitSrcY + blitHeight;
    box.back   = _blitSrcZ + blitDepth;

    const UINT srcIndex = CalcTextureIndex(_blitSrcMipIndex, _blitSrcArrayIndex, _blitSrcTexture.numMips);
    const UINT dstIndex = CalcTextureIndex(_blitDstMipIndex, _blitDstArrayIndex, _blitDstTexture.numMips);

    pd3d11DeviceContext->CopySubresourceRegion(
        _blitDstTexture.pResource,
        dstIndex,
        _blitDstX,
        _blitDstY,
        _blitDstZ,
        _blitSrcTexture.pResource,
        srcIndex,
        &box);
}

void GfxContext::UploadTexture(
    const TextureRef&  _dstTexture,
    const Int32        _dstMipIndex,
    const Int32        _dstArrayIndex,
    const Int32        _dstX,
    const Int32        _dstY,
    const Int32        _dstZ,
    const ConstMemView _uploadData,
    const Int32        _uploadWidth,
    const Int32        _uploadHeight,
    const Int32        _uploadDepth,
    const Int32        _uploadDataRowPitch,
    const Int32        _uploadDataDepthPitch) const
{
    JAM_ASSERT(_dstTexture.pResource, "Destination texture resource is null!");

    auto [w, h, d] = CalcTextureResolution(_dstTexture.width, _dstTexture.height, _dstTexture.depth, _dstMipIndex);

    D3D11_BOX dstBox;
    dstBox.left   = _dstX;
    dstBox.top    = _dstY;
    dstBox.front  = _dstZ;
    dstBox.right  = Min(_dstX + _uploadWidth, w);
    dstBox.bottom = Min(_dstY + _uploadHeight, h);
    dstBox.back   = Min(_dstZ + _uploadDepth, d);

    const UINT dstIndex = CalcTextureIndex(_dstMipIndex, _dstArrayIndex, _dstTexture.numMips);
    pd3d11DeviceContext->UpdateSubresource(
        _dstTexture.pResource,
        dstIndex,
        &dstBox,
        _uploadData.GetDataPtr(),
        _uploadDataRowPitch,
        _uploadDataDepthPitch);
}

ID3D11RasterizerState* GfxContext::GetOrCreateD3D11RasterizerState(
    const RasterizerStateD3D11 _state)
{
    MurmurHash32 hasher;
    hasher.Begin();
    hasher.Add(&_state, sizeof(RasterizerStateD3D11));
    const UInt32 key = hasher.End();
    const auto   it  = d3d11RasterizerStateCache.find(key);

    if (it != d3d11RasterizerStateCache.end())
    {
        return it->second;
    }

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
    d3d11RasterizerStateCache[key] = pRasterizerState;
    return pRasterizerState;
}

ID3D11BlendState* GfxContext::GetOrCreateD3D11BlendState(
    const BlendStateD3D11& _state)
{
    MurmurHash64 hasher;
    hasher.Begin();
    hasher.Add(&_state, sizeof(BlendStateD3D11));
    const UInt64 key = hasher.End();
    const auto   it  = d3d11BlendStateCache.find(key);

    if (it != d3d11BlendStateCache.end())
    {
        return it->second;
    }

    D3D11_BLEND_DESC desc {};
    desc.AlphaToCoverageEnable  = _state.bAlphaToCoverage;
    desc.IndependentBlendEnable = _state.bIndependentBlend;
    for (Int32 i = 0; i < k_numRenderTargetLimit; ++i)
    {
        const BlendState&               rtBlend = _state.blends[i];
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
    d3d11BlendStateCache[key] = pBlendState;
    return pBlendState;
}

ID3D11DepthStencilState* GfxContext::GetOrCreateD3D11DepthStencilState(
    const DepthStencilStateD3D11 _state)
{
    MurmurHash64 hasher;
    hasher.Begin();
    hasher.Add(&_state, sizeof(DepthStencilStateD3D11));
    const UInt64 key = hasher.End();
    const auto   it  = d3d11DepthStencilStateCache.find(key);

    if (it != d3d11DepthStencilStateCache.end())
    {
        return it->second;
    }

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
    d3d11DepthStencilStateCache[key] = pDepthStencilState;
    return pDepthStencilState;
}

ID3D11SamplerState* GfxContext::GetOrCreateD3D11SamplerState(
    const SamplerState _state,
    const RGBA         _color)
{
    const UInt64 value = static_cast<UInt64>(_state.value);
    const UInt64 key   = (value << 32ull) | _color.color;
    const auto   it    = d3d11SamplerStateCache.find(key);

    if (it != d3d11SamplerStateCache.end())
    {
        return it->second;
    }

    const SamplerInfo& filter          = k_samplerInfoTable[EnumToInt(_state.filter)];
    const bool         bIsComparison   = _state.compareFunc != eCompareFunc::None;
    const Int32        anisotopicLevel = _state.filter == eTextureFilter::Anisotropic4  ? 4
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
    desc.MinLOD         = k_floatMin;
    desc.MaxLOD         = k_floatMax;

    ID3D11SamplerState* pSamplerState = nullptr;
    JAM_CHECK_COM(pd3d11Device->CreateSamplerState(&desc, &pSamplerState));
    d3d11SamplerStateCache[key] = pSamplerState;
    return pSamplerState;
}

ShaderHandle GfxContext::GetOrCreateShader(const ConstMemView _shaderByteCode)
{
    const void*  pByteCode = _shaderByteCode.GetDataPtr();
    const size_t byteWidth = _shaderByteCode.GetByteWidth();

    // hashing
    MurmurHash64 hasher;
    hasher.Begin();
    hasher.Add(pByteCode, byteWidth);
    const ShaderCacheKey key  = hasher.End();
    const auto           iter = shaderCache.find(key);

    if (iter != shaderCache.end())
    {
        const ShaderHandle sh = iter->second;
        ++shaders[sh].refCount;
        return sh;
    }

    // 쉐이더 생성
    ShaderRef shader;
    shader.refCount = 1;
    shader.cache    = key;

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

VertexLayoutHandle GfxContext::GetOrCreateVertexLayout(const VertexLayout& _vl)
{
    // calculate hash
    MurmurHash64 hasher;
    hasher.Begin();
    hasher.Add(&_vl, sizeof(_vl));
    const VertexLayoutCacheKey hash = hasher.End();

    // check cache
    if (const auto iter = vertexLayoutHandles.find(hash); iter != vertexLayoutHandles.end())
    {
        const VertexLayoutHandle vlh = iter->second;
        ++vertexLayouts[vlh].refCount;
        return vlh;
    }

    const std::span<const VertexLayoutElement> elems = _vl.GetElements();

    // prepare dummy hlsl and input descriptors
    std::string                           dummyHLSL = "struct VSInput {";
    std::vector<D3D11_INPUT_ELEMENT_DESC> inputDescs;
    inputDescs.reserve(elems.size());

    for (const VertexLayoutElement& elem: elems)
    {
        JAM_ASSERT(elem.format != eAttributeFormat::Unknown && elem.attrib != eVertexAttribute::Unknown,
                   "vertex layout contains unknown attribute or format");

        const Int32 attribIdx = EnumToInt(elem.attrib);
        const Int32 formatIdx = EnumToInt(elem.format);

        // generate hlsl struct field
        const char* const      dataType     = k_hlslFormatNames[formatIdx];
        const std::string_view variableName = EnumToString(elem.attrib);
        const char* const      semanticName = k_hlslSemanticTable[attribIdx];
        const Int32            index        = k_d3d11IndexTable[attribIdx];

        dummyHLSL += fmt::format("{} {} : {}{};", dataType, variableName, semanticName, index);

        // configure d3d11 input element desc
        D3D11_INPUT_ELEMENT_DESC desc = {};
        desc.SemanticName             = semanticName;
        desc.SemanticIndex            = index;
        desc.Format                   = k_dxgiFormatAttributeTable[formatIdx];
        desc.InputSlot                = 0;
        desc.AlignedByteOffset        = D3D11_APPEND_ALIGNED_ELEMENT;
        desc.InputSlotClass           = D3D11_INPUT_PER_VERTEX_DATA;
        desc.InstanceDataStepRate     = 0;

        inputDescs.push_back(desc);
    }

    // finalize hlsl source
    dummyHLSL += "}; void main(VSInput IN) { }";

    // compile dummy vertex shader to get bytecode signature
    ShaderCompiler compiler;
    const bool     bResult = compiler.CompileFromSource(dummyHLSL, eShader::VertexShader, "main", eShaderCompileFlags_Optimization);
    JAM_ASSERT(bResult, "failed to compile dummy vertex shader for input layout creation: {}", GetErrorString());

    ConstMemView byteCode = compiler.GetBytecode();

    // create d3d11 input layout
    ID3D11InputLayout* pInputLayout = nullptr;
    const UINT         numElems     = NarrowCast<UINT>(elems.size());
    JAM_CHECK_COM(pd3d11Device->CreateInputLayout(
        inputDescs.data(),
        numElems,
        byteCode.GetDataPtr(),
        byteCode.GetByteWidth(),
        &pInputLayout));

    // allocate handle and initialize resource
    const VertexLayoutHandle vlh = vertexLayouts.Alloc();

    VertexLayoutRef vl;
    vl.refCount     = 1;
    vl.vertexLayout = _vl;
    vl.pInputLayout = pInputLayout;
    vl.hash         = hash;

    SetD3D11Name(pInputLayout, CreateName("InputLayout", vlh, ""));

    // register to cache
    vertexLayouts[vlh]        = std::move(vl);
    vertexLayoutHandles[hash] = vlh;
    return vlh;
}

std::string GfxContext::CreateName(std::string_view _tag, const AnyHandle _handle, std::string_view _userName) const
{
    std::string name = std::format("[{}] Index: {}, Token: {}", _tag, _handle.index, _handle.token);
    if (_userName.empty() == false)
    {
        name += std::format(", Name: {}", _userName);
    }
    return name;
}

std::string GfxContext::SourceLocToString(const SourceLoc& _loc) const
{
    return std::format("Func: {}, File: {}, Line: {}", _loc.function, _loc.filename, _loc.line);
}

void GfxContext::SetD3D11Name(ID3D11DeviceChild* _pDeviceChild, const std::string_view _name) const
{
    JAM_ASSERT(_pDeviceChild, "Resource is null");
    JAM_CHECK_COM(_pDeviceChild->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
    JAM_CHECK_COM(_pDeviceChild->SetPrivateData(WKPDID_D3DDebugObjectName, NarrowCast<UINT>(_name.size()), _name.data()));
}

UInt32 GfxContext::QueryMaxMultisampleQuality(const DXGI_FORMAT _format, const Int32 _sampleCount) const
{
    JAM_ASSERT(_sampleCount > 0 && _sampleCount <= 32, "Sample numVerts must be between 1 and 32");
    UINT qualityLevels = 0;
    JAM_CHECK_COM(pd3d11Device->CheckMultisampleQualityLevels(_format, _sampleCount, &qualityLevels));
    JAM_ASSERT(qualityLevels > 0, "The specified format does not support the given sample numVerts.");
    return qualityLevels - 1;
}

}   // namespace jam

namespace jam
{

void Graphics::Init(const HWND _hwnd, const Int32 _width, const Int32 _height)
{
    JAM_ASSERT(g_pGfxCtx == nullptr, "Graphics already initialized");
    g_pGfxCtx = JAM_NEW(GfxContext);

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
            &g_pGfxCtx->pd3d11Device,
            nullptr,
            &g_pGfxCtx->pd3d11DeviceContext));
    }

    // 스왑체인 생성
    {
        ComPtr<IDXGIDevice2> dxgiDevice;
        JAM_CHECK_COM(g_pGfxCtx->pd3d11Device->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

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

        JAM_CHECK_COM(dxgiFactory->CreateSwapChainForHwnd(g_pGfxCtx->pd3d11Device, _hwnd, &swapChainDesc, nullptr, nullptr, &g_pGfxCtx->pdxgiSwapchain));
        g_pGfxCtx->OnResize(_width, _height);
    }
}

void Graphics::Shutdown()
{
    JAM_ASSERT(g_pGfxCtx, "Graphics not initialized");

    // 내부 리소스 해제
    JAM_DESTROY_TEXTURE(g_pGfxCtx->backBufferTexh);
    JAM_DESTROY_FRAME_BUFFER(g_pGfxCtx->backBufferFbh);

    // d3d11 cache 해체
    for (ID3D11BlendState* pBlendState: std::views::values(g_pGfxCtx->d3d11BlendStateCache))
    {
        pBlendState->Release();
    }

    for (ID3D11DepthStencilState* pDepthStencilState: std::views::values(g_pGfxCtx->d3d11DepthStencilStateCache))
    {
        pDepthStencilState->Release();
    }

    for (ID3D11RasterizerState* pRasterizerState: std::views::values(g_pGfxCtx->d3d11RasterizerStateCache))
    {
        pRasterizerState->Release();
    }

    for (ID3D11SamplerState* pSamplerState: std::views::values(g_pGfxCtx->d3d11SamplerStateCache))
    {
        pSamplerState->Release();
    }

    // d3d11 device 해제
    g_pGfxCtx->pdxgiSwapchain->Release();
    g_pGfxCtx->pd3d11DeviceContext->Release();
    g_pGfxCtx->pd3d11Device->Release();

    // 릭 검사
    if (DetectLeak())
    {
        JAM_ASSERT(0, "Graphics resource leak detected!");
    }

    // 그래픽스 컨텍스트 해제
    JAM_DELETE(g_pGfxCtx);
}

bool Graphics::DetectLeak()
{
    bool bLeakDetected = false;

    for (const auto& [handle, value]: g_pGfxCtx->vertexBuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("VertexBuffer leak detected: Handle index = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_pGfxCtx->indexBuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("IndexBuffer leak detected: Handle index = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_pGfxCtx->textures)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("Texture leak detected: Handle index = {}, Token = {}, Name = '{}', RefCount = {}", index, token, value.name, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_pGfxCtx->frameBuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("FrameBuffer leak detected: Handle index = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_pGfxCtx->shaders)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("Shader leak detected: Handle index = {}, Token = {}, Stage = {}, RefCount = {}", index, token, EnumToString(value.stage), value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_pGfxCtx->vertexLayouts)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("VertexLayout leak detected: Handle index = {}, Token = {}, RefCount = {}", index, token, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: g_pGfxCtx->cbuffers)
    {
        UInt32 index = handle.index;
        UInt32 token = handle.token;
        Log::Warn("ConstantBuffer leak detected: Handle index = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (auto handle: g_pGfxCtx->programs | std::views::keys)
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
    HandleEvent<WindowResizedEvent>(_event, g_pGfxCtx, &GfxContext::OnWindowResizedEvent);
    HandleEvent<WindowMaximizedEvent>(_event, g_pGfxCtx, &GfxContext::OnWindowMaxizeEvent);
}

void Graphics::Frame(const bool _bVSync)
{
    JAM_CHECK_COM(g_pGfxCtx->pdxgiSwapchain->Present(_bVSync ? 1 : 0, 0));

    // 렌더 타겟은 매 프레임마다 다시 설정해줘야 함
    g_pGfxCtx->fbhBind = k_invalidHandle;
}

VertexBufferHandle Graphics::CreateVertexBuffer(
    ConstMemView        _vertexData,
    const VertexLayout& _vl)
{
    const Int32 bufferWIdth = NarrowCast<Int32>(_vertexData.GetByteWidth());

    VertexBuffer vb;
    vb.buffer   = g_pGfxCtx->CreateBuffer(bufferWIdth, eBufferType::VertexBuffer, _vertexData);
    vb.stride   = _vl.GetStride();
    vb.numVerts = bufferWIdth / vb.stride;
    vb.vlh      = g_pGfxCtx->GetOrCreateVertexLayout(_vl);

    const VertexBufferHandle vbh  = g_pGfxCtx->vertexBuffers.Alloc();
    g_pGfxCtx->vertexBuffers[vbh] = std::move(vb);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetVertexBufferName(vbh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetVertexBufferName(vbh);
#endif

    return vbh;
}

VertexBufferHandle Graphics::CreateDynamicVertexBuffer(
    const Int32         _numVerts,
    const VertexLayout& _vl)
{
    JAM_ASSERT(_numVerts > 0, "Vertex count must be greater than 0");

    const Int32 stride = _vl.GetStride();

    VertexBuffer vb;
    vb.buffer   = g_pGfxCtx->CreateBuffer(_numVerts * stride, eBufferType::VertexBuffer, std::nullopt);
    vb.stride   = stride;
    vb.numVerts = _numVerts;
    vb.vlh      = g_pGfxCtx->GetOrCreateVertexLayout(_vl);

    const VertexBufferHandle vbh  = g_pGfxCtx->vertexBuffers.Alloc();
    g_pGfxCtx->vertexBuffers[vbh] = std::move(vb);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetVertexBufferName(vbh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetVertexBufferName(vbh);
#endif

    return vbh;
}

IndexBufferHandle Graphics::CreateIndexBuffer(
    ConstMemView _indexData,
    const bool   _bUseU32)
{
    const Int32 stride    = GetIndexStride(_bUseU32);
    const Int32 byteWidth = NarrowCast<Int32>(_indexData.GetByteWidth());

    IndexBuffer ib;
    ib.buffer     = g_pGfxCtx->CreateBuffer(byteWidth, eBufferType::IndexBuffer, _indexData);
    ib.numIndices = byteWidth / stride;
    ib.stride     = stride;
    ib.bUseU32    = _bUseU32;

    const IndexBufferHandle ibh  = g_pGfxCtx->indexBuffers.Alloc();
    g_pGfxCtx->indexBuffers[ibh] = std::move(ib);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetIndexBufferName(ibh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetIndexBufferName(ibh);
#endif

    return ibh;
}

IndexBufferHandle Graphics::CreateDynamicIndexBuffer(
    const Int32 _numIndices,
    const bool  _bUseU32)
{
    JAM_ASSERT(_numIndices > 0, "Index count must be greater than 0");

    const Int32 stride = GetIndexStride(_bUseU32);

    IndexBuffer ib;
    ib.buffer     = g_pGfxCtx->CreateBuffer(_numIndices * stride, eBufferType::IndexBuffer, std::nullopt);
    ib.numIndices = _numIndices;
    ib.stride     = stride;
    ib.bUseU32    = _bUseU32;

    const IndexBufferHandle ibh  = g_pGfxCtx->indexBuffers.Alloc();
    g_pGfxCtx->indexBuffers[ibh] = std::move(ib);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetIndexBufferName(ibh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetIndexBufferName(ibh);
#endif

    return ibh;
}

ConstantBufferHandle Graphics::CreateConstantBuffer(const Int32 _byteWidth)
{
    JAM_ASSERT(_byteWidth > 0 && _byteWidth <= 65536, "Constant buffer size must be between 1 and 65536 bytes");
    JAM_ASSERT(_byteWidth % 16 == 0, "Constant buffer size must be a multiple of 16 bytes");

    ConstantBuffer cb;
    cb.buffer = g_pGfxCtx->CreateBuffer(_byteWidth, eBufferType::ConstantBuffer, std::nullopt);

    ConstantBufferHandle cbh = g_pGfxCtx->cbuffers.Alloc();
    g_pGfxCtx->cbuffers[cbh] = std::move(cb);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetConstantBufferName(cbh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetConstantBufferName(cbh);
#endif

    return cbh;
}

TextureHandle Graphics::CreateTexture2D(
    const Int32                          _width,
    const Int32                          _height,
    const eTextureFormat                 _format,
    const Int32                          _arraySize,
    const eTextureCreateFlags            _flags,
    const std::span<const ConstMemView>& _initData)
{
    const TextureHandle texh  = g_pGfxCtx->textures.Alloc();
    g_pGfxCtx->textures[texh] = g_pGfxCtx->CreateTexture(_width, _height, 1, _format, _arraySize, _flags, eTextureType::Texture2D, _initData);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetTextureName(texh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetTextureName(texh);
#endif

    return texh;
}
TextureHandle Graphics::CreateTextureCube(
    const Int32                          _width,
    const Int32                          _height,
    const eTextureFormat                 _format,
    const Int32                          _cubeCount,
    const eTextureCreateFlags            _flags,
    const std::span<const ConstMemView>& _initData)
{
    const TextureHandle texh  = g_pGfxCtx->textures.Alloc();
    g_pGfxCtx->textures[texh] = g_pGfxCtx->CreateTexture(_width, _height, 1, _format, _cubeCount * 6, _flags, eTextureType::TextureCube, _initData);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetTextureName(texh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetTextureName(texh);
#endif

    return texh;
}

TextureHandle Graphics::CreateTexture3D(
    const Int32                          _width,
    const Int32                          _height,
    const Int32                          _depth,
    const eTextureFormat                 _format,
    const eTextureCreateFlags            _texFlags,
    const std::span<const ConstMemView>& _initData)
{
    const TextureHandle texh  = g_pGfxCtx->textures.Alloc();
    g_pGfxCtx->textures[texh] = g_pGfxCtx->CreateTexture(_width, _height, _depth, _format, 1, _texFlags, eTextureType::Texture3D, _initData);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetTextureName(texh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetTextureName(texh);
#endif

    return texh;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const std::span<const TextureHandle> _attachments,
    const bool                           _bDestroyTexture)
{
    const FrameBufferHandle fbh  = g_pGfxCtx->frameBuffers.Alloc();
    g_pGfxCtx->frameBuffers[fbh] = g_pGfxCtx->CreateFrameBuffer(_attachments, _bDestroyTexture);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetFrameBufferName(fbh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetFrameBufferName(fbh);
#endif

    return fbh;
}

ShaderHandle Graphics::CreateShader(
    const ConstMemView _shaderByteCode)
{
    const ShaderHandle sh = g_pGfxCtx->GetOrCreateShader(_shaderByteCode);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetShaderName(sh, g_pGfxCtx->SourceLocToString(loc));
#endif

    return sh;
}

ProgramHandle Graphics::CreateProgram(
    const ShaderHandle _vsh,
    const ShaderHandle _psh,
    const ShaderHandle _gsh,
    const ShaderHandle _dsh,
    const ShaderHandle _hsh,
    const bool         _bDestroyShader)
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(_vsh) || JAM_IS_VALID_HANDLE(_psh), "At least one of VS or PS must be provided");

    const ShaderHandle shs[k_numGraphicsShaderStage] = {
        _vsh,
        _psh,
        _gsh,
        _hsh,
        _dsh
    };

    Program program = {};
    for (Int32 i = 0; i < k_numGraphicsShaderStage; ++i)
    {
        const eShader      stage = static_cast<eShader>(i);
        const ShaderHandle sh    = shs[i];

        if (!JAM_IS_VALID_HANDLE(sh))
        {
            continue;
        }

        ShaderRef& shader = g_pGfxCtx->shaders[sh];
        JAM_ASSERT(shader.stage == stage, "Shader handle does not match the expected shader stage");

        // add ref
        if (_bDestroyShader == false)
        {
            ++shader.refCount;
        }

        program.shs[i] = sh;
    }

    const ProgramHandle ph  = g_pGfxCtx->programs.Alloc();
    g_pGfxCtx->programs[ph] = std::move(program);
    return ph;
}

ProgramHandle Graphics::CreateProgram(
    const ShaderHandle _vsh,
    const ShaderHandle _psh,
    const bool         _bDestroyShader)
{
    return CreateProgram(
        _vsh,
        _psh,
        k_invalidHandle,
        k_invalidHandle,
        k_invalidHandle,
        _bDestroyShader);
}

void Graphics::DestroyVertexBuffer(const VertexBufferHandle _vbh)
{
    Int32         refCount;
    VertexBuffer& vb = g_pGfxCtx->vertexBuffers[_vbh];

    // destory d3d11 resource
    {
        ID3D11Buffer* pd3d11Buffer = vb.buffer.pBuffer;
        refCount                   = NarrowCast<Int32>(pd3d11Buffer->Release());
        if (refCount > 0)
        {
            JAM_ASSERT(0, "Vertex buffer reference count is not zero. RefCount: {}", refCount);
        }
    }

    // release VertexLayoutRef
    {
        VertexLayoutHandle vlh = vb.vlh;
        VertexLayoutRef&   vl  = g_pGfxCtx->vertexLayouts[vlh];
        refCount               = --vl.refCount;
        JAM_ASSERT(refCount >= 0, "VertexLayoutRef reference count is already zero");

        // destroy vertex vertexLayout if no more references
        if (refCount == 0)
        {
            refCount = vl.pInputLayout->Release();   // destory d3d11 resource
            if (refCount > 0)
            {
                JAM_ASSERT(0, "Vertex vertexLayout.pInputLayout reference count is not zero. RefCount: {}", refCount);
            }

            g_pGfxCtx->vertexLayoutHandles.erase(vl.hash);
            g_pGfxCtx->vertexLayouts.Free(vlh);
        }

        g_pGfxCtx->vertexBuffers.Free(_vbh);
    }
}

void Graphics::DestroyIndexBuffer(const IndexBufferHandle _ibh)
{
    IndexBuffer& ib = g_pGfxCtx->indexBuffers[_ibh];
    UINT         refCount;

    // destroy d3d11 resource
    {
        ID3D11Buffer* pd3d11Buffer = ib.buffer.pBuffer;
        refCount                   = pd3d11Buffer->Release();
        if (refCount > 0)
        {
            JAM_ASSERT(0, "Index buffer.pBuffer reference count is not zero. RefCount: {}", refCount);
        }
    }

    // free handle
    g_pGfxCtx->indexBuffers.Free(_ibh);
}

void Graphics::DestroyTexture(const TextureHandle _texh)
{
    // 텍스처 참조 해제
    TextureRef& tex      = g_pGfxCtx->textures[_texh];
    Int32       refCount = --tex.refCount;
    JAM_ASSERT(refCount >= 0, "TextureRef reference count is already zero");

    // 텍스처 파괴
    if (tex.refCount == 0)
    {
        if (tex.pSRV)   // SRV 파괴
        {
            UINT d3dRefCount = tex.pSRV->Release();
            if (d3dRefCount > 0)
            {
                JAM_ASSERT(0, "TextureRef shader resource view reference count is not zero. RefCount: {}", d3dRefCount);
            }
        }

        UINT d3dRefCount = tex.pResource->Release();   // 리소스 파괴
        if (d3dRefCount > 0)
        {
            JAM_ASSERT(0, "TextureRef resource reference count is not zero. RefCount: {}", d3dRefCount);
        }

        g_pGfxCtx->textures.Free(_texh);   // 핸들 해제
    }
}

void Graphics::DestroyFrameBuffer(const FrameBufferHandle _fbh)
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(_fbh), "Invalid FrameBuffer handle");

    FrameBuffer& fb = g_pGfxCtx->frameBuffers[_fbh];
    UINT         refCount;

    // destory rtv
    for (Int32 i = 0; i < fb.numRenderTarget; ++i)
    {
        ID3D11RenderTargetView* pRTV = fb.pRTV[i];
        refCount                     = pRTV->Release();   // d3d 리소스 파괴
        if (refCount > 0)
        {
            JAM_ASSERT(0, "Render target view reference count is not zero. RefCount: {}", refCount);
        }

        DestroyTexture(fb.renderTargetTexhs[i]);   // TextureHandle 해제
    }

    // DSV 파괴
    if (fb.pDSV)
    {
        ID3D11DepthStencilView* pDSV = fb.pDSV;
        refCount                     = pDSV->Release();   // d3d 리소스 파괴
        if (refCount > 0)
        {
            JAM_ASSERT(0, "Depth stencil view reference count is not zero. RefCount: {}", refCount);
        }

        DestroyTexture(fb.depthStencilTexh);   // TextureHandle 해제
    }

    g_pGfxCtx->frameBuffers.Free(_fbh);   // FrameBuffer 핸들 해제
}

void Graphics::DestroyProgram(const ProgramHandle _ph)
{
    Program& program = g_pGfxCtx->programs[_ph];

    for (ShaderHandle handle: program.shs)
    {
        if (JAM_IS_VALID_HANDLE(handle))
        {
            DestroyShader(handle);   // 셰이더 참조 해제
        }
    }

    g_pGfxCtx->programs.Free(_ph);   // Program 핸들 해제
}

void Graphics::DestroyShader(const ShaderHandle _sh)
{
    ShaderRef& shader = g_pGfxCtx->shaders[_sh];
    Int32      refCount;

    // 참조 해제
    refCount = --shader.refCount;
    JAM_ASSERT(refCount >= 0, "ShaderRef reference count is already zero");

    if (refCount == 0)
    {
        // d3d 쉐이더 객체 파괴
        switch (shader.stage)
        {
            case eShader::VertexShader:
                refCount = NarrowCast<Int32>(shader.pVertexShader->Release());
                break;
            case eShader::PixelShader:
                refCount = NarrowCast<Int32>(shader.pPixelShader->Release());
                break;
            case eShader::GeometryShader:
                refCount = NarrowCast<Int32>(shader.pGeometryShader->Release());
                break;
            case eShader::HullShader:
                refCount = NarrowCast<Int32>(shader.pHullShader->Release());
                break;
            case eShader::DomainShader:
                refCount = NarrowCast<Int32>(shader.pDomainShader->Release());
                break;
            case eShader::ComputeShader:
                refCount = NarrowCast<Int32>(shader.pComputeShader->Release());
                break;
            default:
                JAM_ASSERT(0, "Unsupported shader stage");
        }

        if (refCount > 0)
        {
            JAM_ASSERT(0, "Shader reference count is not zero. RefCount: {}", refCount);
        }

        g_pGfxCtx->shaderCache.erase(shader.cache);   // 캐시에서 제거
        g_pGfxCtx->shaders.Free(_sh);                 // Shader 핸들 해제
    }
}

void Graphics::DestroyConstantBuffer(
    const ConstantBufferHandle _cbh)
{
    ConstantBuffer& cbuffer  = g_pGfxCtx->cbuffers[_cbh];
    const UINT      refCount = cbuffer.buffer.pBuffer->Release();
    if (refCount > 0)
    {
        JAM_ASSERT(0, "Constant buffer.pBuffer reference count is not zero. RefCount: {}", refCount);
    }

    g_pGfxCtx->cbuffers.Free(_cbh);   // 핸들 해제
}

void Graphics::SetVertexBufferName(
    const VertexBufferHandle _vbh,
    const std::string_view   _name)
{
    VertexBuffer& vb = g_pGfxCtx->vertexBuffers[_vbh];
    vb.name          = g_pGfxCtx->CreateName("VertexBuffer", _vbh, _name);
    g_pGfxCtx->SetD3D11Name(vb.buffer.pBuffer, vb.name);
}

void Graphics::SetIndexBufferName(
    const IndexBufferHandle _ibh,
    const std::string_view  _name)
{
    IndexBuffer& ib = g_pGfxCtx->indexBuffers[_ibh];
    ib.name         = g_pGfxCtx->CreateName("IndexBuffer", _ibh, _name);
    g_pGfxCtx->SetD3D11Name(ib.buffer.pBuffer, ib.name);
}

void Graphics::SetConstantBufferName(
    const ConstantBufferHandle _cbh,
    const std::string_view     _name)
{
    ConstantBuffer& cb = g_pGfxCtx->cbuffers[_cbh];
    cb.name            = g_pGfxCtx->CreateName("ConstantBuffer", _cbh, _name);
    g_pGfxCtx->SetD3D11Name(cb.buffer.pBuffer, cb.name);
}

void Graphics::SetTextureName(
    const TextureHandle    _texh,
    const std::string_view _name)
{
    constexpr const char* const k_names[EnumCount<eTextureType>()] = {
        "Texture2D",
        "Texture3D",
        "TextureCube",
    };

    TextureRef& tex = g_pGfxCtx->textures[_texh];
    tex.name        = g_pGfxCtx->CreateName(k_names[EnumToInt(tex.type)], _texh, _name);
    g_pGfxCtx->SetD3D11Name(tex.pResource, tex.name);

    if (tex.pSRV)
    {
        constexpr const char* const k_srvNames[EnumCount<eTextureType>()] = {
            "Texture2D_SRV",
            "Texture3D_SRV",
            "TextureCube_SRV",
        };

        std::string name = g_pGfxCtx->CreateName(k_srvNames[EnumToInt(tex.type)], _texh, _name);
        g_pGfxCtx->SetD3D11Name(tex.pSRV, name);
    }
}

void Graphics::SetFrameBufferName(
    const FrameBufferHandle _fbh,
    const std::string_view  _name)
{
    constexpr const char* const k_names[k_numRenderTargetLimit] = {
        "FrameBuffer_RTV0",
        "FrameBuffer_RTV1",
        "FrameBuffer_RTV2",
        "FrameBuffer_RTV3",
        "FrameBuffer_RTV4",
        "FrameBuffer_RTV5",
        "FrameBuffer_RTV6",
        "FrameBuffer_RTV7",
    };

    FrameBuffer& fb = g_pGfxCtx->frameBuffers[_fbh];
    fb.name         = g_pGfxCtx->CreateName("FrameBuffer", _fbh, _name);

    for (Int32 i = 0; i < fb.numRenderTarget; ++i)
    {
        if (fb.pRTV[i])
        {
            std::string name = g_pGfxCtx->CreateName(k_names[i], _fbh, _name);
            g_pGfxCtx->SetD3D11Name(fb.pRTV[i], name);
        }
    }

    if (fb.pDSV)
    {
        std::string name = g_pGfxCtx->CreateName("FrameBuffer_DSV", _fbh, _name);
        g_pGfxCtx->SetD3D11Name(fb.pDSV, name);
    }
}

void Graphics::SetShaderName(
    const ShaderHandle     _sh,
    const std::string_view _name)
{
    constexpr const char* const k_names[EnumCount<eShader>()] = {
        "VertexShader", "PixelShader", "GeometryShader", "HullShader", "DomainShader", "ComputeShader"
    };

    ShaderRef& shader = g_pGfxCtx->shaders[_sh];
    shader.name       = g_pGfxCtx->CreateName(k_names[EnumToInt(shader.stage)], _sh, _name);

    switch (shader.stage)
    {
        case eShader::VertexShader:
            g_pGfxCtx->SetD3D11Name(shader.pVertexShader, shader.name);
            break;
        case eShader::PixelShader:
            g_pGfxCtx->SetD3D11Name(shader.pPixelShader, shader.name);
            break;
        case eShader::GeometryShader:
            g_pGfxCtx->SetD3D11Name(shader.pGeometryShader, shader.name);
            break;
        case eShader::HullShader:
            g_pGfxCtx->SetD3D11Name(shader.pHullShader, shader.name);
            break;
        case eShader::DomainShader:
            g_pGfxCtx->SetD3D11Name(shader.pDomainShader, shader.name);
            break;
        case eShader::ComputeShader:
            g_pGfxCtx->SetD3D11Name(shader.pComputeShader, shader.name);
            break;
        default:
            JAM_ASSERT(0, "Unsupported shader stage");
    }
}

void Graphics::Blit(
    const TextureHandle _blitDstTexh,
    const TextureHandle _blitSrcTexh)
{
    const TextureRef& dstTex = g_pGfxCtx->textures[_blitDstTexh];
    const TextureRef& srcTex = g_pGfxCtx->textures[_blitSrcTexh];
    g_pGfxCtx->Blit(dstTex, 0, 0, 0, 0, 0, srcTex, 0, 0, 0, 0, 0, 0, 0, 0);
}

void Graphics::Blit(
    const TextureHandle _blitDstTexh,
    const Int32         _blitDstMipIndex,
    const Int32         _blitDstArrayIndex,
    const Int32         _blitDstX,
    const Int32         _blitDstY,
    const Int32         _blitDstZ,
    const TextureHandle _blitSrcTexh,
    const Int32         _blitSrcMipIndex,
    const Int32         _blitSrcArrayIndex,
    const Int32         _blitSrcX,
    const Int32         _blitSrcY,
    const Int32         _blitSrcZ,
    const Int32         _blitSrcWidthOrZero,
    const Int32         _blitSrcHeightOrZero,
    const Int32         _blitSrcDepthOrZero)
{
    const TextureRef& dstTex = g_pGfxCtx->textures[_blitDstTexh];
    const TextureRef& srcTex = g_pGfxCtx->textures[_blitSrcTexh];
    g_pGfxCtx->Blit(
        dstTex,
        _blitDstMipIndex,
        _blitDstArrayIndex,
        _blitDstX,
        _blitDstY,
        _blitDstZ,
        srcTex,
        _blitSrcMipIndex,
        _blitSrcArrayIndex,
        _blitSrcX,
        _blitSrcY,
        _blitSrcZ,
        _blitSrcWidthOrZero,
        _blitSrcHeightOrZero,
        _blitSrcDepthOrZero);
}

void Graphics::UploadTexture2D(
    const TextureHandle _texh,
    const Int32         _dstMipIndex,
    const Int32         _dstArrayIndex,
    const Int32         _dstX,
    const Int32         _dstY,
    const ConstMemView  _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDataRowPitch)
{
    const TextureRef& tex = g_pGfxCtx->textures[_texh];
    g_pGfxCtx->UploadTexture(
        tex,
        _dstMipIndex,
        _dstArrayIndex,
        _dstX,
        _dstY,
        0,
        _uploadData,
        _uploadWidth,
        _uploadHeight,
        1,
        _uploadDataRowPitch,
        0);
}

void Graphics::UploadTexture2D(
    const TextureHandle _texh,
    const ConstMemView  _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDataRowPitch)
{
    UploadTexture2D(
        _texh,
        0,
        0,
        0,
        0,
        _uploadData,
        _uploadWidth,
        _uploadHeight,
        _uploadDataRowPitch);
}

void Graphics::UploadTextureCube(
    const TextureHandle _texh,
    const Int32         _dstMipIndex,
    const Int32         _dstCubeIndex,
    const eCubeFace     _dstCubeFace,
    const Int32         _dstX,
    const Int32         _dstY,
    const ConstMemView  _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDataRowPitch)
{
    const TextureRef& tex = g_pGfxCtx->textures[_texh];
    g_pGfxCtx->UploadTexture(
        tex,
        _dstMipIndex,
        _dstCubeIndex * 6 + k_d3d11CubeFaceTable[EnumToInt(_dstCubeFace)],
        _dstX,
        _dstY,
        0,
        _uploadData,
        _uploadWidth,
        _uploadHeight,
        1,
        _uploadDataRowPitch,
        0);
}

void Graphics::UploadTextureCube(
    const TextureHandle _texh,
    const eCubeFace     _dstCubeFace,
    const ConstMemView  _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDataRowPitch)
{
    UploadTextureCube(
        _texh,
        0,
        0,
        _dstCubeFace,
        0,
        0,
        _uploadData,
        _uploadWidth,
        _uploadHeight,
        _uploadDataRowPitch);
}

void Graphics::UploadTexture3D(
    const TextureHandle _texh,
    const Int32         _dstMipIndex,
    const Int32         _dstArrayIndex,
    const Int32         _dstX,
    const Int32         _dstY,
    const Int32         _dstZ,
    const ConstMemView  _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDepth,
    const Int32         _uploadDataRowPitch,
    const Int32         _uploadDataDepthPitch)
{
    const TextureRef& tex = g_pGfxCtx->textures[_texh];
    g_pGfxCtx->UploadTexture(
        tex,
        _dstMipIndex,
        _dstArrayIndex,
        _dstX,
        _dstY,
        _dstZ,
        _uploadData,
        _uploadWidth,
        _uploadHeight,
        _uploadDepth,
        _uploadDataRowPitch,
        _uploadDataDepthPitch);
}

void Graphics::UploadTexture3D(
    const TextureHandle _texh,
    const ConstMemView  _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDepth,
    const Int32         _uploadDataRowPitch,
    const Int32         _uploadDataDepthPitch)
{
    UploadTexture3D(
        _texh,
        0,
        0,
        0,
        0,
        0,
        _uploadData,
        _uploadWidth,
        _uploadHeight,
        _uploadDepth,
        _uploadDataRowPitch,
        _uploadDataDepthPitch);
}

std::vector<UInt8> Graphics::DownloadTexture2D(
    const TextureHandle _texh,
    const Int32         _mipIndex,
    const Int32         _arrayIndex)
{
    const TextureRef& tex = g_pGfxCtx->textures[_texh];
    return g_pGfxCtx->DownloadTexture(
        tex,
        _mipIndex,
        _arrayIndex);
}

std::vector<UInt8> Graphics::DownloadTextureCube(
    const TextureHandle _texh,
    const Int32         _mipIndex,
    const Int32         _cubeIndex,
    const eCubeFace     _cubeFace)
{
    const TextureRef& tex = g_pGfxCtx->textures[_texh];
    return g_pGfxCtx->DownloadTexture(
        tex,
        _mipIndex,
        _cubeIndex * 6 + k_d3d11CubeFaceTable[EnumToInt(_cubeFace)]);
}

std::vector<UInt8> Graphics::DownloadTexture3D(
    const TextureHandle _texh,
    const Int32         _mipIndex)
{
    const TextureRef& tex = g_pGfxCtx->textures[_texh];
    return g_pGfxCtx->DownloadTexture(
        tex,
        _mipIndex,
        0);
}

DirectX::ScratchImage Graphics::Capture(
    const TextureHandle _texh)
{
    const TextureRef&     tex = g_pGfxCtx->textures[_texh];
    DirectX::ScratchImage scratch;
    JAM_CHECK_COM(DirectX::CaptureTexture(g_pGfxCtx->pd3d11Device, g_pGfxCtx->pd3d11DeviceContext, tex.pResource, scratch));
    return scratch;
}

void Graphics::Resolve(
    const TextureHandle _srcTexh,
    const Int32         _srcArrayIndex,
    const TextureHandle _dstTexh,
    const Int32         _dstArrayIndex)
{
    TextureRef& srcTex = g_pGfxCtx->textures[_srcTexh];
    TextureRef& dstTex = g_pGfxCtx->textures[_dstTexh];
    JAM_ASSERT(dstTex.samples == 1, "Destination texture must not be multi-sampled.");

    const Int32             srcIndex   = srcTex.numMips * _srcArrayIndex;
    const Int32             dstIndex   = dstTex.numMips * _dstArrayIndex;
    const TextureFormatInfo formatInfo = GetTextureFormatInfo(dstTex.format);
    g_pGfxCtx->pd3d11DeviceContext->ResolveSubresource(
        dstTex.pResource,
        dstIndex,
        srcTex.pResource,
        srcIndex,
        formatInfo.texFormat);
}

TextureResolution Graphics::GetTextureResolution(
    const TextureHandle _texh,
    const Int32         _mipIndex)
{
    const TextureRef& tex = g_pGfxCtx->textures[_texh];
    TextureResolution res;
    res.width  = Max(1, tex.width >> _mipIndex);
    res.height = Max(1, tex.height >> _mipIndex);
    res.depth  = Max(1, tex.depth >> _mipIndex);
    return res;
}

void Graphics::ClearRenderTarget(
    const FrameBufferHandle _fbh,
    const RGBA              _color,
    const Int32             _renderTargetIndex)
{
    FrameBufferHandle fbh         = JAM_IS_VALID_HANDLE(_fbh) ? _fbh : g_pGfxCtx->backBufferFbh;
    FrameBuffer&      frameBuffer = g_pGfxCtx->frameBuffers[fbh];
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < frameBuffer.numRenderTarget, "Render target m_index out of range.");
    FLOAT colors[4] = {
        static_cast<float>(_color.r) / 255.0f,
        static_cast<float>(_color.g) / 255.0f,
        static_cast<float>(_color.b) / 255.0f,
        static_cast<float>(_color.a) / 255.0f
    };
    g_pGfxCtx->pd3d11DeviceContext->ClearRenderTargetView(frameBuffer.pRTV[_renderTargetIndex], colors);
}

void Graphics::ClearDepthStencil(
    const FrameBufferHandle _fbh,
    const bool              _bClearDepth,
    const bool              _bClearStencil,
    const float             _depth,
    const UInt8             _stencil)
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(_fbh), "back buffer do not have depth stencil.");

    FrameBuffer& fb = g_pGfxCtx->frameBuffers[_fbh];
    JAM_ASSERT(fb.pDSV, "Frame buffer does not have a depth stencil view.");

    UINT flags = 0;
    if (_bClearDepth)
        flags |= D3D11_CLEAR_DEPTH;
    if (_bClearStencil)
        flags |= D3D11_CLEAR_STENCIL;

    g_pGfxCtx->pd3d11DeviceContext->ClearDepthStencilView(fb.pDSV, flags, _depth, _stencil);
}

const TextureHandle& Graphics::GetRenderTargetTexture(
    const FrameBufferHandle _fbh,
    const Int32             _renderTargetIndex)
{
    const FrameBufferHandle fbh = JAM_IS_VALID_HANDLE(_fbh) ? _fbh : g_pGfxCtx->backBufferFbh;
    const FrameBuffer&      fb  = g_pGfxCtx->frameBuffers[fbh];
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < fb.numRenderTarget, "Render target m_index out of range.");
    return fb.renderTargetTexhs[_renderTargetIndex];
}

const TextureHandle& Graphics::GetDepthStencilTexture(
    const FrameBufferHandle _fbh)
{
    JAM_ASSERT(JAM_IS_VALID_HANDLE(_fbh), "back buffer do not have depth stencil.");
    const FrameBuffer& fb = g_pGfxCtx->frameBuffers[_fbh];
    JAM_ASSERT(fb.pDSV, "Frame buffer does not have a depth stencil view.");
    return fb.depthStencilTexh;
}

const VertexLayout& Graphics::GetVertexLayout(
    const VertexLayoutHandle _vlh)
{
    const VertexLayoutRef& vl = g_pGfxCtx->vertexLayouts[_vlh];
    return vl.vertexLayout;
}

VertexBufferInfo Graphics::GetVertexBufferInfo(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer& vb = g_pGfxCtx->vertexBuffers[_vbh];

    VertexBufferInfo vbi;
    vbi.byteWidth    = vb.buffer.byteWidth;
    vbi.numVerts     = vb.numVerts;
    vbi.vertexStride = vb.stride;
    vbi.bDynamic     = vb.buffer.bDynamic;
    vbi.layoutHandle = vb.vlh;
    return vbi;
}

IndexBufferInfo Graphics::GetIndexBufferInfo(
    const IndexBufferHandle _ibh)
{
    const IndexBuffer& ib = g_pGfxCtx->indexBuffers[_ibh];

    IndexBufferInfo ibi;
    ibi.byteWidth  = ib.buffer.byteWidth;
    ibi.numIndices = ib.numIndices;
    ibi.bUseU32    = ib.bUseU32;
    ibi.bDynamic   = ib.buffer.bDynamic;
    return ibi;
}

ConstantBufferInfo Graphics::GetConstantBufferInfo(
    const ConstantBufferHandle _cbh)
{
    const ConstantBuffer& cb = g_pGfxCtx->cbuffers[_cbh];

    ConstantBufferInfo cbi;
    cbi.byteWidth = cb.buffer.byteWidth;
    return cbi;
}

TextureInfo Graphics::GetTextureInfo(
    const TextureHandle _texh)
{
    const TextureRef& tex = g_pGfxCtx->textures[_texh];

    TextureInfo texi;
    texi.width                   = tex.width;
    texi.height                  = tex.height;
    texi.depth                   = tex.depth;
    texi.type                    = tex.type;
    texi.format                  = tex.format;
    texi.arraySize               = tex.arraySize;
    texi.samples                 = tex.samples;
    texi.numMips                 = tex.numMips;
    texi.bMutable                = tex.bMutable;
    texi.bRenderTargetAttachable = tex.bRtvAttachable;
    texi.bDepthStencilAttachable = tex.bDsvAttachable;
    return texi;
}

FrameBufferInfo Graphics::GetFrameBufferInfo(
    const FrameBufferHandle _fbh)
{
    const FrameBufferHandle fbh = JAM_IS_VALID_HANDLE(_fbh) ? _fbh : g_pGfxCtx->backBufferFbh;
    const FrameBuffer&      fb  = g_pGfxCtx->frameBuffers[fbh];

    FrameBufferInfo fbi;
    fbi.numRenderTargets = fb.numRenderTarget;
    fbi.bHasDepthStencil = JAM_IS_VALID_HANDLE(fb.depthStencilTexh);
    std::memcpy(&fbi.renderTargetTexhs, fb.renderTargetTexhs, sizeof(fb.renderTargetTexhs));
    fbi.depthStencilTexh = fb.depthStencilTexh;
    return fbi;
}

ShaderInfo Graphics::GetShaderInfo(
    const ShaderHandle _sh)
{
    const ShaderRef& shader = g_pGfxCtx->shaders[_sh];

    ShaderInfo si;
    si.stage = shader.stage;
    return si;
}

ProgramInfo Graphics::GetProgramInfo(
    const ProgramHandle _ph)
{
    const Program& program = g_pGfxCtx->programs[_ph];

    ProgramInfo pi;
    std::memcpy(pi.shs, program.shs, sizeof(program.shs));
    return pi;
}

void Graphics::UploadBuffer(
    const VertexBufferHandle _vbh,
    const ConstMemView       _writeData,
    const Int32              _offset,
    const bool               _bDiscard)
{
    const VertexBuffer& buffer = g_pGfxCtx->vertexBuffers[_vbh];
    g_pGfxCtx->UploadBuffer(buffer.buffer, _writeData, _offset, _bDiscard);
}

void Graphics::UploadBuffer(
    const IndexBufferHandle _ibh,
    const ConstMemView      _writeData,
    const Int32             _offset,
    const bool              _bDiscard)
{
    const IndexBuffer& buffer = g_pGfxCtx->indexBuffers[_ibh];
    g_pGfxCtx->UploadBuffer(buffer.buffer, _writeData, _offset, _bDiscard);
}

void Graphics::UploadBuffer(
    const ConstantBufferHandle _cbh,
    const ConstMemView         _writeData)
{
    ConstantBuffer& buffer = g_pGfxCtx->cbuffers[_cbh];
    g_pGfxCtx->UploadBuffer(buffer.buffer, _writeData, 0, true);
}

MemView Graphics::BeginUploadBuffer(
    const VertexBufferHandle _vbh)
{
    VertexBuffer& buffer = g_pGfxCtx->vertexBuffers[_vbh];
    return g_pGfxCtx->BeginUploadBuffer(buffer.buffer);
}

MemView Graphics::BeginUploadBuffer(
    const IndexBufferHandle _ibh)
{
    IndexBuffer& buffer = g_pGfxCtx->indexBuffers[_ibh];
    return g_pGfxCtx->BeginUploadBuffer(buffer.buffer);
}

void Graphics::EndUploadBuffer(
    const VertexBufferHandle _vbh)
{
    VertexBuffer& buffer = g_pGfxCtx->vertexBuffers[_vbh];
    g_pGfxCtx->EndUploadBuffer(buffer.buffer);
}

void Graphics::EndUploadBuffer(
    const IndexBufferHandle _ibh)
{
    IndexBuffer& buffer = g_pGfxCtx->indexBuffers[_ibh];
    g_pGfxCtx->EndUploadBuffer(buffer.buffer);
}

std::vector<UInt8> Graphics::DownloadBuffer(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer& buffer = g_pGfxCtx->vertexBuffers[_vbh];
    return g_pGfxCtx->DownloadBuffer(buffer.buffer);
}

std::vector<UInt8> Graphics::DownloadBuffer(
    const IndexBufferHandle _ibh)
{
    const IndexBuffer& buffer = g_pGfxCtx->indexBuffers[_ibh];
    return g_pGfxCtx->DownloadBuffer(buffer.buffer);
}

std::vector<Byte> Graphics::DownloadBuffer(
    const ConstantBufferHandle _cbh)
{
    const ConstantBuffer& buffer = g_pGfxCtx->cbuffers[_cbh];
    return g_pGfxCtx->DownloadBuffer(buffer.buffer);
}

const VertexLayout& Graphics::GetVertexLayout(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer&    buffer = g_pGfxCtx->vertexBuffers[_vbh];
    const VertexLayoutRef& vl     = g_pGfxCtx->vertexLayouts[buffer.vlh];
    return vl.vertexLayout;
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _vbh)
{
    if (JAM_IS_VALID_HANDLE(_vbh))
    {
        const VertexBuffer& buffer = g_pGfxCtx->vertexBuffers[_vbh];
        SetVertexBuffer(_vbh, buffer.numVerts, 0);
    }
    else
    {
        SetVertexBuffer(_vbh, 0, 0);
    }
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _vbh,
    const Int32              _numVerts,
    const Int32              _vertexOffset)
{
    if (g_pGfxCtx->vbhBind != _vbh || g_pGfxCtx->vbOffsetBind != _vertexOffset)
    {
        if (JAM_IS_VALID_HANDLE(_vbh))
        {
            const VertexBuffer& buffer = g_pGfxCtx->vertexBuffers[_vbh];
            if (g_pGfxCtx->vlhBind != buffer.vlh)
            {
                g_pGfxCtx->vlhBind = buffer.vlh;
                g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_InputLayout;
            }
        }

        g_pGfxCtx->vbhBind      = _vbh;
        g_pGfxCtx->vbOffsetBind = _vertexOffset;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_VertexBuffer;
    }
    g_pGfxCtx->numVertsBind = _numVerts;
}

void Graphics::SetIndexBuffer(const IndexBufferHandle _handleOrInvalid)
{
    const IndexBuffer& buffer = g_pGfxCtx->indexBuffers[_handleOrInvalid];
    SetIndexBuffer(_handleOrInvalid, buffer.numIndices, 0);
}

void Graphics::SetIndexBuffer(
    const IndexBufferHandle _ibh,
    const Int32             _indexCount,
    const Int32             _indexOffset)
{
    if (g_pGfxCtx->ibhBind != _ibh || g_pGfxCtx->ibOffsetBind != _indexOffset)
    {
        g_pGfxCtx->ibhBind      = _ibh;
        g_pGfxCtx->ibOffsetBind = _indexOffset;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_IndexBuffer;
    }
    g_pGfxCtx->numIndicesBind = _indexCount;
}

void Graphics::SetConstantBuffer(
    const ConstantBufferHandle _cbh,
    const eShader              _shader,
    const Int32                _slot)
{
    const Int32          stage = EnumToInt(_shader);
    ConstantBufferBinds& binds = g_pGfxCtx->cbufferBinds[stage];

    if (binds.cbhs[_slot] != _cbh)
    {
        binds.cbhs[_slot] = _cbh;
        binds.dirtyStart  = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd    = Max(binds.dirtyEnd, _slot);
        g_pGfxCtx->pipelineDirtyFlags |= (ePipelineDirtyFlags_VSConstantBuffer << stage);
    }
}

void Graphics::SetTexture(
    const TextureHandle _texh,
    const eShader       _shader,
    const Int32         _slot)
{
    const Int32   index = EnumToInt(_shader);
    TextureBinds& binds = g_pGfxCtx->textureBinds[index];

    if (binds.texhs[_slot] != _texh)
    {
        binds.texhs[_slot] = _texh;
        binds.dirtyStart   = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd     = Max(binds.dirtyEnd, _slot);
        g_pGfxCtx->pipelineDirtyFlags |= (ePipelineDirtyFlags_VSShaderResourceView << index);
    }
}

void Graphics::SetSampler(
    const SamplerState _state,
    const eShader      _shader,
    const Int32        _slot)
{
    const Int32   index = EnumToInt(_shader);
    SamplerBinds& binds = g_pGfxCtx->samplerBinds[index];

    if (!MemEqual(binds.samplers[_slot], _state))
    {
        binds.samplers[_slot] = _state;
        binds.dirtyStart      = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd        = Max(binds.dirtyEnd, _slot);
        g_pGfxCtx->pipelineDirtyFlags |= (ePipelineDirtyFlags_VSSamplerState << index);
    }
}

void Graphics::SetBorderColor(
    const eShader _shader,
    const Int32   _slot,
    const RGBA    _rgba)
{
    const Int32   index = EnumToInt(_shader);
    SamplerBinds& binds = g_pGfxCtx->samplerBinds[index];
    if (binds.borderColors[_slot].color != _rgba.color)
    {
        binds.borderColors[_slot] = _rgba;
        binds.dirtyStart          = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd            = Max(binds.dirtyEnd, _slot);
        g_pGfxCtx->pipelineDirtyFlags |= (ePipelineDirtyFlags_VSSamplerState << index);
    }
}

void Graphics::SetGfxState(const RenderState _state)
{
    if (!MemEqual(g_pGfxCtx->renderStateBind, _state))
    {
        g_pGfxCtx->renderStateBind = _state;

        // depth stencil state
        DepthStencilStateD3D11 dss = g_pGfxCtx->depthStencilStateBind;
        dss.bDepthWrite            = _state.bDepthWrite;
        dss.depthTestFunc          = _state.depthTestFunc;
        if (!MemEqual(g_pGfxCtx->depthStencilStateBind, dss))
        {
            g_pGfxCtx->depthStencilStateBind = dss;
            g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
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
        if (!MemEqual(g_pGfxCtx->rasterizerStateBind, rs))
        {
            g_pGfxCtx->rasterizerStateBind = rs;
            g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_RasterizerState;
        }

        // blend state
        BlendStateD3D11 bs   = g_pGfxCtx->blendStateBind;
        bs.bAlphaToCoverage  = _state.bAlphaToCoverage;
        bs.bIndependentBlend = _state.bIndependentBlend;
        if (!MemEqual(g_pGfxCtx->blendStateBind, bs))
        {
            g_pGfxCtx->blendStateBind = bs;
            g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_BlendState;
        }

        // primitive
        if (g_pGfxCtx->primBind != _state.primitive)
        {
            g_pGfxCtx->primBind = _state.primitive;
            g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_Primitive;
        }
    }
}

void Graphics::SetBlendState(
    const BlendState _state,
    const Int32      _renderTargetIndex)
{
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < k_numRenderTargetLimit, "Render target index out of range.");
    BlendState bs = g_pGfxCtx->blendStateBind.blends[_renderTargetIndex];
    if (!MemEqual(bs, _state))
    {
        g_pGfxCtx->blendStateBind.blends[_renderTargetIndex] = _state;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_BlendState;
    }
}

void Graphics::SetBlendFactor(const RGBA _rgba)
{
    if (g_pGfxCtx->blendFactorBind.color != _rgba.color)
    {
        g_pGfxCtx->blendFactorBind = _rgba;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_BlendState;
    }
}

void Graphics::SetStencil(
    const StencilState _fStencil,
    const StencilState _bStencil,
    const UInt8        _stencilRef)
{
    if (!MemEqual(g_pGfxCtx->depthStencilStateBind.fStencil, _fStencil))
    {
        g_pGfxCtx->depthStencilStateBind.fStencil = _fStencil;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
    }

    if (!MemEqual(g_pGfxCtx->depthStencilStateBind.bStencil, _bStencil))
    {
        g_pGfxCtx->depthStencilStateBind.bStencil = _bStencil;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
    }

    if (g_pGfxCtx->stencilRefBind != _stencilRef)
    {
        g_pGfxCtx->stencilRefBind = _stencilRef;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_DepthStencilState;
    }
}

void Graphics::SetProgram(const ProgramHandle _ph)
{
    if (g_pGfxCtx->phBind != _ph)
    {
        g_pGfxCtx->phBind = _ph;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_Program;
    }
}

void Graphics::SetFrameBuffer(const FrameBufferHandle _fbh)
{
    const FrameBufferHandle fbh = JAM_IS_VALID_HANDLE(_fbh) ? _fbh : g_pGfxCtx->backBufferFbh;
    if (g_pGfxCtx->fbhBind != fbh)
    {
        g_pGfxCtx->fbhBind = fbh;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_FrameBuffer;
    }
}

void Graphics::SetViewport(const Viewport _viewport)
{
    if (!MemEqual(g_pGfxCtx->vpBind, _viewport))
    {
        g_pGfxCtx->vpBind = _viewport;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_Viewport;
    }
}

void Graphics::SetViewport(const UInt16 _x, const UInt16 _y, const UInt16 _width, const UInt16 _height)
{
    SetViewport(Viewport { _x, _y, _width, _height });
}

void Graphics::SetScissorRect(const Rect _rect)
{
    if (!MemEqual(g_pGfxCtx->scissorRectBind, _rect))
    {
        g_pGfxCtx->scissorRectBind = _rect;
        g_pGfxCtx->pipelineDirtyFlags |= ePipelineDirtyFlags_ScissorRect;
    }
}

void Graphics::SetScissorRect(const UInt16 _left, const UInt16 _top, const UInt16 _right, const UInt16 _bottom)
{
    SetScissorRect(Rect { _left, _top, _right, _bottom });
}

void Graphics::SetSubmitParameter(const eSubmitParameterType _type, const Int32 _value)
{
    g_pGfxCtx->submitParameters[EnumToInt(_type)] = _value;
}

void Graphics::Flush()
{
    g_pGfxCtx->ApplyPipeline();
}

ShaderHandle Graphics::GetShader(
    const ProgramHandle _ph,
    const eShader       _stage)
{
    const Program&     program = g_pGfxCtx->programs[_ph];
    const ShaderHandle sh      = program.shs[EnumToInt(_stage)];
    JAM_ASSERT(JAM_IS_VALID_HANDLE(sh), "Program does not have the specified shader stage.");
    return sh;
}

Resolution Graphics::GetBackBufferResolution()
{
    const TextureRef& bbTex = g_pGfxCtx->textures[g_pGfxCtx->backBufferTexh];

    Resolution res;
    res.width  = bbTex.width;
    res.height = bbTex.height;
    return res;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const TextureHandle _texh,
    const bool          _bDestroyTexture)
{
    const TextureHandle     attachments[1] = { _texh };
    const FrameBufferHandle fbh            = CreateFrameBuffer(attachments, _bDestroyTexture);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetFrameBufferName(fbh, g_pGfxCtx->SourceLocToString(loc));
#endif

    return fbh;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const Int32          _width,
    const Int32          _height,
    const eTextureFormat _renderTargetFormat,
    const eTextureFormat _depthFormat,
    const bool           _writeOnlyRenderTarget,
    const bool           _writeOnlyDepthStencil)
{
    JAM_ASSERT(_width > 0 && _height > 0, "Frame buffer width and height must be greater than 0");
    JAM_ASSERT(_renderTargetFormat != eTextureFormat::Unknown, "Render target format must be specified");
    JAM_ASSERT(_depthFormat != eTextureFormat::Unknown, "Depth stencil format must be specified");
    JAM_ASSERT(!IsDepthFormat(_renderTargetFormat), "Render target format must not be a depth format");
    JAM_ASSERT(IsDepthFormat(_depthFormat), "Depth format must be a depth format");

    eTextureCreateFlags rtvFlags = eTextureCreateFlags_RenderTarget;
    if (_writeOnlyRenderTarget)
        rtvFlags |= eTextureCreateFlags_WriteOnly;

    eTextureCreateFlags dsvFlags = eTextureCreateFlags_DepthStencil;
    if (_writeOnlyDepthStencil)
        dsvFlags |= eTextureCreateFlags_WriteOnly;

    TextureHandle attachments[2] = { k_invalidHandle, k_invalidHandle };
    attachments[0]               = CreateTexture2D(_width, _height, _renderTargetFormat, 1, rtvFlags);
    attachments[1]               = CreateTexture2D(_width, _height, _depthFormat, 1, dsvFlags);
    const FrameBufferHandle fbh  = CreateFrameBuffer(attachments, true);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetFrameBufferName(fbh, g_pGfxCtx->SourceLocToString(loc));
#endif

    return fbh;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const Int32          _width,
    const Int32          _height,
    const eTextureFormat _format,
    const bool           _bWriteOnly)
{
    eTextureCreateFlags flags = 0;
    if (_bWriteOnly)
        flags |= eTextureCreateFlags_WriteOnly;

    if (IsDepthFormat(_format))
        flags |= eTextureCreateFlags_DepthStencil;
    else
        flags |= eTextureCreateFlags_RenderTarget;

    TextureHandle attachments[1] = { k_invalidHandle };
    attachments[0]               = CreateTexture2D(_width, _height, _format, 1, flags);
    const FrameBufferHandle fbh  = CreateFrameBuffer(attachments, true);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetFrameBufferName(fbh, g_pGfxCtx->SourceLocToString(loc));
#endif

    return fbh;
}

TextureHandle Graphics::LoadTexture(
    const DirectX::ScratchImage& _scratchImage)
{
    ID3D11Resource* pResource = nullptr;
    JAM_CHECK_COM(DirectX::CreateTexture(
        g_pGfxCtx->pd3d11Device,
        _scratchImage.GetImages(),
        _scratchImage.GetImageCount(),
        _scratchImage.GetMetadata(),
        &pResource));

    const DirectX::TexMetadata& metadata = _scratchImage.GetMetadata();

    TextureRef tex;
    tex.width     = NarrowCast<Int32>(metadata.width);
    tex.height    = NarrowCast<Int32>(metadata.height);
    tex.depth     = NarrowCast<Int32>(metadata.depth);
    tex.format    = GetTextureFormatFromPlatform(metadata.format);
    tex.arraySize = NarrowCast<Int32>(metadata.arraySize);
    tex.samples   = 1;
    tex.numMips   = NarrowCast<Int32>(metadata.mipLevels);
    tex.refCount  = 1;
    tex.bManaged  = false;
    tex.bMutable  = false;
    tex.pResource = pResource;
    tex.type      = metadata.IsCubemap()                                   ? eTextureType::TextureCube
                  : metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D ? eTextureType::Texture3D
                  : metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D ? eTextureType::Texture2D
                                                                           : eTextureType::Unknown;

    JAM_ASSERT(tex.type != eTextureType::Unknown, "Unsupported texture type.");

    JAM_CHECK_COM(DirectX::CreateShaderResourceView(
        g_pGfxCtx->pd3d11Device,
        _scratchImage.GetImages(),
        _scratchImage.GetImageCount(),
        _scratchImage.GetMetadata(),
        &tex.pSRV));

    const TextureHandle texh  = g_pGfxCtx->textures.Alloc();
    g_pGfxCtx->textures[texh] = std::move(tex);

#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
    const SourceLoc loc = TraceCallerInfo();
    SetTextureName(texh, g_pGfxCtx->SourceLocToString(loc));
#else
    SetTextureName(texh);
#endif

    return texh;
}

std::optional<TextureHandle> Graphics::LoadTextureFromFile(
    const fs::path& _path)
{
    ImageImporter importer;
    importer.SetPath(_path);

    if (importer.Import())
    {
        const TextureHandle texh = LoadTexture(importer.GetScratchImage());
#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
        const SourceLoc loc = TraceCallerInfo();
        SetTextureName(texh, g_pGfxCtx->SourceLocToString(loc));
#endif
        return texh;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<TextureHandle> Graphics::LoadTextureFromMemory(
    const ConstMemView _mem,
    const eImageFormat _format)
{
    ImageImporter importer;
    importer.SetMemory(_mem, _format);

    if (importer.Import())
    {
        const TextureHandle texh = LoadTexture(importer.GetScratchImage());
#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
        const SourceLoc loc = TraceCallerInfo();
        SetTextureName(texh, g_pGfxCtx->SourceLocToString(loc));
#endif
        return texh;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<ShaderHandle> Graphics::LoadShaderFromFile(
    const fs::path&        _path,
    const eShader          _stage,
    const std::string_view _entryPoint)
{
    ShaderCompiler compiler;
    if (compiler.CompileFromFile(_path, _stage, _entryPoint))
    {
        const ShaderHandle sh = CreateShader(compiler.GetBytecode());
#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
        const SourceLoc loc = TraceCallerInfo();
        SetShaderName(sh, g_pGfxCtx->SourceLocToString(loc));
#endif
        return sh;
    }
    else
    {
        return std::nullopt;
    }
}

std::optional<ShaderHandle> Graphics::LoadShaderFromSource(
    const std::string_view _source,
    const eShader          _stage,
    const std::string_view _entryPoint)
{
    ShaderCompiler compiler;
    if (compiler.CompileFromSource(_source, _stage, _entryPoint))
    {
        const ShaderHandle sh = CreateShader(compiler.GetBytecode());
#ifdef JAM_GRAPHICS_TRACE_RESOURCE_CREATION_CALLER
        const SourceLoc loc = TraceCallerInfo();
        SetShaderName(sh, g_pGfxCtx->SourceLocToString(loc));
#endif
        return sh;
    }
    else
    {
        return std::nullopt;
    }
}

void Graphics::Submit()
{
    g_pGfxCtx->ApplyPipeline();
    if (JAM_IS_VALID_HANDLE(g_pGfxCtx->ibhBind))
    {
        const Int32 numIndex    = g_pGfxCtx->numIndicesBind;
        const Int32 startIdxLoc = g_pGfxCtx->submitParameters[EnumToInt(eSubmitParameterType::StartIndexLocation)];
        const Int32 baseVtxLoc  = g_pGfxCtx->submitParameters[EnumToInt(eSubmitParameterType::BaseVertexLocation)];
        g_pGfxCtx->pd3d11DeviceContext->DrawIndexed(numIndex, startIdxLoc, baseVtxLoc);
    }
    else
    {
        const Int32 numVerts     = g_pGfxCtx->numVertsBind;
        const Int32 startVertLoc = g_pGfxCtx->submitParameters[EnumToInt(eSubmitParameterType::StartVertexLocation)];
        g_pGfxCtx->pd3d11DeviceContext->Draw(numVerts, startVertLoc);
    }
}

}   // namespace jam