#include "pch.h"

#include "Graphics.h"
#include <d3d11shader.h>
#include <DirectXTex.h>
#include <d3dcompiler.h>

#include "BaseUtilities.h"
#include "Event.h"
#include "MemoryHasher.h"
#include "ResourcePool.h"
#include "WindowsPlatform.h"
#include "GeometryUtilities.h"
#include "ShaderCompiler.h"

namespace Jam
{

namespace
{
    struct SamplerInfo
    {
        D3D11_FILTER filter           = {};
        D3D11_FILTER comparisonFilter = {};
    };

    struct HLSLSemanticInfo
    {
        StringView name  = {};
        UINT       index = {};
    };

    struct HLSLDataTypeInfo
    {
        StringView  name   = {};
        DXGI_FORMAT format = {};
    };

    [[nodiscard]] D3D11_BLEND D3D11BlendFrom_(const eBlend _blend)
    {
        switch (_blend)
        {
            case eBlend::Zero:
                return D3D11_BLEND_ZERO;
            case eBlend::One:
                return D3D11_BLEND_ONE;
            case eBlend::SrcColor:
                return D3D11_BLEND_SRC_COLOR;
            case eBlend::InvSrcColor:
                return D3D11_BLEND_INV_SRC_COLOR;
            case eBlend::SrcAlpha:
                return D3D11_BLEND_SRC_ALPHA;
            case eBlend::InvSrcAlpha:
                return D3D11_BLEND_INV_SRC_ALPHA;
            case eBlend::DstAlpha:
                return D3D11_BLEND_DEST_ALPHA;
            case eBlend::InvDstAlpha:
                return D3D11_BLEND_INV_DEST_ALPHA;
            case eBlend::DstColor:
                return D3D11_BLEND_DEST_COLOR;
            case eBlend::InvDstColor:
                return D3D11_BLEND_INV_DEST_COLOR;
            case eBlend::SrcAlphaSat:
                return D3D11_BLEND_SRC_ALPHA_SAT;
            case eBlend::BlendFactor:
                return D3D11_BLEND_BLEND_FACTOR;
            case eBlend::InvBlendFactor:
                return D3D11_BLEND_INV_BLEND_FACTOR;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] D3D11_BLEND_OP D3D11BlendOpFrom_(
        const eBlendOp _blendOp)
    {
        switch (_blendOp)
        {
            case eBlendOp::Add:
                return D3D11_BLEND_OP_ADD;
            case eBlendOp::Subtract:
                return D3D11_BLEND_OP_SUBTRACT;
            case eBlendOp::RevSubtract:
                return D3D11_BLEND_OP_REV_SUBTRACT;
            case eBlendOp::Min:
                return D3D11_BLEND_OP_MIN;
            case eBlendOp::Max:
                return D3D11_BLEND_OP_MAX;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] D3D11_COMPARISON_FUNC D3D11ComparisonFuncFrom_(
        const eCompareFunc _func)
    {
        switch (_func)
        {
            case eCompareFunc::Never:
                return D3D11_COMPARISON_NEVER;
            case eCompareFunc::Less:
                return D3D11_COMPARISON_LESS;
            case eCompareFunc::Equal:
                return D3D11_COMPARISON_EQUAL;
            case eCompareFunc::LessEqual:
                return D3D11_COMPARISON_LESS_EQUAL;
            case eCompareFunc::Greater:
                return D3D11_COMPARISON_GREATER;
            case eCompareFunc::NotEqual:
                return D3D11_COMPARISON_NOT_EQUAL;
            case eCompareFunc::GreaterEqual:
                return D3D11_COMPARISON_GREATER_EQUAL;
            case eCompareFunc::Always:
                return D3D11_COMPARISON_ALWAYS;
            case eCompareFunc::None:
                return D3D11_COMPARISON_LESS;   // None 일 경우 comparisonFunc은 의미 없음. 단 d3d11 에러를 피하기 위해서 placeholder
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] D3D11_STENCIL_OP D3D11StencilOpFrom_(
        const eStencilOp _op)
    {
        switch (_op)
        {
            case eStencilOp::Keep:
                return D3D11_STENCIL_OP_KEEP;
            case eStencilOp::Zero:
                return D3D11_STENCIL_OP_ZERO;
            case eStencilOp::Replace:
                return D3D11_STENCIL_OP_REPLACE;
            case eStencilOp::IncrSat:
                return D3D11_STENCIL_OP_INCR_SAT;
            case eStencilOp::DecrSat:
                return D3D11_STENCIL_OP_DECR_SAT;
            case eStencilOp::Invert:
                return D3D11_STENCIL_OP_INVERT;
            case eStencilOp::Incr:
                return D3D11_STENCIL_OP_INCR;
            case eStencilOp::Decr:
                return D3D11_STENCIL_OP_DECR;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] D3D11_TEXTURE_ADDRESS_MODE D3D11TextureAddressModeFrom_(
        const eTextureAddressMode _mode)
    {
        switch (_mode)
        {
            case eTextureAddressMode::Wrap:
                return D3D11_TEXTURE_ADDRESS_WRAP;
            case eTextureAddressMode::Mirror:
                return D3D11_TEXTURE_ADDRESS_MIRROR;
            case eTextureAddressMode::Clamp:
                return D3D11_TEXTURE_ADDRESS_CLAMP;
            case eTextureAddressMode::Border:
                return D3D11_TEXTURE_ADDRESS_BORDER;
            case eTextureAddressMode::MirrorOnce:
                return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] SamplerInfo SamplerInfoFrom_(
        const eTextureFilter _filter)
    {
        switch (_filter)
        {
            case eTextureFilter::MinPoint_MagPoint_MipPoint:
                return { D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT };
            case eTextureFilter::MinPoint_MagPoint_MipLinear:
                return { D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR };
            case eTextureFilter::MinPoint_MagLinear_MipPoint:
                return { D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT };
            case eTextureFilter::MinPoint_MagLinear_MipLinear:
                return { D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR };
            case eTextureFilter::MinLinear_MagPoint_MipPoint:
                return { D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT };
            case eTextureFilter::MinLinear_MagPoint_MipLinear:
                return { D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR };
            case eTextureFilter::MinLinear_MagLinear_MipPoint:
                return { D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT };
            case eTextureFilter::MinLinear_MagLinear_MipLinear:
                return { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR };
            case eTextureFilter::Anisotropic4:
            case eTextureFilter::Anisotropic8:
            case eTextureFilter::Anisotropic16:
                return { D3D11_FILTER_ANISOTROPIC, D3D11_FILTER_COMPARISON_ANISOTROPIC };
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] HLSLSemanticInfo HLSLSemanticInfoFrom_(
        const eVertexAttribute _attrib)
    {
        switch (_attrib)
        {
            case eVertexAttribute::Position:
                return { "POSITION", 0 };
            case eVertexAttribute::Normal:
                return { "NORMAL", 0 };
            case eVertexAttribute::Tangent:
                return { "TANGENT", 0 };
            case eVertexAttribute::Bitangent:
                return { "BINORMAL", 0 };
            case eVertexAttribute::Color0:
                return { "COLOR", 0 };
            case eVertexAttribute::Color1:
                return { "COLOR", 1 };
            case eVertexAttribute::Color2:
                return { "COLOR", 2 };
            case eVertexAttribute::Color3:
                return { "COLOR", 3 };
            case eVertexAttribute::TexCoord0:
                return { "TEXCOORD", 0 };
            case eVertexAttribute::TexCoord1:
                return { "TEXCOORD", 1 };
            case eVertexAttribute::TexCoord2:
                return { "TEXCOORD", 2 };
            case eVertexAttribute::TexCoord3:
                return { "TEXCOORD", 3 };
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] HLSLDataTypeInfo HLSLDataTypeInfoFrom_(
        const eAttributeFormat _format)
    {
        switch (_format)
        {
            case eAttributeFormat::Float:
                return { "float", DXGI_FORMAT_R32_FLOAT };
            case eAttributeFormat::Float2:
                return { "float2", DXGI_FORMAT_R32G32_FLOAT };
            case eAttributeFormat::Float3:
                return { "float3", DXGI_FORMAT_R32G32B32_FLOAT };
            case eAttributeFormat::Float4:
                return { "float4", DXGI_FORMAT_R32G32B32A32_FLOAT };
            case eAttributeFormat::UNorm8x4:
                return { "float4", DXGI_FORMAT_R8G8B8A8_UNORM };
            case eAttributeFormat::UInt8x4:
                return { "uint4", DXGI_FORMAT_R8G8B8A8_UINT };
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] String CreateName_(
        const StringView _handleName,
        const AnyHandle  _handle,
        const StringView _userName)
    {
        String result = {};
        result.Format("[{}] index: {}, token: {}", _handleName, _handle.index, _handle.token);
        if (!_userName.IsEmpty())
        {
            result.AppendFormat(", name: {}", _userName);
        }

        return result;
    }

    void SetD3DDebugName_(
        ID3D11DeviceChild* _pDeviceChild,
        const StringView   _name)
    {
        JAM_ASSERT(_pDeviceChild, "Resource is null");
        JAM_CHECK_HRESULT(_pDeviceChild->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr));
        JAM_CHECK_HRESULT(_pDeviceChild->SetPrivateData(WKPDID_D3DDebugObjectName, NumericCast<UInt32>(_name.GetLength()), _name.GetPtr()));
    }
}   // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Graphics& Graphics::GetInstance()
{
    return *s_pInstance;
}

bool Graphics::DetectLeak()
{
    bool bLeakDetected = false;

    for (const auto& [handle, value]: m_vertexBuffers)
    {
        const UInt32 index = handle.index;
        const UInt32 token = handle.token;
        JAM_LOG_WARN("VertexBuffer leak detected: Handle pos = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_indexBuffers)
    {
        const UInt32 index = handle.index;
        const UInt32 token = handle.token;
        JAM_LOG_WARN("IndexBuffer leak detected: Handle pos = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_textures)
    {
        const UInt32 index = handle.index;
        const UInt32 token = handle.token;
        JAM_LOG_WARN("Texture leak detected: Handle pos = {}, Token = {}, Name = '{}', RefCount = {}", index, token, value.name, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_frameBuffers)
    {
        const UInt32 index = handle.index;
        const UInt32 token = handle.token;
        JAM_LOG_WARN("FrameBuffer leak detected: Handle pos = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_shaders)
    {
        constexpr EnumArray<eShader, const char*> k_names = {
            "VertexShader", "PixelShader", "GeometryShader", "HullShader", "DomainShader", "ComputeShader"
        };

        const UInt32 index     = handle.index;
        const UInt32 token     = handle.token;
        const char*  stageName = k_names[value.stage];
        JAM_LOG_WARN("Shader leak detected: Handle pos = {}, Token = {}, Stage = {}, RefCount = {}", index, token, stageName, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_vertexLayouts)
    {
        const UInt32 index = handle.index;
        const UInt32 token = handle.token;
        JAM_LOG_WARN("VertexLayout leak detected: Handle pos = {}, Token = {}, RefCount = {}", index, token, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_cbuffers)
    {
        const UInt32 index = handle.index;
        const UInt32 token = handle.token;
        JAM_LOG_WARN("ConstantBuffer leak detected: Handle pos = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const ProgramHandle handle: m_programs | std::views::keys)
    {
        const UInt32 index = handle.index;
        const UInt32 token = handle.token;
        JAM_LOG_WARN("Program leak detected: Handle pos = {}, Token = {}", index, token);
        bLeakDetected = true;
    }

    return bLeakDetected;
}

void Graphics::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<WindowResizedEvent>(this, &Graphics::OnWindowResizedEvent_);
    dispatcher.Dispatch<WindowMaximizedEvent>(this, &Graphics::OnWindowMaximizeEvent_);
}

void Jam::Graphics::Frame(const bool _bVSync)
{
    const HRESULT hr = m_pdxgiSwapchain->Present(_bVSync ? 1 : 0, m_bAllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0);
    if (FAILED(hr))
    {
        JAM_CHECK_HRESULT(m_pd3d11Device->GetDeviceRemovedReason());
    }

    m_fbhBind = k_nullHandle;   // frame buffer must rebind after present (this is api demand)
}
VertexBufferHandle Graphics::CreateVertexBuffer(
    const MemoryView    _vertData,
    const VertexLayout& _vl)
{
    const Int32          byteWidth = NumericCast<Int32>(_vertData.GetByteWidth());
    const HardwareBuffer hwBuffer  = CreateHardwareBuffer_(byteWidth, eBufferType::VertexBuffer, _vertData);

    VertexBuffer vb = {};
    vb.pBuffer      = hwBuffer.pBuffer;
    vb.byteWidth    = hwBuffer.byteWidth;
    vb.bDynamic     = hwBuffer.bDynamic;
    vb.stride       = _vl.GetStride();
    vb.numVerts     = byteWidth / vb.stride;
    vb.vlh          = GetOrCreateVertexLayout_(_vl);

    const VertexBufferHandle vbh = m_vertexBuffers.Alloc();
    m_vertexBuffers[vbh]         = std::move(vb);
    SetVertexBufferName(vbh, "");
    return vbh;
}

VertexBufferHandle Graphics::CreateDynamicVertexBuffer(
    const Int32         _numVerts,
    const VertexLayout& _vl)
{
    JAM_ASSERT(_numVerts > 0, "Vertex count must be greater than 0");

    const Int32          stride   = _vl.GetStride();
    const HardwareBuffer hwBuffer = CreateHardwareBuffer_(_numVerts * stride, eBufferType::VertexBuffer, {});

    VertexBuffer vb = {};
    vb.pBuffer      = hwBuffer.pBuffer;
    vb.byteWidth    = hwBuffer.byteWidth;
    vb.bDynamic     = hwBuffer.bDynamic;
    vb.stride       = stride;
    vb.numVerts     = _numVerts;
    vb.vlh          = GetOrCreateVertexLayout_(_vl);

    const VertexBufferHandle vbh = m_vertexBuffers.Alloc();
    m_vertexBuffers[vbh]         = std::move(vb);
    SetVertexBufferName(vbh, "");
    return vbh;
}

IndexBufferHandle Graphics::CreateIndexBuffer(
    const MemoryView _idxData,
    const bool       _bUseU32)
{
    const Int32          byteWidth = NumericCast<Int32>(_idxData.GetByteWidth());
    const HardwareBuffer hwBuffer  = CreateHardwareBuffer_(byteWidth, eBufferType::IndexBuffer, _idxData);

    IndexBuffer ib = {};
    ib.pBuffer     = hwBuffer.pBuffer;
    ib.byteWidth   = hwBuffer.byteWidth;
    ib.bDynamic    = hwBuffer.bDynamic;
    ib.numIdxes    = CalcNumIndices(_idxData, _bUseU32);
    ib.stride      = _bUseU32 ? 4 : 2;
    ib.bUseU32     = _bUseU32;

    const IndexBufferHandle ibh = m_indexBuffers.Alloc();
    m_indexBuffers[ibh]         = std::move(ib);
    SetIndexBufferName(ibh, "");
    return ibh;
}

IndexBufferHandle Graphics::CreateDynamicIndexBuffer(
    const Int32 _numIndices,
    const bool  _bUseU32)
{
    JAM_ASSERT(_numIndices > 0, "Index count must be greater than 0");

    const Int32          stride   = _bUseU32 ? 4 : 2;
    const HardwareBuffer hwBuffer = CreateHardwareBuffer_(_numIndices * stride, eBufferType::IndexBuffer, {});

    IndexBuffer ib = {};
    ib.pBuffer     = hwBuffer.pBuffer;
    ib.byteWidth   = hwBuffer.byteWidth;
    ib.bDynamic    = hwBuffer.bDynamic;
    ib.numIdxes    = _numIndices;
    ib.stride      = stride;
    ib.bUseU32     = _bUseU32;

    const IndexBufferHandle ibh = m_indexBuffers.Alloc();
    m_indexBuffers[ibh]         = std::move(ib);
    SetIndexBufferName(ibh, "");
    return ibh;
}

ConstantBufferHandle Graphics::CreateConstantBuffer(const Int32 _byteWidth)
{
    JAM_ASSERT(_byteWidth > 0 && _byteWidth <= 0xFFFF, "Constant buffer size must be between 1 and 65536 bytes");
    JAM_ASSERT(_byteWidth % 16 == 0, "Constant buffer size must be a multiple of 16 bytes");

    const HardwareBuffer hwBuffer = CreateHardwareBuffer_(_byteWidth, eBufferType::ConstantBuffer, {});

    ConstantBuffer cb = {};
    cb.pBuffer        = hwBuffer.pBuffer;
    cb.byteWidth      = hwBuffer.byteWidth;
    cb.bDynamic       = hwBuffer.bDynamic;

    const ConstantBufferHandle cbh = m_cbuffers.Alloc();
    m_cbuffers[cbh]                = std::move(cb);
    SetConstantBufferName(cbh, "");
    return cbh;
}

TextureHandle Graphics::CreateTexture2D(
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const Int32                      _arraySize,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initDataOrEmpty)
{
    const TextureHandle texh = m_textures.Alloc();
    m_textures[texh]         = CreateHardwareTexture_(_width, _height, 1, _format, _arraySize, _flags, eTextureType::Texture2D, _initDataOrEmpty);
    SetTextureName(texh, "");
    return texh;
}

TextureHandle Graphics::CreateTextureCube(
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const Int32                      _cubeCount,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initDataOrEmpty)
{
    const TextureHandle texh = m_textures.Alloc();
    m_textures[texh]         = CreateHardwareTexture_(_width, _height, 1, _format, _cubeCount * 6, _flags, eTextureType::TextureCube, _initDataOrEmpty);
    SetTextureName(texh, "");
    return texh;
}

TextureHandle Graphics::CreateTexture3D(
    const Int32                      _width,
    const Int32                      _height,
    const Int32                      _depth,
    const eTextureFormat             _format,
    const Flags<eTextureCreateFlags> _texFlags,
    const Span<const MemoryView>     _initDataOrEmpty)
{
    const TextureHandle texh = m_textures.Alloc();
    m_textures[texh]         = CreateHardwareTexture_(_width, _height, _depth, _format, 1, _texFlags, eTextureType::Texture3D, _initDataOrEmpty);
    SetTextureName(texh, "");
    return texh;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const Span<const TextureHandle> _attachments,
    const bool                      _bDestroyTexture)
{
    const FrameBufferHandle fbh = m_frameBuffers.Alloc();
    m_frameBuffers[fbh]         = CreateFrameBuffer_(_attachments, _bDestroyTexture);
    SetFrameBufferName(fbh, "");
    return fbh;
}

ShaderHandle Graphics::CreateShader(
    const MemoryView _shaderByteCode)
{
    return GetOrCreateShader_(_shaderByteCode);
}

ProgramHandle Graphics::CreateProgram(
    const ShaderHandle _vsh,
    const ShaderHandle _psh,
    const ShaderHandle _hsh,
    const ShaderHandle _dsh,
    const ShaderHandle _gsh,
    const bool         _bDestroyShader)
{
    JAM_ASSERT(_vsh || _psh, "At least one of VS or PS must be provided");

    EnumArray<eShader, ShaderHandle> shs = { _vsh, _psh, _hsh, _dsh, _gsh, k_nullHandle /* compute shader */ };

    Program program = {};
    for (eShader stage: k_gfxShaderStages)
    {
        if (const ShaderHandle sh = shs[stage])
        {
            ShaderRef& shader = m_shaders[sh];
            JAM_ASSERT(shader.stage == stage, "Shader handle does not match the expected shader stage");

            // add ref
            if (_bDestroyShader == false)
            {
                ++shader.refCount;
            }

            program.shs[stage] = sh;
        }
    }

    const ProgramHandle ph = m_programs.Alloc();
    m_programs[ph]         = std::move(program);
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
        k_nullHandle,
        k_nullHandle,
        k_nullHandle,
        _bDestroyShader);
}

void Graphics::DestroyVertexBuffer(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];

    // destory d3d11 resource
    ID3D11Buffer* pd3d11Buffer = vb.pBuffer;
    if (pd3d11Buffer->Release() != 0)
    {
        JAM_ASSERT(0, "Vertex pBuffer reference count is not zero.");
    }

    // release VertexLayoutRef
    {
        const VertexLayoutHandle vlh = vb.vlh;
        VertexLayoutRef&         vl  = m_vertexLayouts[vlh];
        if (--vl.refCount == 0)
        {
            if (vl.pInputLayout->Release() != 0)   // destory d3d11 resource
            {
                JAM_ASSERT(0, "Input layout reference count is not zero");
            }

            m_vlCache.erase(vl.hash);
            m_vertexLayouts.Free(vlh);
        }

        m_vertexBuffers.Free(_vbh);
    }
}

void Graphics::DestroyIndexBuffer(const IndexBufferHandle _ibh)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];

    // destroy d3d11 resource
    ID3D11Buffer* pd3d11Buffer = ib.pBuffer;
    if (pd3d11Buffer->Release() != 0)
    {
        JAM_ASSERT(0, "Index pBuffer reference count is not zero.");
    }

    // free handle
    m_indexBuffers.Free(_ibh);
}

void Graphics::DestroyTexture(
    const TextureHandle _texh)
{
    // 텍스처 참조 해제
    HardwareTextureRef& texture = m_textures[_texh];

    // 텍스처 파괴
    if (--texture.refCount == 0)
    {
        if (texture.pSrv)   // SRV 파괴
        {
            if (texture.pSrv->Release() != 0)
            {
                JAM_ASSERT(0, "Shader resource view reference count is not zero.");
            }
        }

        // 텍스처 리소스 파괴
        if (texture.pResource->Release() != 0)
        {
            JAM_ASSERT(0, "Texture resource reference count is not zero.");
        }

        m_textures.Free(_texh);   // 핸들 해제
    }
}

void Graphics::DestroyFrameBuffer(
    const FrameBufferHandle _fbh)
{
    JAM_ASSERT(_fbh, "Invalid FrameBuffer handle");

    const FrameBuffer& fb = m_frameBuffers[_fbh];

    // destory rtv
    for (Int32 i = 0; i < fb.numRenderTargets; ++i)
    {
        ID3D11RenderTargetView* pRTV = fb.pRtvs[i];
        if (pRTV->Release() != 0)
        {
            JAM_ASSERT(0, "Render target view reference count is not zero");
        }

        DestroyTexture(fb.renderTargetTexhs[i]);   // TextureHandle 해제
    }

    // DSV 파괴
    if (fb.pDsv)
    {
        ID3D11DepthStencilView* pDSV = fb.pDsv;
        if (pDSV->Release() != 0)
        {
            JAM_ASSERT(0, "Depth stencil view reference count is not zero");
        }

        DestroyTexture(fb.depthStencilTexh);   // TextureHandle 해제
    }

    m_frameBuffers.Free(_fbh);   // FrameBuffer 핸들 해제
}

void Graphics::DestroyProgram(
    const ProgramHandle _ph)
{
    Program& program = m_programs[_ph];

    for (ShaderHandle sh: program.shs)
    {
        if (sh)
        {
            DestroyShader(sh);   // 셰이더 참조 해제
        }
    }

    m_programs.Free(_ph);   // Program 핸들 해제
}

void Graphics::DestroyShader(
    const ShaderHandle _sh)
{
    ShaderRef& shader = m_shaders[_sh];

    // 참조 해제
    if (--shader.refCount == 0)
    {
        // d3d 쉐이더 객체 파괴
        switch (shader.stage)
        {
            case eShader::VertexShader:
                if (shader.pVertexShader->Release() != 0)
                {
                    JAM_ASSERT(0, "Vertex shader reference count is not zero.");
                }
                break;
            case eShader::PixelShader:
                if (shader.pPixelShader->Release() != 0)
                {
                    JAM_ASSERT(0, "Pixel shader reference count is not zero.");
                }
                break;
            case eShader::GeometryShader:
                if (shader.pGeometryShader->Release() != 0)
                {
                    JAM_ASSERT(0, "Geometry shader reference count is not zero.");
                }
                break;
            case eShader::HullShader:
                if (shader.pHullShader->Release() != 0)
                {
                    JAM_ASSERT(0, "Hull shader reference count is not zero.");
                }
                break;
            case eShader::DomainShader:
                if (shader.pDomainShader->Release() != 0)
                {
                    JAM_ASSERT(0, "Domain shader reference count is not zero.");
                }
                break;
            case eShader::ComputeShader:
                if (shader.pComputeShader->Release() != 0)
                {
                    JAM_ASSERT(0, "Compute shader reference count is not zero.");
                }
                break;
            default:
                JAM_UNREACHABLE;
        }

        m_shaderCache.erase(shader.cache);   // 캐시에서 제거
        m_shaders.Free(_sh);                 // Shader 핸들 해제
    }
}

void Graphics::DestroyConstantBuffer(
    const ConstantBufferHandle _cbh)
{
    const ConstantBuffer& cbuffer  = m_cbuffers[_cbh];
    const UINT            refCount = cbuffer.pBuffer->Release();
    if (refCount > 0)
    {
        JAM_UNREACHABLE;
    }

    m_cbuffers.Free(_cbh);   // 핸들 해제
}

void Graphics::SetVertexBufferName(
    const VertexBufferHandle _vbh,
    const StringView         _name)
{
    VertexBuffer& vb = m_vertexBuffers[_vbh];
    vb.name          = CreateName_("VertexBuffer", _vbh, _name);
    SetD3DDebugName_(vb.pBuffer, vb.name);
}

void Graphics::SetIndexBufferName(
    const IndexBufferHandle _ibh,
    const StringView        _name)
{
    IndexBuffer& ib = m_indexBuffers[_ibh];
    ib.name         = CreateName_("IndexBuffer", _ibh, _name);
    SetD3DDebugName_(ib.pBuffer, ib.name);
}

void Graphics::SetConstantBufferName(
    const ConstantBufferHandle _cbh,
    const StringView           _name)
{
    ConstantBuffer& cb = m_cbuffers[_cbh];
    cb.name            = CreateName_("ConstantBuffer", _cbh, _name);
    SetD3DDebugName_(cb.pBuffer, cb.name);
}

void Graphics::SetTextureName(
    const TextureHandle _texh,
    const StringView    _name)
{
    constexpr EnumArray<eTextureType, const char*> k_texNames = { "Texture2D", "Texture3D", "TextureCube" };
    constexpr EnumArray<eTextureType, const char*> k_srvNames = { "Texture2D_SRV", "Texture3D_SRV", "TextureCube_SRV" };

    HardwareTextureRef& texture = m_textures[_texh];

    // 리소스 이름 설정
    texture.name = CreateName_(k_texNames[texture.type], _texh, _name);
    SetD3DDebugName_(texture.pResource, texture.name);

    // SRV 이름 설정
    if (texture.pSrv)
    {
        SetD3DDebugName_(texture.pSrv, CreateName_(k_srvNames[texture.type], _texh, _name));
    }
}

void Graphics::SetFrameBufferName(
    const FrameBufferHandle _fbh,
    const StringView        _name)
{
    constexpr const char* k_names[] = {
        "FrameBuffer_RTV0",
        "FrameBuffer_RTV1",
        "FrameBuffer_RTV2",
        "FrameBuffer_RTV3",
        "FrameBuffer_RTV4",
        "FrameBuffer_RTV5",
        "FrameBuffer_RTV6",
        "FrameBuffer_RTV7"
    };

    FrameBuffer& fb = m_frameBuffers[_fbh];

    for (Int32 i = 0; i < fb.numRenderTargets; ++i)
    {
        if (fb.pRtvs[i])
        {
            SetD3DDebugName_(fb.pRtvs[i], CreateName_(k_names[i], _fbh, _name));
        }
    }

    if (fb.pDsv)
    {
        SetD3DDebugName_(fb.pDsv, CreateName_("FrameBuffer_DSV", _fbh, _name));
    }
}

void Graphics::SetShaderName(
    const ShaderHandle _sh,
    const StringView   _name)
{
    constexpr EnumArray<eShader, const char*> k_names = {
        "VertexShader",
        "PixelShader",
        "GeometryShader",
        "HullShader",
        "DomainShader",
        "ComputeShader"
    };

    ShaderRef& shader = m_shaders[_sh];
    shader.name       = CreateName_(k_names[shader.stage], _sh, _name);

    switch (shader.stage)
    {
        case eShader::VertexShader:
            SetD3DDebugName_(shader.pVertexShader, shader.name);
            break;
        case eShader::PixelShader:
            SetD3DDebugName_(shader.pPixelShader, shader.name);
            break;
        case eShader::GeometryShader:
            SetD3DDebugName_(shader.pGeometryShader, shader.name);
            break;
        case eShader::HullShader:
            SetD3DDebugName_(shader.pHullShader, shader.name);
            break;
        case eShader::DomainShader:
            SetD3DDebugName_(shader.pDomainShader, shader.name);
            break;
        case eShader::ComputeShader:
            SetD3DDebugName_(shader.pComputeShader, shader.name);
            break;
        default:
            JAM_UNREACHABLE;
    }
}

void Graphics::Blit(
    const TextureHandle _blitDstTexh,
    const TextureHandle _blitSrcTexh)
{
    const HardwareTextureRef& dstTex = m_textures[_blitDstTexh];
    const HardwareTextureRef& srcTex = m_textures[_blitSrcTexh];
    Blit_(dstTex, 0, 0, 0, 0, 0, srcTex, 0, 0, 0, 0, 0, 0, 0, 0);
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
    const HardwareTextureRef& dstTex = m_textures[_blitDstTexh];
    const HardwareTextureRef& srcTex = m_textures[_blitSrcTexh];
    Blit_(
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
    const MemoryView    _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDataRowPitch)
{
    const HardwareTextureRef& texture = m_textures[_texh];
    UploadTexture_(
        texture,
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
    const MemoryView    _uploadData,
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
    const MemoryView    _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDataRowPitch)
{
    const HardwareTextureRef& texture = m_textures[_texh];
    UploadTexture_(
        texture,
        _dstMipIndex,
        _dstCubeIndex * 6 + UnderlyingFrom(_dstCubeFace),
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
    const MemoryView    _uploadData,
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
    const MemoryView    _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDepth,
    const Int32         _uploadDataRowPitch,
    const Int32         _uploadDataSlicePitch)
{
    const HardwareTextureRef& texture = m_textures[_texh];
    UploadTexture_(
        texture,
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
        _uploadDataSlicePitch);
}
void Graphics::UploadTexture3D(
    const TextureHandle _texh,
    const MemoryView    _uploadData,
    const Int32         _uploadWidth,
    const Int32         _uploadHeight,
    const Int32         _uploadDepth,
    const Int32         _uploadDataRowPitch,
    const Int32         _uploadDataSlicePitch)
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
        _uploadDataSlicePitch);
}

Vector<Byte> Graphics::DownloadTexture2D(
    const TextureHandle _texh,
    const Int32         _mipIndex,
    const Int32         _arrayIndex)
{
    const HardwareTextureRef& texture = m_textures[_texh];
    return DownloadTexture_(
        texture,
        _mipIndex,
        _arrayIndex);
}

Vector<Byte> Graphics::DownloadTextureCube(
    const TextureHandle _texh,
    const Int32         _mipIndex,
    const Int32         _cubeIndex,
    const eCubeFace     _cubeFace)
{
    const HardwareTextureRef& texture = m_textures[_texh];
    return DownloadTexture_(
        texture,
        _mipIndex,
        _cubeIndex * 6 + UnderlyingFrom(_cubeFace));
}

Vector<Byte> Graphics::DownloadTexture3D(
    const TextureHandle _texh,
    const Int32         _mipIndex)
{
    const HardwareTextureRef& texture = m_textures[_texh];
    return DownloadTexture_(texture, _mipIndex, 0);
}

ImageContainer Graphics::Capture(
    const TextureHandle _texh)
{
    const HardwareTextureRef& texture   = m_textures[_texh];
    ImageContainer            container = {};
    JAM_CHECK_HRESULT(DirectX::CaptureTexture(m_pd3d11Device, m_pd3d11DeviceContext, texture.pResource, container));
    return container;
}

void Graphics::Resolve(
    const TextureHandle _srcTexh,
    const Int32         _srcArrayIndex,
    const TextureHandle _dstTexh,
    const Int32         _dstArrayIndex)
{
    const HardwareTextureRef& srcTex = m_textures[_srcTexh];
    const HardwareTextureRef& dstTex = m_textures[_dstTexh];
    JAM_ASSERT(dstTex.samples == 1, "Destination texture must not be multi-sampled.");

    const Int32             srcIndex   = srcTex.numMips * _srcArrayIndex;
    const Int32             dstIndex   = dstTex.numMips * _dstArrayIndex;
    const TextureFormatInfo formatInfo = TextureFormatInfoFrom(dstTex.format);
    m_pd3d11DeviceContext->ResolveSubresource(
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
    const HardwareTextureRef& texture = m_textures[_texh];
    TextureResolution         res;
    res.width  = Max(1, texture.width >> _mipIndex);
    res.height = Max(1, texture.height >> _mipIndex);
    res.depth  = Max(1, texture.depth >> _mipIndex);
    return res;
}

void Graphics::ClearColor(
    const FrameBufferHandle _fbh,
    const RGBA              _color,
    const Int32             _renderTargetIndex)
{
    const FrameBufferHandle fbh = _fbh ? _fbh : m_backBufferFbh;
    const FrameBuffer&      fb  = m_frameBuffers[fbh];
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < fb.numRenderTargets, "Render target m_index out of range.");
    const FLOAT colors[4] = {
        NumericCast<float>(_color.r) / 255.0f,
        NumericCast<float>(_color.g) / 255.0f,
        NumericCast<float>(_color.b) / 255.0f,
        NumericCast<float>(_color.a) / 255.0f
    };
    m_pd3d11DeviceContext->ClearRenderTargetView(fb.pRtvs[_renderTargetIndex], colors);
}

void Graphics::ClearDepthStencil(
    const FrameBufferHandle _fbh,
    const bool              _bClearDepth,
    const bool              _bClearStencil,
    const float             _depth,
    const UInt8             _stencil)
{
    JAM_ASSERT(_fbh, "back buffer do not have depth stencil.");

    const FrameBuffer& fb = m_frameBuffers[_fbh];
    JAM_ASSERT(fb.pDsv, "Frame buffer does not have a depth stencil view.");

    UINT flags = 0;
    if (_bClearDepth)
        flags |= D3D11_CLEAR_DEPTH;
    if (_bClearStencil)
        flags |= D3D11_CLEAR_STENCIL;

    m_pd3d11DeviceContext->ClearDepthStencilView(fb.pDsv, flags, _depth, _stencil);
}

const TextureHandle& Graphics::GetRenderTargetTexture(
    const FrameBufferHandle _fbh,
    const Int32             _renderTargetIndex)
{
    const FrameBufferHandle fbh = _fbh ? _fbh : m_backBufferFbh;
    const FrameBuffer&      fb  = m_frameBuffers[fbh];
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < fb.numRenderTargets, "Render target m_index out of range.");
    return fb.renderTargetTexhs[_renderTargetIndex];
}

const TextureHandle& Graphics::GetDepthStencilTexture(
    const FrameBufferHandle _fbh)
{
    JAM_ASSERT(_fbh, "back buffer do not have depth stencil.");
    const FrameBuffer& fb = m_frameBuffers[_fbh];
    JAM_ASSERT(fb.pDsv, "Frame buffer does not have a depth stencil view.");
    return fb.depthStencilTexh;
}

const VertexLayout& Graphics::GetVertexLayout(
    const VertexLayoutHandle _vlh)
{
    const VertexLayoutRef& vl = m_vertexLayouts[_vlh];
    return vl.vertexLayout;
}

VertexBufferInfo Graphics::GetVertexBufferInfo(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];

    VertexBufferInfo vbi = {};
    vbi.byteWidth        = vb.byteWidth;
    vbi.numVerts         = vb.numVerts;
    vbi.vertexStride     = vb.stride;
    vbi.bDynamic         = vb.bDynamic;
    vbi.layoutHandle     = vb.vlh;
    return vbi;
}

IndexBufferInfo Graphics::GetIndexBufferInfo(
    const IndexBufferHandle _ibh)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];

    IndexBufferInfo info = {};
    info.byteWidth       = ib.byteWidth;
    info.numIndices      = ib.numIdxes;
    info.bUseU32         = ib.bUseU32;
    info.bDynamic        = ib.bDynamic;
    return info;
}

ConstantBufferInfo Graphics::GetConstantBufferInfo(
    const ConstantBufferHandle _cbh)
{
    const ConstantBuffer& cb = m_cbuffers[_cbh];

    ConstantBufferInfo info = {};
    info.byteWidth          = cb.byteWidth;
    return info;
}

TextureInfo Graphics::GetTextureInfo(
    const TextureHandle _texh)
{
    const HardwareTextureRef& texture = m_textures[_texh];

    TextureInfo info             = {};
    info.width                   = texture.width;
    info.height                  = texture.height;
    info.depth                   = texture.depth;
    info.type                    = texture.type;
    info.format                  = texture.format;
    info.arraySize               = texture.arraySize;
    info.samples                 = texture.samples;
    info.numMips                 = texture.numMips;
    info.bMutable                = texture.bMutable;
    info.bRenderTargetAttachable = texture.bRtvAttachable;
    info.bDepthStencilAttachable = texture.bDsvAttachable;
    return info;
}

FrameBufferInfo Graphics::GetFrameBufferInfo(
    const FrameBufferHandle _fbh)
{
    const FrameBufferHandle fbh = _fbh ? _fbh : m_backBufferFbh;
    const FrameBuffer&      fb  = m_frameBuffers[fbh];

    FrameBufferInfo info   = {};
    info.numRenderTargets  = fb.numRenderTargets;
    info.bHasDepthStencil  = fb.depthStencilTexh;
    info.renderTargetTexhs = fb.renderTargetTexhs;
    info.depthStencilTexh  = fb.depthStencilTexh;
    return info;
}

ShaderInfo Graphics::GetShaderInfo(
    const ShaderHandle _sh)
{
    const ShaderRef& shader = m_shaders[_sh];

    ShaderInfo si = {};
    si.stage      = shader.stage;
    return si;
}

ProgramInfo Graphics::GetProgramInfo(
    const ProgramHandle _ph)
{
    const Program& program = m_programs[_ph];

    ProgramInfo pi = {};
    pi.shs         = program.shs;
    return pi;
}

void Graphics::UploadBuffer(
    const VertexBufferHandle _vbh,
    const MemoryView         _writeData,
    const Int32              _offset,
    const bool               _bDiscard)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];
    UploadBuffer_(vb, _writeData, _offset, _bDiscard);
}

void Graphics::UploadBuffer(
    const IndexBufferHandle _ibh,
    const MemoryView        _writeData,
    const Int32             _offset,
    const bool              _bDiscard)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];
    UploadBuffer_(ib, _writeData, _offset, _bDiscard);
}

void Graphics::UploadBuffer(
    const ConstantBufferHandle _cbh,
    const MemoryView           _writeData)
{
    const ConstantBuffer& cb = m_cbuffers[_cbh];
    UploadBuffer_(cb, _writeData, 0, true);
}

MutableMemoryView Graphics::BeginUploadBuffer(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];
    return BeginUploadBuffer_(vb);
}

MutableMemoryView Graphics::BeginUploadBuffer(
    const IndexBufferHandle _ibh)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];
    return BeginUploadBuffer_(ib);
}

void Graphics::EndUploadBuffer(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];
    EndUploadBuffer_(vb);
}

void Graphics::EndUploadBuffer(
    const IndexBufferHandle _ibh)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];
    EndUploadBuffer_(ib);
}

Vector<Byte> Graphics::DownloadBuffer(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];
    return DownloadBuffer_(vb);
}

Vector<Byte> Graphics::DownloadBuffer(
    const IndexBufferHandle _ibh)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];
    return DownloadBuffer_(ib);
}

Vector<Byte> Graphics::DownloadBuffer(
    const ConstantBufferHandle _cbh)
{
    const ConstantBuffer& cb = m_cbuffers[_cbh];
    return DownloadBuffer_(cb);
}

const VertexLayout& Graphics::GetVertexLayout(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer&    vb = m_vertexBuffers[_vbh];
    const VertexLayoutRef& vl = m_vertexLayouts[vb.vlh];
    return vl.vertexLayout;
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _vbh)
{
    if (_vbh)
    {
        const VertexBuffer& vb = m_vertexBuffers[_vbh];
        SetVertexBuffer(_vbh, vb.numVerts, 0);
    }
    else
    {
        SetVertexBuffer(_vbh, 0, 0);
    }
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _vbhOrInvalid,
    const Int32              _numVerts,
    const Int32              _vertOffset)
{
    if (m_vbhBind != _vbhOrInvalid || m_vbOffsetBind != _vertOffset)
    {
        if (_vbhOrInvalid)
        {
            const VertexBuffer& vb = m_vertexBuffers[_vbhOrInvalid];
            if (m_vlhBind != vb.vlh)
            {
                m_vlhBind = vb.vlh;
                m_dirtyFlags |= ePipelineDirtyFlags::InputLayout;
            }
        }

        m_vbhBind      = _vbhOrInvalid;
        m_vbOffsetBind = _vertOffset;
        m_dirtyFlags |= ePipelineDirtyFlags::VertexBuffer;
    }
    m_numVertsBind = _numVerts;
}

void Graphics::SetIndexBuffer(
    const IndexBufferHandle _ibhOrInvalid)
{
    if (_ibhOrInvalid)
    {
        const IndexBuffer& ib = m_indexBuffers[_ibhOrInvalid];
        SetIndexBuffer(_ibhOrInvalid, ib.numIdxes, 0);
    }
    else
    {
        SetIndexBuffer(_ibhOrInvalid, 0, 0);
    }
}

void Graphics::SetIndexBuffer(
    const IndexBufferHandle _ibhOrInvalid,
    const Int32             _numIdxes,
    const Int32             _idxOffset)
{
    if (m_ibhBind != _ibhOrInvalid || m_ibOffsetBind != _idxOffset)
    {
        m_ibhBind      = _ibhOrInvalid;
        m_ibOffsetBind = _idxOffset;
        m_dirtyFlags |= ePipelineDirtyFlags::IndexBuffer;
    }
    m_numIndicesBind = _numIdxes;
}

void Graphics::SetConstantBuffer(
    const ConstantBufferHandle _cbh,
    const eShader              _shader,
    const Int32                _slot)
{
    constexpr EnumArray<eShader, ePipelineDirtyFlags> k_flags = {
        ePipelineDirtyFlags::VSConstantBuffer,
        ePipelineDirtyFlags::PSConstantBuffer,
        ePipelineDirtyFlags::GSConstantBuffer,
        ePipelineDirtyFlags::HSConstantBuffer,
        ePipelineDirtyFlags::DSConstantBuffer,
        ePipelineDirtyFlags::CSConstantBuffer
    };

    ConstantBufferBinds& binds = m_cbufferBinds[_shader];

    if (binds.cbhs[_slot] != _cbh)
    {
        binds.cbhs[_slot] = _cbh;
        binds.dirtyStart  = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd    = Max(binds.dirtyEnd, _slot);
        m_dirtyFlags |= k_flags[_shader];   // set flags
    }
}

void Graphics::SetTexture(
    const TextureHandle _texh,
    const eShader       _shader,
    const Int32         _slot)
{
    constexpr EnumArray<eShader, ePipelineDirtyFlags> k_flags = {
        ePipelineDirtyFlags::VSShaderResourceView,
        ePipelineDirtyFlags::PSShaderResourceView,
        ePipelineDirtyFlags::GSShaderResourceView,
        ePipelineDirtyFlags::HSShaderResourceView,
        ePipelineDirtyFlags::DSShaderResourceView,
        ePipelineDirtyFlags::CSShaderResourceView
    };

    TextureBinds& binds = m_textureBinds[_shader];

    if (binds.texhs[_slot] != _texh)
    {
        binds.texhs[_slot] = _texh;
        binds.dirtyStart   = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd     = Max(binds.dirtyEnd, _slot);
        m_dirtyFlags |= k_flags[_shader];   // set flags
    }
}

void Graphics::SetSampler(
    const SamplerState _state,
    const eShader      _shader,
    const Int32        _slot)
{
    constexpr EnumArray<eShader, ePipelineDirtyFlags> k_flags = {
        ePipelineDirtyFlags::VSSamplerState,
        ePipelineDirtyFlags::PSSamplerState,
        ePipelineDirtyFlags::GSSamplerState,
        ePipelineDirtyFlags::HSSamplerState,
        ePipelineDirtyFlags::DSSamplerState,
        ePipelineDirtyFlags::CSSamplerState
    };

    SamplerBinds& binds = m_samplerBinds[_shader];

    if (!MemEqual(binds.samplers[_slot], _state))
    {
        binds.samplers[_slot] = _state;
        binds.dirtyStart      = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd        = Max(binds.dirtyEnd, _slot);
        m_dirtyFlags |= k_flags[_shader];   // set flags
    }
}

void Graphics::SetBorderColor(
    const eShader _shader,
    const Int32   _slot,
    const RGBA    _rgba)
{
    constexpr EnumArray<eShader, ePipelineDirtyFlags> k_flags = {
        ePipelineDirtyFlags::VSSamplerState,
        ePipelineDirtyFlags::PSSamplerState,
        ePipelineDirtyFlags::GSSamplerState,
        ePipelineDirtyFlags::HSSamplerState,
        ePipelineDirtyFlags::DSSamplerState,
        ePipelineDirtyFlags::CSSamplerState
    };

    SamplerBinds& binds = m_samplerBinds[_shader];
    if (binds.borderColors[_slot].value != _rgba.value)
    {
        binds.borderColors[_slot] = _rgba;
        binds.dirtyStart          = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd            = Max(binds.dirtyEnd, _slot);
        m_dirtyFlags |= k_flags[_shader];   // set flags
    }
}

void Graphics::SetRenderState(
    const RenderState _state)
{
    if (!MemEqual(m_renderStateBind, _state))
    {
        m_renderStateBind = _state;

        // depth stencil state
        DepthStencilStateD3D11 dss = m_depthStencilStateBind;
        dss.bDepthWrite            = _state.bDepthWrite;
        dss.depthTestFunc          = _state.depthTestFunc;
        if (!MemEqual(m_depthStencilStateBind, dss))
        {
            m_depthStencilStateBind = dss;
            m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
        }

        // rasterizer state
        RasterizerStateD3D11 rs = {};
        rs.cullMode             = _state.cullMode;
        rs.bWireframe           = _state.bWireframe;
        rs.bDepthClamp          = _state.bDepthClamp;
        rs.bMultiSample         = _state.bMultiSample;
        rs.bLineAA              = _state.bLineAA;
        rs.bScissor             = _state.bScissor;
        rs.bCCWFront            = _state.bCCWFront;
        if (!MemEqual(m_rasterizerStateBind, rs))
        {
            m_rasterizerStateBind = rs;
            m_dirtyFlags |= ePipelineDirtyFlags::RasterizerState;
        }

        // blend state
        BlendStateD3D11 bs   = m_blendStateBind;
        bs.bAlphaToCoverage  = _state.bAlphaToCoverage;
        bs.bIndependentBlend = _state.bIndependentBlend;
        if (!MemEqual(m_blendStateBind, bs))
        {
            m_blendStateBind = bs;
            m_dirtyFlags |= ePipelineDirtyFlags::BlendState;
        }

        // primitive
        if (m_primBind != _state.primitive)
        {
            m_primBind = _state.primitive;
            m_dirtyFlags |= ePipelineDirtyFlags::Primitive;
        }
    }
}

void Graphics::SetBlendState(
    const BlendState _state,
    const Int32      _renderTargetIndex)
{
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < k_maxNumRenderTargetViewSlots, "Render target pos out of range.");
    const BlendState bs = m_blendStateBind.blends[_renderTargetIndex];
    if (!MemEqual(bs, _state))
    {
        m_blendStateBind.blends[_renderTargetIndex] = _state;
        m_dirtyFlags |= ePipelineDirtyFlags::BlendState;
    }
}

void Graphics::SetBlendFactor(
    const RGBA _rgba)
{
    if (m_blendFactorBind.value != _rgba.value)
    {
        m_blendFactorBind = _rgba;
        m_dirtyFlags |= ePipelineDirtyFlags::BlendState;
    }
}

void Graphics::SetStencil(
    const StencilState _fStencil,
    const StencilState _bStencil,
    const UInt8        _stencilRef)
{
    if (!MemEqual(m_depthStencilStateBind.fStencil, _fStencil))
    {
        m_depthStencilStateBind.fStencil = _fStencil;
        m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
    }

    if (!MemEqual(m_depthStencilStateBind.bStencil, _bStencil))
    {
        m_depthStencilStateBind.bStencil = _bStencil;
        m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
    }

    if (m_stencilRefBind != _stencilRef)
    {
        m_stencilRefBind = _stencilRef;
        m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
    }
}

void Graphics::SetProgram(
    const ProgramHandle _ph)
{
    if (m_phBind != _ph)
    {
        m_phBind = _ph;
        m_dirtyFlags |= ePipelineDirtyFlags::Program;
    }
}

void Graphics::SetFrameBuffer(
    const FrameBufferHandle _fbh)
{
    const FrameBufferHandle fbh = _fbh ? _fbh : m_backBufferFbh;
    if (m_fbhBind != fbh)
    {
        m_fbhBind = fbh;
        m_dirtyFlags |= ePipelineDirtyFlags::FrameBuffer;
    }
}

void Graphics::SetViewport(
    const Viewport _vp)
{
    if (!MemEqual(m_vpBind, _vp))
    {
        m_vpBind = _vp;
        m_dirtyFlags |= ePipelineDirtyFlags::Viewport;
    }
}

void Graphics::SetViewport(
    const UInt16 _x,
    const UInt16 _y,
    const UInt16 _width,
    const UInt16 _height)
{
    SetViewport(Viewport { _x, _y, _width, _height });
}

void Graphics::SetScissorRect(
    const Rect _rect)
{
    if (!MemEqual(m_scissorRectBind, _rect))
    {
        m_scissorRectBind = _rect;
        m_dirtyFlags |= ePipelineDirtyFlags::ScissorRect;
    }
}

void Graphics::SetScissorRect(
    const UInt16 _left,
    const UInt16 _top,
    const UInt16 _right,
    const UInt16 _bottom)
{
    SetScissorRect(Rect { _left, _top, _right, _bottom });
}

void Graphics::SetSubmitParameter(
    const eSubmitParameterType _type,
    const Int32                _value)
{
    m_submitParams[_type] = _value;
}

void Graphics::Flush()
{
    BindPipeline_();
}

ShaderHandle Graphics::GetShader(
    const ProgramHandle _ph,
    const eShader       _stage)
{
    const Program&     program = m_programs[_ph];
    const ShaderHandle sh      = program.shs[UnderlyingFrom(_stage)];
    JAM_ASSERT(sh, "Program does not have the specified shader stage.");
    return sh;
}

Vec2i Graphics::GetBackBufferResolution() const
{
    return m_backBufferRes;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const TextureHandle _texh,
    const bool          _bDestroyTexture)
{
    const TextureHandle attachments[1] = { _texh };
    return CreateFrameBuffer(attachments, _bDestroyTexture);
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

    Flags<eTextureCreateFlags> rtvFlags = eTextureCreateFlags::RenderTarget;
    if (_writeOnlyRenderTarget)
    {
        rtvFlags |= eTextureCreateFlags::WriteOnly;
    }

    Flags<eTextureCreateFlags> dsvFlags = eTextureCreateFlags::DepthStencil;
    if (_writeOnlyDepthStencil)
    {
        dsvFlags |= eTextureCreateFlags::WriteOnly;
    }

    const TextureHandle attachments[2] = {
        CreateTexture2D(_width, _height, _renderTargetFormat, 1, rtvFlags),
        CreateTexture2D(_width, _height, _depthFormat, 1, dsvFlags)
    };

    return CreateFrameBuffer(attachments, true);
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const Int32          _width,
    const Int32          _height,
    const eTextureFormat _format,
    const bool           _bWriteOnly)
{
    Flags<eTextureCreateFlags> flags = {};
    if (_bWriteOnly)
    {
        flags |= eTextureCreateFlags::WriteOnly;
    }

    if (IsDepthFormat(_format))
    {
        flags |= eTextureCreateFlags::DepthStencil;
    }
    else
    {
        flags |= eTextureCreateFlags::RenderTarget;
    }

    const TextureHandle attachments[] = { CreateTexture2D(_width, _height, _format, 1, flags) };
    return CreateFrameBuffer(attachments, true);
}

TextureHandle Graphics::CreateTexture(
    const ImageContainer& _imageContainer,
    const bool            _bMutable)
{
    const DirectX::TexMetadata& metadata = _imageContainer.GetMetadata();

    Vector<MemoryView> initData = {};
    initData.reserve(metadata.mipLevels * metadata.arraySize);
    for (UInt32 arrayIndex = 0; arrayIndex < metadata.arraySize; ++arrayIndex)
    {
        for (UInt32 mipIndex = 0; mipIndex < metadata.mipLevels; ++mipIndex)
        {
            const DirectX::Image* img = _imageContainer.GetImage(mipIndex, arrayIndex, 0);
            initData.emplace_back(img->pixels, img->rowPitch * img->height);
        }
    }

    const Int32 w = NumericCast<Int32>(metadata.width);
    const Int32 h = NumericCast<Int32>(metadata.height);
    const Int32 d = NumericCast<Int32>(metadata.depth);

    const Flags<eTextureCreateFlags> flags = _bMutable ? eTextureCreateFlags::Mutable : eTextureCreateFlags::None;
    if (metadata.IsCubemap())
    {
        return CreateTextureCube(w, h, TextureFormatFrom(metadata.format), NumericCast<Int32>(metadata.arraySize / 6), flags, initData);
    }
    else if (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D)
    {
        return CreateTexture3D(w, h, d, TextureFormatFrom(metadata.format), flags, initData);
    }
    else if (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D)
    {
        return CreateTexture2D(w, h, TextureFormatFrom(metadata.format), NumericCast<Int32>(metadata.arraySize), flags, initData);
    }
    else
    {
        JAM_UNREACHABLE;
    }
}

void Graphics::Submit()
{
    BindPipeline_();

    if (m_ibhBind)
    {
        const Int32 numIndex    = m_numIndicesBind;
        const Int32 startIdxLoc = m_submitParams[eSubmitParameterType::StartIndexLocation];
        const Int32 baseVtxLoc  = m_submitParams[eSubmitParameterType::BaseVertexLocation];
        m_pd3d11DeviceContext->DrawIndexed(numIndex, startIdxLoc, baseVtxLoc);
    }
    else
    {
        const Int32 numVerts     = m_numVertsBind;
        const Int32 startVertLoc = m_submitParams[eSubmitParameterType::StartVertexLocation];
        m_pd3d11DeviceContext->Draw(numVerts, startVertLoc);
    }
}

Graphics::Graphics(
    const HWND  _hWnd,
    const Int32 _width,
    const Int32 _height,
    const bool  _bAllowTearing)
    : m_bAllowTearing(_bAllowTearing)
{
    // d3d11 device 초기화
    {
        constexpr D3D_FEATURE_LEVEL k_featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };

        Int32 flags = 0;
#ifdef JAM_DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        JAM_CHECK_HRESULT(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            k_featureLevels,
            std::size(k_featureLevels),
            D3D11_SDK_VERSION,
            &m_pd3d11Device,
            nullptr,
            &m_pd3d11DeviceContext));
    }

    // 스왑체인 생성
    {
        ComPtr<IDXGIDevice2> dxgiDevice = nullptr;
        JAM_CHECK_HRESULT(m_pd3d11Device->QueryInterface(IID_PPV_ARGS(dxgiDevice.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIAdapter1> dxgiAdapter = nullptr;
        JAM_CHECK_HRESULT(dxgiDevice->GetParent(IID_PPV_ARGS(dxgiAdapter.ReleaseAndGetAddressOf())));

        ComPtr<IDXGIFactory2> dxgiFactory = nullptr;
        JAM_CHECK_HRESULT(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width                 = _width;
        swapChainDesc.Height                = _height;
        swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo                = false;
        swapChainDesc.SampleDesc.Count      = 1;
        swapChainDesc.SampleDesc.Quality    = 0;
        swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount           = 2;
        swapChainDesc.Scaling               = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags                 = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (_bAllowTearing)
        {
            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        JAM_CHECK_HRESULT(dxgiFactory->CreateSwapChainForHwnd(m_pd3d11Device, _hWnd, &swapChainDesc, nullptr, nullptr, &m_pdxgiSwapchain));
        OnResize_(_width, _height);
    }
}

Graphics::~Graphics()
{
    // 내부 리소스 해제
    JAM_DESTROY_TEXTURE(m_backBufferTexh);
    JAM_DESTROY_FRAME_BUFFER(m_backBufferFbh);

    // d3d11 cache 해체
    for (ID3D11BlendState* pBlendState: m_d3d11BlendStateCache | std::views::values)
    {
        pBlendState->Release();
    }

    for (ID3D11DepthStencilState* pDepthStencilState: m_d3d11DepthStencilStateCache | std::views::values)
    {
        pDepthStencilState->Release();
    }

    for (ID3D11RasterizerState* pRasterizerState: m_d3d11RasterizerStateCache | std::views::values)
    {
        pRasterizerState->Release();
    }

    for (ID3D11SamplerState* pSamplerState: m_d3d11SamplerStateCache | std::views::values)
    {
        pSamplerState->Release();
    }

    // 내부 리소스 해제
    if (m_pUploadBufferCache)
    {
        if (m_pUploadBufferCache->Release() != 0)
        {
            JAM_ASSERT(0, "Upload buffer cache is still in use. All buffers from the cache must be released before destroying the graphics instance.");
        }
    }

    if (m_pReadbackBufferCache)
    {
        if (m_pReadbackBufferCache->Release() != 0)
        {
            JAM_ASSERT(0, "Readback buffer cache is still in use. All buffers from the cache must be released before destroying the graphics instance.");
        }
    }

    for (const ReadbackTextureCacheEntry& entry: m_readbackTextureCache | std::views::values)
    {
        if (entry.pResource->Release() != 0)
        {
            JAM_ASSERT(0, "Readback staging texture is still in use. All textures from the cache must be released before destroying the graphics instance.");
        }
    }

    // d3d11 device 해제
    m_pdxgiSwapchain->Release();
    m_pd3d11DeviceContext->Release();
    m_pd3d11Device->Release();

    // 릭 검사
    if (DetectLeak())
    {
        JAM_UNREACHABLE;
    }
}

void Graphics::CreateInstance(
    const HWND  _hwnd,
    const Int32 _width,
    const Int32 _height,
    const bool  _bAllowTearing)
{
    JAM_ASSERT(s_pInstance == nullptr, "Graphics instance already created");
    s_pInstance = new Graphics(_hwnd, _width, _height, _bAllowTearing);
}

void Graphics::DestroyInstance()
{
    delete s_pInstance;
    s_pInstance = nullptr;
}

void Graphics::BindConstantBuffers_(
    const eShader _stage)
{
    ID3D11Buffer*        binder[k_maxNumConstantBufferSlots];   // do not initialize for performance
    ConstantBufferBinds& binds = m_cbufferBinds[_stage];

    // dirty range
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
        binder[i]                      = m_cbuffers.IsValid(cbh) ? m_cbuffers[cbh].pBuffer : nullptr;
    }

    // bind
    switch (_stage)
    {
        case eShader::VertexShader:
            m_pd3d11DeviceContext->VSSetConstantBuffers(dirtyStart, numDirties, binder);
            break;
        case eShader::PixelShader:
            m_pd3d11DeviceContext->PSSetConstantBuffers(dirtyStart, numDirties, binder);
            break;
        case eShader::GeometryShader:
            m_pd3d11DeviceContext->GSSetConstantBuffers(dirtyStart, numDirties, binder);
            break;
        case eShader::ComputeShader:
            m_pd3d11DeviceContext->CSSetConstantBuffers(dirtyStart, numDirties, binder);
            break;
        case eShader::HullShader:
            m_pd3d11DeviceContext->HSSetConstantBuffers(dirtyStart, numDirties, binder);
            break;
        case eShader::DomainShader:
            m_pd3d11DeviceContext->DSSetConstantBuffers(dirtyStart, numDirties, binder);
            break;
        default:
            JAM_UNREACHABLE;
    }

    // reset dirty range
    binds.dirtyStart = k_maxNumConstantBufferSlots;
    binds.dirtyEnd   = 0;
}

void Graphics::BindShaderResourceViews_(
    const eShader _shader)
{
    ID3D11ShaderResourceView* binder[k_maxNumShaderResourceViewSlots];   // do not initialize for performance
    TextureBinds&             binds = m_textureBinds[_shader];

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
        binder[i]                = m_textures.IsValid(texh) ? m_textures[texh].pSrv : nullptr;
    }

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            m_pd3d11DeviceContext->VSSetShaderResources(dirtyStart, numDirtySlots, binder);
            break;
        case eShader::PixelShader:
            m_pd3d11DeviceContext->PSSetShaderResources(dirtyStart, numDirtySlots, binder);
            break;
        case eShader::GeometryShader:
            m_pd3d11DeviceContext->GSSetShaderResources(dirtyStart, numDirtySlots, binder);
            break;
        case eShader::ComputeShader:
            m_pd3d11DeviceContext->CSSetShaderResources(dirtyStart, numDirtySlots, binder);
            break;
        case eShader::HullShader:
            m_pd3d11DeviceContext->HSSetShaderResources(dirtyStart, numDirtySlots, binder);
            break;
        case eShader::DomainShader:
            m_pd3d11DeviceContext->DSSetShaderResources(dirtyStart, numDirtySlots, binder);
            break;
        default:
            JAM_UNREACHABLE;
    }

    // reset dirty range
    binds.dirtyStart = k_maxNumShaderResourceViewSlots;
    binds.dirtyEnd   = 0;
}

void Graphics::BindSamplerStates_(
    const eShader _shader)
{
    ID3D11SamplerState* s_binder[k_maxNumSamplerSlots];
    SamplerBinds&       binds = m_samplerBinds[_shader];

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
        const RGBA         rgba    = binds.borderColors[dirtyStart + i];
        s_binder[i]                = GetOrCreateD3D11SamplerState_(sampler, rgba);
    }

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            m_pd3d11DeviceContext->VSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::PixelShader:
            m_pd3d11DeviceContext->PSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::GeometryShader:
            m_pd3d11DeviceContext->GSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::ComputeShader:
            m_pd3d11DeviceContext->CSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::HullShader:
            m_pd3d11DeviceContext->HSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        case eShader::DomainShader:
            m_pd3d11DeviceContext->DSSetSamplers(dirtyStart, numDirtySlots, s_binder);
            break;
        default:
            JAM_UNREACHABLE;
    }

    // reset dirty range
    binds.dirtyStart = k_maxNumSamplerSlots;
    binds.dirtyEnd   = 0;
}

void Graphics::BindPipeline_()
{
    // apply pipeline state if dirty
    if (m_dirtyFlags != ePipelineDirtyFlags::None)
    {
        // vertex buffer
        if (m_dirtyFlags & ePipelineDirtyFlags::VertexBuffer)
        {
            if (m_vbhBind)
            {
                const VertexBuffer& vb     = m_vertexBuffers[m_vbhBind];
                const UINT          stride = vb.stride;
                const UINT          offset = m_vbOffsetBind * stride;
                m_pd3d11DeviceContext->IASetVertexBuffers(0, 1, &vb.pBuffer, &stride, &offset);
            }
            else
            {
                constexpr ID3D11Buffer* k_pNullBuffer = nullptr;
                constexpr UINT          k_stride      = 0;
                constexpr UINT          k_offset      = 0;
                m_pd3d11DeviceContext->IASetVertexBuffers(0, 1, &k_pNullBuffer, &k_stride, &k_offset);
            }
        }

        // pos buffer
        if (m_dirtyFlags & ePipelineDirtyFlags::IndexBuffer)
        {
            if (m_ibhBind)
            {
                const IndexBuffer& ib     = m_indexBuffers[m_ibhBind];
                const DXGI_FORMAT  format = ib.bUseU32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
                m_pd3d11DeviceContext->IASetIndexBuffer(ib.pBuffer, format, m_ibOffsetBind);
            }
            else
            {
                m_pd3d11DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
            }
        }

        // topology
        if (m_dirtyFlags & ePipelineDirtyFlags::Primitive)
        {
            switch (m_primBind)
            {
                case ePrimitive::TriangleList:
                    m_pd3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    break;
                case ePrimitive::TriangleStrip:
                    m_pd3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                    break;
                case ePrimitive::LineList:
                    m_pd3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                    break;
                case ePrimitive::LineStrip:
                    m_pd3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
                    break;
                case ePrimitive::PointList:
                    m_pd3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                    break;
                default:
                    JAM_UNREACHABLE;
            }
        }

        // input vertexLayout
        if (m_dirtyFlags & ePipelineDirtyFlags::InputLayout)
        {
            if (m_vlhBind)
            {
                const VertexLayoutRef& vl = m_vertexLayouts[m_vlhBind];
                m_pd3d11DeviceContext->IASetInputLayout(vl.pInputLayout);
            }
            else
            {
                m_pd3d11DeviceContext->IASetInputLayout(nullptr);
            }
        }

        // shader program
        if (m_dirtyFlags & ePipelineDirtyFlags::Program)
        {
            if (m_phBind)
            {
                const Program& program = m_programs[m_phBind];
                for (eShader stage: k_gfxShaderStages)
                {
                    if (const ShaderHandle sh = program.shs[stage])
                    {
                        const ShaderRef& shader = m_shaders[sh];
                        switch (stage)
                        {
                            case eShader::VertexShader:
                                m_pd3d11DeviceContext->VSSetShader(shader.pVertexShader, nullptr, 0);
                                break;
                            case eShader::PixelShader:
                                m_pd3d11DeviceContext->PSSetShader(shader.pPixelShader, nullptr, 0);
                                break;
                            case eShader::GeometryShader:
                                m_pd3d11DeviceContext->GSSetShader(shader.pGeometryShader, nullptr, 0);
                                break;
                            case eShader::HullShader:
                                m_pd3d11DeviceContext->HSSetShader(shader.pHullShader, nullptr, 0);
                                break;
                            case eShader::DomainShader:
                                m_pd3d11DeviceContext->DSSetShader(shader.pDomainShader, nullptr, 0);
                                break;
                            default:
                                JAM_UNREACHABLE;
                        }
                    }
                    else
                    {
                        switch (stage)
                        {
                            case eShader::VertexShader:
                                m_pd3d11DeviceContext->VSSetShader(nullptr, nullptr, 0);
                                break;
                            case eShader::PixelShader:
                                m_pd3d11DeviceContext->PSSetShader(nullptr, nullptr, 0);
                                break;
                            case eShader::GeometryShader:
                                m_pd3d11DeviceContext->GSSetShader(nullptr, nullptr, 0);
                                break;
                            case eShader::HullShader:
                                m_pd3d11DeviceContext->HSSetShader(nullptr, nullptr, 0);
                                break;
                            case eShader::DomainShader:
                                m_pd3d11DeviceContext->DSSetShader(nullptr, nullptr, 0);
                                break;
                            default:
                                JAM_UNREACHABLE;
                        }
                    }
                }
            }
            else
            {
                m_pd3d11DeviceContext->VSSetShader(nullptr, nullptr, 0);
                m_pd3d11DeviceContext->PSSetShader(nullptr, nullptr, 0);
                m_pd3d11DeviceContext->GSSetShader(nullptr, nullptr, 0);
                m_pd3d11DeviceContext->HSSetShader(nullptr, nullptr, 0);
                m_pd3d11DeviceContext->DSSetShader(nullptr, nullptr, 0);
            }
        }

        // constant pBuffer
        if (m_dirtyFlags & ePipelineDirtyFlags::ConstantBufferMask)
        {
            if (m_dirtyFlags & ePipelineDirtyFlags::VSConstantBuffer)
            {
                BindConstantBuffers_(eShader::VertexShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::PSConstantBuffer)
            {
                BindConstantBuffers_(eShader::PixelShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::GSConstantBuffer)
            {
                BindConstantBuffers_(eShader::GeometryShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::HSConstantBuffer)
            {
                BindConstantBuffers_(eShader::HullShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::DSConstantBuffer)
            {
                BindConstantBuffers_(eShader::DomainShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::CSConstantBuffer)
            {
                BindConstantBuffers_(eShader::ComputeShader);
            }
        }

        // shader resource view
        if (m_dirtyFlags & ePipelineDirtyFlags::ShaderResourceViewMask)
        {
            if (m_dirtyFlags & ePipelineDirtyFlags::VSShaderResourceView)
            {
                BindShaderResourceViews_(eShader::VertexShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::PSShaderResourceView)
            {
                BindShaderResourceViews_(eShader::PixelShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::GSShaderResourceView)
            {
                BindShaderResourceViews_(eShader::GeometryShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::HSShaderResourceView)
            {
                BindShaderResourceViews_(eShader::HullShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::DSShaderResourceView)
            {
                BindShaderResourceViews_(eShader::DomainShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::CSShaderResourceView)
            {
                BindShaderResourceViews_(eShader::ComputeShader);
            }
        }

        // sampler state
        if (m_dirtyFlags & ePipelineDirtyFlags::SamplerStateMask)
        {
            if (m_dirtyFlags & ePipelineDirtyFlags::VSSamplerState)
            {
                BindSamplerStates_(eShader::VertexShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::PSSamplerState)
            {
                BindSamplerStates_(eShader::PixelShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::GSSamplerState)
            {
                BindSamplerStates_(eShader::GeometryShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::HSSamplerState)
            {
                BindSamplerStates_(eShader::HullShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::DSSamplerState)
            {
                BindSamplerStates_(eShader::DomainShader);
            }

            if (m_dirtyFlags & ePipelineDirtyFlags::CSSamplerState)
            {
                BindSamplerStates_(eShader::ComputeShader);
            }
        }

        // blend state
        if (m_dirtyFlags & ePipelineDirtyFlags::BlendState)
        {
            const FLOAT blendFactor[4] = {
                NumericCast<float>(m_blendFactorBind.r) / 255.f,
                NumericCast<float>(m_blendFactorBind.g) / 255.f,
                NumericCast<float>(m_blendFactorBind.b) / 255.f,
                NumericCast<float>(m_blendFactorBind.a) / 255.f
            };

            ID3D11BlendState* pBs = GetOrCreateD3D11BlendState_(m_blendStateBind);
            m_pd3d11DeviceContext->OMSetBlendState(pBs, blendFactor, 0xFFFFFFFF);
        }

        // depth stencil state
        if (m_dirtyFlags & ePipelineDirtyFlags::DepthStencilState)
        {
            ID3D11DepthStencilState* pBss = GetOrCreateD3D11DepthStencilState_(m_depthStencilStateBind);
            m_pd3d11DeviceContext->OMSetDepthStencilState(pBss, m_stencilRefBind);
        }

        // rasterizer state
        if (m_dirtyFlags & ePipelineDirtyFlags::RasterizerState)
        {
            ID3D11RasterizerState* pRs = GetOrCreateD3D11RasterizerState_(m_rasterizerStateBind);
            m_pd3d11DeviceContext->RSSetState(pRs);
        }

        // frame buffer
        if (m_dirtyFlags & ePipelineDirtyFlags::FrameBuffer)
        {
            const FrameBuffer& fb = m_frameBuffers[m_fbhBind];
            m_pd3d11DeviceContext->OMSetRenderTargets(fb.numRenderTargets, fb.pRtvs.data(), fb.pDsv);
        }

        // viewport
        if (m_dirtyFlags & ePipelineDirtyFlags::Viewport)
        {
            const D3D11_VIEWPORT vp = {
                NumericCast<float>(m_vpBind.x),
                NumericCast<float>(m_vpBind.y),
                NumericCast<float>(m_vpBind.w),
                NumericCast<float>(m_vpBind.h),
                0.f,
                1.f
            };
            m_pd3d11DeviceContext->RSSetViewports(1, &vp);
        }

        // scissor rect
        if (m_dirtyFlags & ePipelineDirtyFlags::ScissorRect)
        {
            const D3D11_RECT rect = {
                m_scissorRectBind.left,
                m_scissorRectBind.top,
                m_scissorRectBind.right,
                m_scissorRectBind.bottom
            };
            m_pd3d11DeviceContext->RSSetScissorRects(1, &rect);
        }

        // clear dirty flags
        m_dirtyFlags = ePipelineDirtyFlags::None;
    }
}

void Graphics::OnResize_(
    const Int32 _width,
    const Int32 _height)
{
    m_backBufferRes.x = _width;
    m_backBufferRes.y = _height;

    // resize back buffer
    if (m_pdxgiSwapchain)
    {
        // destroy back buffer dependencies
        JAM_DESTROY_TEXTURE(m_backBufferTexh);
        JAM_DESTROY_FRAME_BUFFER(m_backBufferFbh);

        if (JAM_DEBUG_ENABLED)
        {
            // check back buffer dependencies destroyed
            ID3D11Resource* pRes = nullptr;
            JAM_CHECK_HRESULT(m_pdxgiSwapchain->GetBuffer(0, IID_PPV_ARGS(&pRes)));
            const UINT refCount = pRes->Release();
            JAM_ASSERT(refCount == 0, "Back pBuffer reference numVerts is not zero after cleanup. RefCount: {}", refCount);
        }

        // resize
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        JAM_CHECK_HRESULT(m_pdxgiSwapchain->GetDesc1(&swapChainDesc));
        JAM_CHECK_HRESULT(m_pdxgiSwapchain->ResizeBuffers(0, _width, _height, DXGI_FORMAT_UNKNOWN, swapChainDesc.Flags));

        // create dependencies
        {
            // render target texture
            ID3D11Resource* pRes = nullptr;
            JAM_CHECK_HRESULT(m_pdxgiSwapchain->GetBuffer(0, IID_PPV_ARGS(&pRes)));

            DXGI_SWAP_CHAIN_DESC1 desc = {};
            JAM_CHECK_HRESULT(m_pdxgiSwapchain->GetDesc1(&desc));

            HardwareTextureRef texture = {};
            texture.width              = NumericCast<Int32>(desc.Width);
            texture.height             = NumericCast<Int32>(desc.Height);
            texture.depth              = 1;
            texture.type               = eTextureType::Texture2D;
            texture.format             = eTextureFormat::RGBA8_UNorm;
            texture.numMips            = 1;
            texture.arraySize          = 1;
            texture.samples            = 1;
            texture.bMutable           = true;
            texture.refCount           = 1;
            texture.bManaged           = false;
            texture.bRtvAttachable     = true;
            texture.bDsvAttachable     = false;
            texture.pResource          = pRes;

            // register to texture resource pool
            const TextureHandle texh = m_textures.Alloc();
            m_textures[texh]         = std::move(texture);
            Graphics::SetTextureName(texh, "BackBuffer");

            m_backBufferTexh = texh;
            m_backBufferFbh  = Graphics::CreateFrameBuffer(texh);
        }
    }
}

void Graphics::OnWindowResizedEvent_(
    const WindowResizedEvent& _event)
{
    OnResize_(_event.GetWidth(), _event.GetHeight());
}

void Graphics::OnWindowMaximizeEvent_(
    const WindowMaximizedEvent& _event)
{
    OnResize_(_event.GetWidth(), _event.GetHeight());
}

HardwareTextureRef Graphics::CreateHardwareTexture_(
    const Int32                      _width,
    const Int32                      _height,
    const Int32                      _depth,
    const eTextureFormat             _format,
    const Int32                      _arraySize,
    const Flags<eTextureCreateFlags> _flags,
    const eTextureType               _type,
    Span<const MemoryView>           _initDataOrEmpty) const
{
    const bool  bGenMips       = (_flags & eTextureCreateFlags::GenerateMips);
    const bool  bIsMSAA        = (_flags & eTextureCreateFlags::MultiSampled_Internal);
    const bool  bWriteOnly     = (_flags & eTextureCreateFlags::WriteOnly);
    const bool  bRtvAttachable = (_flags & eTextureCreateFlags::RenderTarget);
    const bool  bDsvAttachable = (_flags & eTextureCreateFlags::DepthStencil);
    const bool  bMutable       = _initDataOrEmpty.empty() || (_flags & eTextureCreateFlags::Mutable);
    const bool  bIsCubemap     = _type == eTextureType::TextureCube;
    const Int32 numMips        = bGenMips ? CalcNumMips(_width, _height, _depth) : 1;
    const Int32 samples        = (_flags & eTextureCreateFlags::MSAAx4)  ? 4
                               : (_flags & eTextureCreateFlags::MSAAx8)  ? 8
                               : (_flags & eTextureCreateFlags::MSAAx16) ? 16
                                                                         : 1;

    // validation
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "Texture dimensions must be greater than zero.");
    JAM_ASSERT(_arraySize > 0, "Texture array size must be greater than zero.");
    JAM_ASSERT(!(bIsMSAA && bGenMips), "Multisampled texture cannot have mipmaps.");

    // format
    const TextureFormatInfo formatInfo       = TextureFormatInfoFrom(_format);
    const bool              bWriteOnlyDsvTex = bWriteOnly && bDsvAttachable && !bRtvAttachable;
    const DXGI_FORMAT       dxgiFormat       = bWriteOnlyDsvTex ? formatInfo.depthFormat : formatInfo.texFormat;

    // native
    HardwareTextureRef texture = {};
    texture.width              = _width;
    texture.height             = _height;
    texture.depth              = _depth;
    texture.type               = _type;
    texture.format             = _format;
    texture.arraySize          = _arraySize;
    texture.samples            = samples;
    texture.numMips            = numMips;
    texture.bMutable           = bMutable;
    texture.refCount           = 1;
    texture.bManaged           = false;
    texture.bRtvAttachable     = bRtvAttachable;
    texture.bDsvAttachable     = bDsvAttachable;

    if (_type == eTextureType::Texture2D || _type == eTextureType::TextureCube)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width                = texture.width;
        desc.Height               = texture.height;
        desc.MipLevels            = texture.numMips;
        desc.ArraySize            = _arraySize;
        desc.Format               = dxgiFormat;
        desc.Usage                = bMutable ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
        desc.CPUAccessFlags       = 0;

        // MSAA 설정
        desc.SampleDesc.Count   = samples;
        desc.SampleDesc.Quality = bIsMSAA ? QueryMaxMsaaQualityLevel_(desc.Format, samples) : 0;

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

        if (_initDataOrEmpty.empty())
        {
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture2D(&desc, nullptr, &texture.pTexture2D));
        }
        else
        {
            JAM_ASSERT(_initDataOrEmpty.size() == _arraySize * numMips, "Texture data count mismatch");

            Vector<D3D11_SUBRESOURCE_DATA> initData = {};
            initData.reserve(_arraySize * numMips);
            for (Int32 arrayIdx = 0; arrayIdx < _arraySize; arrayIdx++)
            {
                for (Int32 mipIdx = 0; mipIdx < numMips; mipIdx++)
                {
                    const auto [width, height, _] = CalcTextureResolution(_width, _height, 1, mipIdx);
                    const Int32      texIndex     = CalcTextureIndex(mipIdx, arrayIdx, numMips);
                    const MemoryView mem          = _initDataOrEmpty[texIndex];
                    const UINT       rowPitch     = width * BytePerPixelOf(_format);
                    JAM_ASSERT(mem.GetByteWidth() == rowPitch * height, "Texture data size mismatch");
                    initData.emplace_back(mem.GetPtr(), rowPitch, 0);
                }
            }

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture2D(&desc, initData.data(), &texture.pTexture2D));
        }
    }
    else
    {
        JAM_ASSERT(!(_flags & eTextureCreateFlags::MultiSampled_Internal), "3D texture cannot be multisampled");
        JAM_ASSERT(_arraySize == 1, "3D texture array size must be 1");
        JAM_ASSERT(!bIsCubemap, "3D texture cannot be a cubemap");
        JAM_ASSERT(!bDsvAttachable, "3D texture cannot be a Depth/Stencil view.");

        D3D11_TEXTURE3D_DESC desc = {};
        desc.Width                = texture.width;
        desc.Height               = texture.height;
        desc.Depth                = texture.depth;
        desc.MipLevels            = texture.numMips;
        desc.Format               = dxgiFormat;
        desc.Usage                = bMutable ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
        desc.CPUAccessFlags       = 0;

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

        if (_initDataOrEmpty.empty())
        {
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture3D(&desc, nullptr, &texture.pTexture3D));
        }
        else
        {
            JAM_ASSERT(_initDataOrEmpty.size() == numMips, "Texture data count mismatch");

            Vector<D3D11_SUBRESOURCE_DATA> initData = {};
            initData.reserve(numMips);
            for (Int32 mipIdx = 0; mipIdx < numMips; mipIdx++)
            {
                const auto [width, height, depth] = CalcTextureResolution(_width, _height, _depth, mipIdx);
                const MemoryView mem              = _initDataOrEmpty[mipIdx];
                const UINT       rowPitch         = width * BytePerPixelOf(_format);
                const UINT       slicePitch       = rowPitch * height;
                JAM_ASSERT(mem.GetByteWidth() == slicePitch * depth, "Texture data size mismatch");
                initData.emplace_back(mem.GetPtr(), rowPitch, slicePitch);
            }

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture3D(&desc, initData.data(), &texture.pTexture3D));
        }
    }

    // SRV 생성s
    if (!bWriteOnly)
    {
        const bool bIsArray = _arraySize > 1;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = TextureFormatInfoFrom(_format).colorFormat;
        if (_type == eTextureType::Texture2D || _type == eTextureType::TextureCube)
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
        else
        {
            srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MostDetailedMip = 0;
            srvDesc.Texture3D.MipLevels       = numMips;
        }

        JAM_CHECK_HRESULT(m_pd3d11Device->CreateShaderResourceView(texture.pResource, &srvDesc, &texture.pSrv));
    }

    return texture;
}

HardwareBuffer Graphics::CreateHardwareBuffer_(
    const Int32       _byteWidth,
    const eBufferType _type,
    const MemoryView  _initDataOrEmpty) const
{
    JAM_ASSERT(_byteWidth > 0, "Buffer size must be greater than zero.");

    // validation
    const bool bDynamic = _initDataOrEmpty.IsEmpty() || _type == eBufferType::ConstantBuffer;
    if (JAM_DEBUG_ENABLED && _type == eBufferType::ConstantBuffer)
    {
        JAM_ASSERT(_byteWidth % 16 == 0, "Constant buffer size must be multiple of 16 bytes.");
        JAM_ASSERT(bDynamic, "Constant buffer must be dynamic.");
    }

    // native buffer
    HardwareBuffer buffer = {};
    buffer.byteWidth      = _byteWidth;
    buffer.bDynamic       = bDynamic;

    // d3d11 buffer
    D3D11_BUFFER_DESC desc   = {};
    desc.ByteWidth           = _byteWidth;
    desc.CPUAccessFlags      = bDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.StructureByteStride = 0;
    desc.Usage               = bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
    desc.BindFlags           = _type == eBufferType::VertexBuffer   ? D3D11_BIND_VERTEX_BUFFER
                             : _type == eBufferType::IndexBuffer    ? D3D11_BIND_INDEX_BUFFER
                             : _type == eBufferType::ConstantBuffer ? D3D11_BIND_CONSTANT_BUFFER
                                                                    : 0;

    desc.MiscFlags = 0;

    // create
    if (_initDataOrEmpty.IsEmpty())
    {
        JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, nullptr, &buffer.pBuffer));
    }
    else
    {
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = _initDataOrEmpty.GetPtr();
        initData.SysMemPitch            = 0;
        initData.SysMemSlicePitch       = 0;
        JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, &initData, &buffer.pBuffer));
    }

    return buffer;
}

FrameBuffer Graphics::CreateFrameBuffer_(
    Span<const TextureHandle> _attachments,
    const bool                _bDestoryTexture)
{
    JAM_ASSERT(!_attachments.empty(), "Framebuffer must have at least one attachment.");

    FrameBuffer fb               = {};
    bool        bHasDepth        = false;
    Int32       numRenderTargets = 0;

    for (const TextureHandle texh: _attachments)
    {
        JAM_ASSERT(texh, "Invalid texture handle in render target m_textures.");

        HardwareTextureRef& texture = m_textures[texh];
        JAM_ASSERT(texture.pResource, "Texture resource is null.");
        JAM_ASSERT(texture.bManaged == false, "Cannot attach a texture that is managed by a framebuffer.");

        // 렌더 타겟 / 뎁스 스텐실 객체 생성
        const bool bIsArray   = texture.arraySize > 1;
        const bool bIsMSAA    = texture.samples > 1;
        const bool bIsCubemap = texture.type == eTextureType::TextureCube;

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

        if (IsDepthFormat(texture.format))
        {
            JAM_ASSERT(bHasDepth == false, "Only one depth stencil texture can be attached to a framebuffer.");
            JAM_ASSERT(texture.bDsvAttachable, "Depth stencil texture must be created with Depth/Stencil flag.");
            JAM_ASSERT(bIsCubemap == false, "Cubemap depth stencil view is not supported.");
            JAM_ASSERT(texture.type != eTextureType::Texture3D, "3D texture depth stencil view is not supported.");

            D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format                        = TextureFormatInfoFrom(texture.format).depthFormat;

            if (bIsArray)
            {
                if (bIsMSAA)
                {
                    desc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    desc.Texture2DMSArray.FirstArraySlice = 0;
                    desc.Texture2DMSArray.ArraySize       = texture.arraySize;
                }
                else
                {
                    desc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.ArraySize       = texture.arraySize;
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

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateDepthStencilView(texture.pResource, &desc, &fb.pDsv));
            fb.depthStencilTexh = texh;
            bHasDepth           = true;
        }
        else   // 렌더 타겟
        {
            JAM_ASSERT(numRenderTargets < k_maxNumRenderTargetViewSlots, "Exceeded maximum render target count.");
            JAM_ASSERT(texture.bRtvAttachable, "Render target texture must be created with RenderTarget flag.");
            JAM_ASSERT(bIsCubemap == false, "Cubemap render target view is not supported.");

            D3D11_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format                        = TextureFormatInfoFrom(texture.format).texFormat;

            if (texture.type == eTextureType::Texture2D || texture.type == eTextureType::TextureCube)
            {

                if (bIsArray)
                {
                    if (bIsMSAA)
                    {
                        desc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                        desc.Texture2DMSArray.FirstArraySlice = 0;
                        desc.Texture2DMSArray.ArraySize       = texture.arraySize;
                    }
                    else
                    {
                        desc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                        desc.Texture2DArray.ArraySize       = texture.arraySize;
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
            else
            {
                desc.ViewDimension         = D3D11_RTV_DIMENSION_TEXTURE3D;
                desc.Texture3D.MipSlice    = 0;
                desc.Texture3D.FirstWSlice = 0;
                desc.Texture3D.WSize       = -1;
            }

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateRenderTargetView(texture.pResource, &desc, &fb.pRtvs[numRenderTargets]));
            fb.renderTargetTexhs[numRenderTargets] = texh;
            ++numRenderTargets;
        }
    }

    fb.numRenderTargets = numRenderTargets;
    return fb;
}

ShaderHandle Graphics::GetOrCreateShader_(
    const MemoryView _byteCode)
{
    Murmur3Hasher64 hasher {};
    hasher.Mix(_byteCode);
    const UInt64 key = hasher.Finish();

    const auto it = m_shaderCache.find(key);
    if (it != m_shaderCache.end())
    {
        const ShaderHandle sh = it->second;
        ++m_shaders[sh].refCount;
        return sh;
    }

    // 쉐이더 생성
    ShaderRef shader = {};
    shader.refCount  = 1;
    shader.cache     = key;

    // 리플렉션 생성
    const Byte*  pByteCode = _byteCode.GetPtr();
    const SIZE_T byteWidth = _byteCode.GetByteWidth();

    D3D11_SHADER_DESC              desc     = {};
    ComPtr<ID3D11ShaderReflection> pReflect = nullptr;
    JAM_CHECK_HRESULT(D3DReflect(pByteCode, byteWidth, IID_PPV_ARGS(pReflect.ReleaseAndGetAddressOf())));
    JAM_CHECK_HRESULT(pReflect->GetDesc(&desc));

    // 쉐이더 객체 생성
    const D3D11_SHADER_VERSION_TYPE stage = static_cast<D3D11_SHADER_VERSION_TYPE>(D3D11_SHVER_GET_TYPE(desc.Version));
    switch (stage)
    {
        case D3D11_SHVER_PIXEL_SHADER:
        {
            shader.stage = eShader::PixelShader;
            JAM_CHECK_HRESULT(m_pd3d11Device->CreatePixelShader(pByteCode, byteWidth, nullptr, &shader.pPixelShader));
        }
        break;

        case D3D11_SHVER_VERTEX_SHADER:
        {
            shader.stage = eShader::VertexShader;
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateVertexShader(pByteCode, byteWidth, nullptr, &shader.pVertexShader));
        }
        break;

        case D3D11_SHVER_GEOMETRY_SHADER:
        {
            shader.stage = eShader::GeometryShader;
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateGeometryShader(pByteCode, byteWidth, nullptr, &shader.pGeometryShader));
        }
        break;

        case D3D11_SHVER_HULL_SHADER:
        {
            shader.stage = eShader::HullShader;
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateHullShader(pByteCode, byteWidth, nullptr, &shader.pHullShader));
        }
        break;

        case D3D11_SHVER_DOMAIN_SHADER:
        {
            shader.stage = eShader::DomainShader;
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateDomainShader(pByteCode, byteWidth, nullptr, &shader.pDomainShader));
        }
        break;

        case D3D11_SHVER_COMPUTE_SHADER:
        {
            shader.stage = eShader::ComputeShader;
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateComputeShader(pByteCode, byteWidth, nullptr, &shader.pComputeShader));
        }
        break;

        default:
            JAM_UNREACHABLE;
    }

    const ShaderHandle sh = m_shaders.Alloc();
    m_shaders[sh]         = std::move(shader);
    m_shaderCache[key]    = sh;
    Graphics::SetShaderName(sh, "");
    return sh;
}

VertexLayoutHandle Graphics::GetOrCreateVertexLayout_(
    const VertexLayout& _vl)
{
    // calculate hash
    Murmur3Hasher64 hasher {};
    for (const VertexLayoutElement& vle: _vl.GetElems())
    {
        hasher.Mix(AsMemoryView(vle.attrib));
        hasher.Mix(AsMemoryView(vle.format));
    }
    const UInt64 hash = hasher.Finish();

    // check cache
    const auto it = m_vlCache.find(hash);
    if (it != m_vlCache.end())
    {
        const VertexLayoutHandle vlh = it->second;
        ++m_vertexLayouts[vlh].refCount;
        return vlh;
    }

    // generate dummy hlsl source
    String source = "struct VSInput {";

    // generate input element descs
    Vector<D3D11_INPUT_ELEMENT_DESC> inputElems = {};
    inputElems.reserve(_vl.GetNumElems());
    Int32 i = 0;
    for (const VertexLayoutElement& elem: _vl.GetElems())
    {
        const HLSLSemanticInfo& semanticInfo = HLSLSemanticInfoFrom_(elem.attrib);
        const HLSLDataTypeInfo& dataTypeInfo = HLSLDataTypeInfoFrom_(elem.format);

        // generate hlsl struct field
        const StringView  typeName   = dataTypeInfo.name;
        const DXGI_FORMAT dxgiFormat = dataTypeInfo.format;
        const StringView  semantic   = semanticInfo.name;
        const UINT        index      = semanticInfo.index;

        // append to hlsl source
        source.AppendFormat("{} variable_{} : {}{};", typeName, i++, semantic, index);

        // create input element desc
        inputElems.emplace_back(semantic.GetPtr(), index, dxgiFormat, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0);
    }

    // finalize hlsl source
    source += "}; void main(VSInput IN) { }";

    // compile dummy vertex shader to get bytecode signature
    ShaderCompilerInfo desc = {};
    desc.stage              = eShader::VertexShader;
    desc.entryPoint         = "main";
    desc.flags              = eShaderCompileFlags::ReleaseDefault;

    ShaderCompiler compiler = ShaderCompiler::CompileHLSLSource(source, desc);
    JAM_ASSERT(compiler.IsSucceeded(), "Failed to compile dummy vertex shader for input layout creation.");
    const MemoryView pBytecode = compiler.GetBytecode();

    // create d3d11 input layout
    ID3D11InputLayout* pd3d11InputLayout = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateInputLayout(
        inputElems.data(),
        NumericCast<UINT>(inputElems.size()),
        pBytecode.GetPtr(),
        pBytecode.GetByteWidth(),
        &pd3d11InputLayout));

    // allocate handle and initialize resource
    const VertexLayoutHandle vlh = m_vertexLayouts.Alloc();

    // store vertex layout
    VertexLayoutRef vl = {};
    vl.refCount        = 1;
    vl.vertexLayout    = _vl;
    vl.pInputLayout    = pd3d11InputLayout;
    vl.hash            = hash;

    // set debug name
    SetD3DDebugName_(pd3d11InputLayout, CreateName_("InputLayout", vlh, ""));

    // register to cache
    m_vertexLayouts[vlh] = std::move(vl);
    m_vlCache[hash]      = vlh;
    return vlh;
}

MutableMemoryView Graphics::BeginUploadBuffer_(
    const HardwareBuffer& _buffer) const
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(_buffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    return { mapped.pData, _buffer.byteWidth };
}

void Graphics::EndUploadBuffer_(
    const HardwareBuffer& _buffer) const
{
    m_pd3d11DeviceContext->Unmap(_buffer.pBuffer, 0);
}

void Graphics::UploadBuffer_(
    const HardwareBuffer& _buffer,
    const MemoryView      _uploadData,
    const Int32           _offset,
    const bool            _bDiscard)
{
    JAM_ASSERT(_buffer.bDynamic, "Only dynamic buffer can be updated");

    const Byte* pUploadData     = _uploadData.GetPtr();
    Int32       uploadDataWidth = NumericCast<Int32>(_uploadData.GetByteWidth());
    uploadDataWidth             = Min<Int32>(uploadDataWidth, _buffer.byteWidth - _offset);

    if (_bDiscard)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(_buffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
        std::memcpy(static_cast<Byte*>(mappedResource.pData) + _offset, pUploadData, uploadDataWidth);
        m_pd3d11DeviceContext->Unmap(_buffer.pBuffer, 0);
    }
    else
    {
        // 해당 코드는 ConstantBuffer에서 오류가 발생함
        // ConstantBuffer는 CopySubresourceRegion로 복사가 안됨.
        // https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-copysubresourceregion

        ID3D11Buffer* pUploadBuffer = GetOrCreateUploadBuffer_(uploadDataWidth);

        D3D11_MAPPED_SUBRESOURCE mapped;
        JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(pUploadBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
        std::memcpy(mapped.pData, pUploadData, uploadDataWidth);
        m_pd3d11DeviceContext->Unmap(pUploadBuffer, 0);

        D3D11_BOX box = {};
        box.left      = 0;
        box.right     = uploadDataWidth;
        box.top       = 0;
        box.bottom    = 1;
        box.front     = 0;
        box.back      = 1;

        m_pd3d11DeviceContext->CopySubresourceRegion(_buffer.pBuffer, 0, _offset, 0, 0, pUploadBuffer, 0, &box);
    }
}

Vector<Byte> Graphics::DownloadBuffer_(
    const HardwareBuffer& _buffer)
{
    Vector<Byte> readData(_buffer.byteWidth);

    // copy to staging buffer
    ID3D11Buffer* pBuffer = GetOrCreateReadbackBuffer_(_buffer.byteWidth);
    m_pd3d11DeviceContext->CopyResource(pBuffer, _buffer.pBuffer);

    // map staging buffer
    D3D11_MAPPED_SUBRESOURCE mapped;
    JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(pBuffer, 0, D3D11_MAP_READ, 0, &mapped));
    std::memcpy(readData.data(), mapped.pData, _buffer.byteWidth);
    m_pd3d11DeviceContext->Unmap(pBuffer, 0);

    return readData;
}

void Graphics::UploadTexture_(
    const HardwareTextureRef& _dstTexture,
    const Int32               _dstMipIndex,
    const Int32               _dstArrayIndex,
    const Int32               _dstX,
    const Int32               _dstY,
    const Int32               _dstZ,
    const MemoryView          _uploadData,
    const Int32               _uploadWidth,
    const Int32               _uploadHeight,
    const Int32               _uploadDepth,
    const Int32               _uploadDataRowPitch,
    const Int32               _uploadDataSlicePitch) const
{
    JAM_ASSERT(_dstTexture.pResource, "Destination texture resource is null!");

    const auto [w, h, d] = CalcTextureResolution(_dstTexture.width, _dstTexture.height, _dstTexture.depth, _dstMipIndex);

    D3D11_BOX dstBox = {};
    dstBox.left      = _dstX;
    dstBox.top       = _dstY;
    dstBox.front     = _dstZ;
    dstBox.right     = Min(_dstX + _uploadWidth, w);
    dstBox.bottom    = Min(_dstY + _uploadHeight, h);
    dstBox.back      = Min(_dstZ + _uploadDepth, d);

    const UINT dstIndex = CalcTextureIndex(_dstMipIndex, _dstArrayIndex, _dstTexture.numMips);
    m_pd3d11DeviceContext->UpdateSubresource(
        _dstTexture.pResource,
        dstIndex,
        &dstBox,
        _uploadData.GetPtr(),
        _uploadDataRowPitch,
        _uploadDataSlicePitch);
}

Vector<Byte> Graphics::DownloadTexture_(
    const HardwareTextureRef& _texture,
    const Int32               _mipIndex,
    const Int32               _arrayIndex)
{
    JAM_ASSERT(_mipIndex >= 0 && _arrayIndex >= 0, "Mip and array pos must be non-negative");
    JAM_ASSERT(_mipIndex < _texture.numMips, "Mip handle out of range");
    JAM_ASSERT(_arrayIndex < _texture.arraySize, "Array handle out of range");

    const auto [width, height, depth] = CalcTextureResolution(_texture.width, _texture.height, _texture.depth, _mipIndex);
    const Int32 rowPitch              = width * BytePerPixelOf(_texture.format);
    const Int32 slicePitch            = rowPitch * height;

    Vector<Byte> data(slicePitch * depth);
    if (_texture.type == eTextureType::TextureCube || _texture.type == eTextureType::Texture2D)
    {
        // copy to staging texture
        ID3D11Texture2D* pReadback = GetOrCreateReadbackTexture2D_(width, height, _texture.format);
        const UINT       index     = D3D11CalcSubresource(_mipIndex, _arrayIndex, _texture.numMips);
        m_pd3d11DeviceContext->CopySubresourceRegion(pReadback, 0, 0, 0, 0, _texture.pTexture2D, index, nullptr);

        // map staging texture
        D3D11_MAPPED_SUBRESOURCE mapped;
        JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(pReadback, 0, D3D11_MAP_READ, 0, &mapped));

        Byte*       pDstHead = data.data();
        const Byte* pSrcHead = static_cast<const Byte*>(mapped.pData);
        for (Int32 row = 0; row < height; ++row)
        {
            Byte*       pDst = pDstHead + row * rowPitch;
            const Byte* pSrc = pSrcHead + row * mapped.RowPitch;
            std::memcpy(pDst, pSrc, rowPitch);
        }

        m_pd3d11DeviceContext->Unmap(pReadback, 0);
    }
    else
    {
        ID3D11Texture3D* pReadback = GetOrCreateReadbackTexture3D_(width, height, depth, _texture.format);
        const UINT       index     = D3D11CalcSubresource(_mipIndex, 0, _texture.numMips);
        m_pd3d11DeviceContext->CopySubresourceRegion(pReadback, 0, 0, 0, 0, _texture.pTexture3D, index, nullptr);

        D3D11_MAPPED_SUBRESOURCE mapped;
        JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(pReadback, 0, D3D11_MAP_READ, 0, &mapped));

        Byte*       pDstHead = data.data();
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
        m_pd3d11DeviceContext->Unmap(pReadback, 0);
    }

    return data;
}

void Graphics::Blit_(
    const HardwareTextureRef& _blitDstTexture,
    const Int32               _blitDstMipIndex,
    const Int32               _blitDstArrayIndex,
    const Int32               _blitDstX,
    const Int32               _blitDstY,
    const Int32               _blitDstZ,
    const HardwareTextureRef& _blitSrcTexture,
    const Int32               _blitSrcMipIndex,
    const Int32               _blitSrcArrayIndex,
    const Int32               _blitSrcX,
    const Int32               _blitSrcY,
    const Int32               _blitSrcZ,
    const Int32               _blitSrcWidthOrZero,
    const Int32               _blitSrcHeightOrZero,
    const Int32               _blitSrcDepthOrZero) const
{
    const auto [srcWidth, srcHeight, srcDepth] = CalcTextureResolution(_blitSrcTexture.width, _blitSrcTexture.height, _blitSrcTexture.depth, _blitSrcMipIndex);
    const auto [dstWidth, dstHeight, dstDepth] = CalcTextureResolution(_blitDstTexture.width, _blitDstTexture.height, _blitDstTexture.depth, _blitDstMipIndex);

    Int32 blitWidth  = _blitSrcWidthOrZero > 0 ? _blitSrcWidthOrZero : srcWidth - _blitSrcX;
    Int32 blitHeight = _blitSrcHeightOrZero > 0 ? _blitSrcHeightOrZero : srcHeight - _blitSrcY;
    Int32 blitDepth  = _blitSrcDepthOrZero > 0 ? _blitSrcDepthOrZero : srcDepth - _blitSrcZ;
    blitWidth        = Min(blitWidth, srcWidth - _blitSrcX, dstWidth - _blitDstX);
    blitHeight       = Min(blitHeight, srcHeight - _blitSrcY, dstHeight - _blitDstY);
    blitDepth        = Min(blitDepth, srcDepth - _blitSrcZ, dstDepth - _blitDstZ);

    D3D11_BOX box = {};
    box.left      = _blitSrcX;
    box.top       = _blitSrcY;
    box.front     = _blitSrcZ;
    box.right     = _blitSrcX + blitWidth;
    box.bottom    = _blitSrcY + blitHeight;
    box.back      = _blitSrcZ + blitDepth;

    const UINT srcIndex = CalcTextureIndex(_blitSrcMipIndex, _blitSrcArrayIndex, _blitSrcTexture.numMips);
    const UINT dstIndex = CalcTextureIndex(_blitDstMipIndex, _blitDstArrayIndex, _blitDstTexture.numMips);

    m_pd3d11DeviceContext->CopySubresourceRegion(
        _blitDstTexture.pResource,
        dstIndex,
        _blitDstX,
        _blitDstY,
        _blitDstZ,
        _blitSrcTexture.pResource,
        srcIndex,
        &box);
}

ID3D11RasterizerState* Graphics::GetOrCreateD3D11RasterizerState_(
    const RasterizerStateD3D11 _state)
{
    Murmur3Hasher32 hasher {};
    hasher.Mix(AsMemoryView(_state));
    const UInt32 key = hasher.Finish();

    const auto it = m_d3d11RasterizerStateCache.find(key);
    if (it != m_d3d11RasterizerStateCache.end())
    {
        return it->second;
    }

    D3D11_RASTERIZER_DESC desc = {};
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
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateRasterizerState(&desc, &pRasterizerState));
    m_d3d11RasterizerStateCache[key] = pRasterizerState;
    return pRasterizerState;
}

ID3D11BlendState* Graphics::GetOrCreateD3D11BlendState_(
    const BlendStateD3D11& _state)
{
    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(_state));
    const UInt64 hash = hasher.Finish();

    const auto it = m_d3d11BlendStateCache.find(hash);
    if (it != m_d3d11BlendStateCache.end())
    {
        return it->second;
    }

    D3D11_BLEND_DESC desc       = {};
    desc.AlphaToCoverageEnable  = _state.bAlphaToCoverage;
    desc.IndependentBlendEnable = _state.bIndependentBlend;
    const Int32 numRts          = _state.bIndependentBlend ? k_maxNumRenderTargetViewSlots : 1;
    for (Int32 i = 0; i < numRts; ++i)
    {
        const BlendState&               rtBlend = _state.blends[i];
        D3D11_RENDER_TARGET_BLEND_DESC& rtDesc  = desc.RenderTarget[i];

        rtDesc.BlendEnable    = rtBlend.bEnable;
        rtDesc.SrcBlend       = D3D11BlendFrom_(rtBlend.srcBlend);
        rtDesc.DestBlend      = D3D11BlendFrom_(rtBlend.dstBlend);
        rtDesc.BlendOp        = D3D11BlendOpFrom_(rtBlend.blendOp);
        rtDesc.SrcBlendAlpha  = D3D11BlendFrom_(rtBlend.srcBlendAlpha);
        rtDesc.DestBlendAlpha = D3D11BlendFrom_(rtBlend.dstBlendAlpha);
        rtDesc.BlendOpAlpha   = D3D11BlendOpFrom_(rtBlend.blendOpAlpha);

        rtDesc.RenderTargetWriteMask = 0;
        if (rtBlend.writeMask & eWriteFlags::Red)
        {
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
        }

        if (rtBlend.writeMask & eWriteFlags::Green)
        {
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
        }

        if (rtBlend.writeMask & eWriteFlags::Blue)
        {
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
        }

        if (rtBlend.writeMask & eWriteFlags::Alpha)
        {
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
        }
    }

    ID3D11BlendState* pBlendState = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateBlendState(&desc, &pBlendState));
    m_d3d11BlendStateCache[hash] = pBlendState;
    return pBlendState;
}

ID3D11DepthStencilState* Graphics::GetOrCreateD3D11DepthStencilState_(
    const DepthStencilStateD3D11 _state)
{
    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(_state));
    const UInt64 key = hasher.Finish();

    const auto it = m_d3d11DepthStencilStateCache.find(key);
    if (it != m_d3d11DepthStencilStateCache.end())
    {
        return it->second;
    }

    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable              = _state.depthTestFunc != eCompareFunc::None;
    desc.DepthWriteMask           = _state.bDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc                = D3D11ComparisonFuncFrom_(_state.depthTestFunc);
    desc.StencilEnable            = _state.fStencil.compareFunc != eCompareFunc::None || _state.bStencil.compareFunc != eCompareFunc::None;
    desc.StencilReadMask          = 0xFF;
    desc.StencilWriteMask         = 0xFF;

    // front
    desc.FrontFace.StencilFailOp      = D3D11StencilOpFrom_(_state.fStencil.stencilFailOp);
    desc.FrontFace.StencilDepthFailOp = D3D11StencilOpFrom_(_state.fStencil.depthFailOp);
    desc.FrontFace.StencilPassOp      = D3D11StencilOpFrom_(_state.fStencil.passOp);
    desc.FrontFace.StencilFunc        = D3D11ComparisonFuncFrom_(_state.fStencil.compareFunc);

    // back
    desc.BackFace.StencilFailOp      = D3D11StencilOpFrom_(_state.bStencil.stencilFailOp);
    desc.BackFace.StencilDepthFailOp = D3D11StencilOpFrom_(_state.bStencil.depthFailOp);
    desc.BackFace.StencilPassOp      = D3D11StencilOpFrom_(_state.bStencil.passOp);
    desc.BackFace.StencilFunc        = D3D11ComparisonFuncFrom_(_state.bStencil.compareFunc);

    ID3D11DepthStencilState* pDepthStencilState;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateDepthStencilState(&desc, &pDepthStencilState));
    m_d3d11DepthStencilStateCache[key] = pDepthStencilState;
    return pDepthStencilState;
}

ID3D11SamplerState* Graphics::GetOrCreateD3D11SamplerState_(
    const SamplerState _state,
    const RGBA         _color)
{
    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(_state));
    hasher.Mix(AsMemoryView(_color));
    const UInt64 key = hasher.Finish();

    const auto it = m_d3d11SamplerStateCache.find(key);
    if (it != m_d3d11SamplerStateCache.end())
    {
        return it->second;
    }

    const SamplerInfo& filter          = SamplerInfoFrom_(_state.filter);
    const bool         bIsComparison   = _state.compareFunc != eCompareFunc::None;
    const Int32        anisotopicLevel = _state.filter == eTextureFilter::Anisotropic4  ? 4
                                       : _state.filter == eTextureFilter::Anisotropic8  ? 8
                                       : _state.filter == eTextureFilter::Anisotropic16 ? 16
                                                                                        : 1;

    D3D11_SAMPLER_DESC desc = {};
    desc.Filter             = bIsComparison ? filter.comparisonFilter : filter.filter;
    desc.AddressU           = D3D11TextureAddressModeFrom_(_state.addressU);
    desc.AddressV           = D3D11TextureAddressModeFrom_(_state.addressV);
    desc.AddressW           = D3D11TextureAddressModeFrom_(_state.addressW);
    desc.MipLODBias         = 0.0f;
    desc.MaxAnisotropy      = anisotopicLevel;
    desc.ComparisonFunc     = bIsComparison ? D3D11ComparisonFuncFrom_(_state.compareFunc) : D3D11_COMPARISON_ALWAYS;
    desc.BorderColor[0]     = NumericCast<float>(_color.r) / 255.0f;
    desc.BorderColor[1]     = NumericCast<float>(_color.g) / 255.0f;
    desc.BorderColor[2]     = NumericCast<float>(_color.b) / 255.0f;
    desc.BorderColor[3]     = NumericCast<float>(_color.a) / 255.0f;
    desc.MinLOD             = -Limits<float>::k_max;
    desc.MaxLOD             = Limits<float>::k_max;

    ID3D11SamplerState* pSamplerState = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateSamplerState(&desc, &pSamplerState));
    m_d3d11SamplerStateCache[key] = pSamplerState;
    return pSamplerState;
}

ID3D11Buffer* Graphics::GetOrCreateUploadBuffer_(
    const Int32 _byteWidth)
{
    JAM_ASSERT(_byteWidth > 0, "Buffer byte width must be positive");
    if (_byteWidth <= m_pUploadBufferCacheSize)
    {
        return m_pUploadBufferCache;
    }

    if (m_pUploadBufferCache)
    {
        if (m_pUploadBufferCache->Release() != 0)
        {
            JAM_ASSERT(0, "Upload buffer cache reference count is expected to be 1, but it's not. This indicates a potential resource leak or misuse of the upload buffer.");
        }
    }

    D3D11_BUFFER_DESC desc   = {};
    desc.Usage               = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth           = _byteWidth;
    desc.BindFlags           = 0;
    desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, nullptr, &m_pUploadBufferCache));
    m_pUploadBufferCacheSize = _byteWidth;
    return m_pUploadBufferCache;
}

ID3D11Buffer* Graphics::GetOrCreateReadbackBuffer_(
    const Int32 _byteWidth)
{
    JAM_ASSERT(_byteWidth > 0, "Buffer byte width must be positive");

    if (_byteWidth <= m_pReadbackBufferCacheSize)
    {
        return m_pReadbackBufferCache;
    }

    if (m_pReadbackBufferCache)
    {
        if (m_pReadbackBufferCache->Release() != 0)
        {
            JAM_ASSERT(0, "Readback buffer cache reference count is expected to be 1, but it's not. This indicates a potential resource leak or misuse of the readback buffer.");
        }
    }

    D3D11_BUFFER_DESC desc   = {};
    desc.Usage               = D3D11_USAGE_STAGING;
    desc.ByteWidth           = _byteWidth;
    desc.BindFlags           = 0;
    desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, nullptr, &m_pReadbackBufferCache));
    m_pReadbackBufferCacheSize = _byteWidth;
    return m_pReadbackBufferCache;
}

ID3D11Texture2D* Graphics::GetOrCreateReadbackTexture2D_(
    const Int32          _width,
    const Int32          _height,
    const eTextureFormat _format)
{
    JAM_ASSERT(_width > 0 && _height > 0, "Texture width and height must be positive");

    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(_format));
    hasher.Mix(AsMemoryView(eTextureType::Texture2D));
    const UInt64 key = hasher.Finish();
    auto         it  = m_readbackTextureCache.find(key);
    if (it != m_readbackTextureCache.end() && _width <= it->second.width && _height <= it->second.height)
    {
        return static_cast<ID3D11Texture2D*>(it->second.pResource);
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width                = _width;
    desc.Height               = _height;
    desc.MipLevels            = 1;
    desc.ArraySize            = 1;
    desc.Format               = TextureFormatInfoFrom(_format).texFormat;
    desc.SampleDesc.Count     = 1;
    desc.SampleDesc.Quality   = 0;
    desc.Usage                = D3D11_USAGE_STAGING;
    desc.BindFlags            = 0;
    desc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags            = 0;

    ID3D11Texture2D* pTexture = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture2D(&desc, nullptr, &pTexture));
    ReadbackTextureCacheEntry& entry = m_readbackTextureCache[key];
    if (entry.pResource)
    {
        if (entry.pResource->Release() != 0)
        {
            JAM_ASSERT(0, "Readback staging texture is still in use. All textures from the cache must be released before destroying the graphics instance.");
        }
    }

    entry.pResource = pTexture;
    entry.width     = _width;
    entry.height    = _height;
    entry.depth     = 1;
    return pTexture;
}

ID3D11Texture3D* Graphics::GetOrCreateReadbackTexture3D_(
    const Int32          _width,
    const Int32          _height,
    const Int32          _depth,
    const eTextureFormat _format)
{
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "Texture width, height and depth must be positive");

    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(_format));
    hasher.Mix(AsMemoryView(eTextureType::Texture3D));
    const UInt64 key = hasher.Finish();
    auto         it  = m_readbackTextureCache.find(key);
    if (it != m_readbackTextureCache.end() && _width <= it->second.width && _height <= it->second.height && _depth <= it->second.depth)
    {
        return static_cast<ID3D11Texture3D*>(it->second.pResource);
    }

    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width                = _width;
    desc.Height               = _height;
    desc.Depth                = _depth;
    desc.MipLevels            = 1;
    desc.Format               = TextureFormatInfoFrom(_format).texFormat;
    desc.Usage                = D3D11_USAGE_STAGING;
    desc.BindFlags            = 0;
    desc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags            = 0;

    ID3D11Texture3D* pTexture = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture3D(&desc, nullptr, &pTexture));
    ReadbackTextureCacheEntry& entry = m_readbackTextureCache[key];
    if (entry.pResource)
    {
        if (entry.pResource->Release() != 0)
        {
            JAM_ASSERT(0, "Readback staging texture is still in use. All textures from the cache must be released before destroying the graphics instance.");
        }
    }

    entry.pResource = pTexture;
    entry.width     = _width;
    entry.height    = _height;
    entry.depth     = _depth;
    return pTexture;
}

UInt32 Graphics::QueryMaxMsaaQualityLevel_(
    const DXGI_FORMAT _format,
    const Int32       _numSamples) const
{
    JAM_ASSERT(_numSamples > 0 && _numSamples <= 32, "Sample numVerts must be between 1 and 32");
    UINT numLevels = 0;
    JAM_CHECK_HRESULT(m_pd3d11Device->CheckMultisampleQualityLevels(_format, _numSamples, &numLevels));
    JAM_ASSERT(numLevels > 0, "The specified format does not support the given sample numVerts.");
    return numLevels - 1;
}

}   // namespace Jam