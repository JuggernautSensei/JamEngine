#include "pch.h"

#include "Graphics.h"
#include <d3d11shader.h>
#include <DirectXTex.h>
#include <d3dcompiler.h>

#include "BaseUtilities.h"
#include "Bit.h"
#include "Event.h"
#include "MemoryHasher.h"
#include "ResourcePool.h"
#include "WindowsPlatform.h"
#include "GeometryUtilities.h"
#include "Profile.h"
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
        // ReSharper disable once CppDeclaratorNeverUsed
        StringView  name   = {};
        DXGI_FORMAT format = {};
    };

    struct BufferDataTypeInfo
    {
        DXGI_FORMAT format = {};
        Int32       stride = {};
    };

    [[nodiscard]] D3D11_BLEND D3D11BlendFrom_(const UInt32 _blend)
    {
        switch (_blend)
        {
            case 0:
                return D3D11_BLEND_ZERO;
            case 1:
                return D3D11_BLEND_ONE;
            case 2:
                return D3D11_BLEND_SRC_COLOR;
            case 3:
                return D3D11_BLEND_INV_SRC_COLOR;
            case 4:
                return D3D11_BLEND_SRC_ALPHA;
            case 5:
                return D3D11_BLEND_INV_SRC_ALPHA;
            case 6:
                return D3D11_BLEND_DEST_ALPHA;
            case 7:
                return D3D11_BLEND_INV_DEST_ALPHA;
            case 8:
                return D3D11_BLEND_DEST_COLOR;
            case 9:
                return D3D11_BLEND_INV_DEST_COLOR;
            case 10:
                return D3D11_BLEND_SRC_ALPHA_SAT;
            case 11:
                return D3D11_BLEND_BLEND_FACTOR;
            case 12:
                return D3D11_BLEND_INV_BLEND_FACTOR;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] D3D11_BLEND_OP D3D11BlendOpFrom_(
        const UInt32 _op)
    {
        switch (_op)
        {
            case 0:
                return D3D11_BLEND_OP_ADD;
            case 1:
                return D3D11_BLEND_OP_SUBTRACT;
            case 2:
                return D3D11_BLEND_OP_REV_SUBTRACT;
            case 3:
                return D3D11_BLEND_OP_MIN;
            case 4:
                return D3D11_BLEND_OP_MAX;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] D3D11_COMPARISON_FUNC D3D11ComparisonFuncFrom_(
        const UInt32 _func)
    {
        switch (_func)
        {
            case 0:
                return D3D11_COMPARISON_LESS;   // None 일 경우 comparisonFunc은 의미 없음. 단 d3d11 에러를 피하기 위해서 placeholder
            case 1:
                return D3D11_COMPARISON_LESS;
            case 2:
                return D3D11_COMPARISON_LESS_EQUAL;
            case 3:
                return D3D11_COMPARISON_GREATER;
            case 4:
                return D3D11_COMPARISON_GREATER_EQUAL;
            case 5:
                return D3D11_COMPARISON_EQUAL;
            case 6:
                return D3D11_COMPARISON_NOT_EQUAL;
            case 7:
                return D3D11_COMPARISON_ALWAYS;
            case 8:
                return D3D11_COMPARISON_NEVER;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] D3D11_STENCIL_OP D3D11StencilOpFrom_(
        const UInt32 _op)
    {
        switch (_op)
        {
            case 0:
                return D3D11_STENCIL_OP_KEEP;
            case 1:
                return D3D11_STENCIL_OP_ZERO;
            case 2:
                return D3D11_STENCIL_OP_REPLACE;
            case 3:
                return D3D11_STENCIL_OP_INCR_SAT;
            case 4:
                return D3D11_STENCIL_OP_DECR_SAT;
            case 5:
                return D3D11_STENCIL_OP_INVERT;
            case 6:
                return D3D11_STENCIL_OP_INCR;
            case 7:
                return D3D11_STENCIL_OP_DECR;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] D3D11_TEXTURE_ADDRESS_MODE D3D11TextureAddressModeFrom_(
        const UInt32 _mode)
    {
        switch (_mode)
        {
            case 0:
                return D3D11_TEXTURE_ADDRESS_WRAP;
            case 1:
                return D3D11_TEXTURE_ADDRESS_MIRROR;
            case 2:
                return D3D11_TEXTURE_ADDRESS_CLAMP;
            case 3:
                return D3D11_TEXTURE_ADDRESS_BORDER;
            case 4:
                return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] D3D11_PRIMITIVE_TOPOLOGY D3D11PrimitiveTopologyFrom_(
        const UInt32 _state)
    {
        switch (_state)
        {
            case 0:
                return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case 1:
                return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            case 2:
                return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            case 3:
                return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
            case 4:
                return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] SamplerInfo SamplerInfoFrom_(
        const UInt32 _filter)
    {
        switch (_filter)
        {
            case 0:
                return { D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT };
            case 1:
                return { D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR };
            case 2:
                return { D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT };
            case 3:
                return { D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR };
            case 4:
                return { D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT };
            case 5:
                return { D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR };
            case 6:
                return { D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT };
            case 7:
                return { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR };
            case 8:
            case 9:
            case 10:
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
            case eVertexAttribute::BlendIndex:
                return { "BlendIndex", 0 };
            case eVertexAttribute::BlendWeight:
                return { "BlendWeight", 0 };
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

    [[nodiscard]] BufferDataTypeInfo BufferDataTypeInfoFrom_(
        const eStorageBufferData _dataType)
    {
        switch (_dataType)
        {
            case eStorageBufferData::UInt8x1:
                return { DXGI_FORMAT_R8_UINT, 1 };
            case eStorageBufferData::UInt8x2:
                return { DXGI_FORMAT_R8G8_UINT, 2 };
            case eStorageBufferData::UInt8x4:
                return { DXGI_FORMAT_R8G8B8A8_UINT, 4 };
            case eStorageBufferData::Int8x1:
                return { DXGI_FORMAT_R8_SINT, 1 };
            case eStorageBufferData::Int8x2:
                return { DXGI_FORMAT_R8G8_SINT, 2 };
            case eStorageBufferData::Int8x4:
                return { DXGI_FORMAT_R8G8B8A8_SINT, 4 };
            case eStorageBufferData::UInt16x1:
                return { DXGI_FORMAT_R16_UINT, 2 };
            case eStorageBufferData::UInt16x2:
                return { DXGI_FORMAT_R16G16_UINT, 4 };
            case eStorageBufferData::UInt16x4:
                return { DXGI_FORMAT_R16G16B16A16_UINT, 8 };
            case eStorageBufferData::Int16x1:
                return { DXGI_FORMAT_R16_SINT, 2 };
            case eStorageBufferData::Int16x2:
                return { DXGI_FORMAT_R16G16_SINT, 4 };
            case eStorageBufferData::Int16x4:
                return { DXGI_FORMAT_R16G16B16A16_SINT, 8 };
            case eStorageBufferData::Float16x1:
                return { DXGI_FORMAT_R16_FLOAT, 2 };
            case eStorageBufferData::Float16x2:
                return { DXGI_FORMAT_R16G16_FLOAT, 4 };
            case eStorageBufferData::Float16x4:
                return { DXGI_FORMAT_R16G16B16A16_FLOAT, 8 };
            case eStorageBufferData::UInt32x1:
                return { DXGI_FORMAT_R32_UINT, 4 };
            case eStorageBufferData::UInt32x2:
                return { DXGI_FORMAT_R32G32_UINT, 8 };
            case eStorageBufferData::UInt32x4:
                return { DXGI_FORMAT_R32G32B32A32_UINT, 16 };
            case eStorageBufferData::Int32x1:
                return { DXGI_FORMAT_R32_SINT, 4 };
            case eStorageBufferData::Int32x2:
                return { DXGI_FORMAT_R32G32_SINT, 8 };
            case eStorageBufferData::Int32x4:
                return { DXGI_FORMAT_R32G32B32A32_SINT, 16 };
            case eStorageBufferData::Float32x1:
                return { DXGI_FORMAT_R32_FLOAT, 4 };
            case eStorageBufferData::Float32x2:
                return { DXGI_FORMAT_R32G32_FLOAT, 8 };
            case eStorageBufferData::Float32x4:
                return { DXGI_FORMAT_R32G32B32A32_FLOAT, 16 };
            default:
                JAM_UNREACHABLE;
        }
    }

    [[nodiscard]] String CreateName_(
        const StringView _handleName,
        const UInt32     _handleIndex,
        const UInt32     _handleToken,
        const StringView _userName)
    {
        String result = {};
        result.Format("[{}] index: {}, token: {}", _handleName, _handleIndex, _handleToken);
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
        JAM_CHECK_HRESULT(_pDeviceChild->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UInt32>(_name.GetLength()), _name.GetPtr()));
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
        const UInt32 index = handle.GetIndex();
        const UInt32 token = handle.GetToken();
        JAM_LOG_WARN("VertexBuffer leak detected: Handle pos = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_indexBuffers)
    {
        const UInt32 index = handle.GetIndex();
        const UInt32 token = handle.GetToken();
        JAM_LOG_WARN("IndexBuffer leak detected: Handle pos = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_textures)
    {
        const UInt32 index = handle.GetIndex();
        const UInt32 token = handle.GetToken();
        JAM_LOG_WARN("Texture leak detected: Handle pos = {}, Token = {}, Name = '{}', RefCount = {}", index, token, value.name, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_frameBuffers)
    {
        const UInt32 index = handle.GetIndex();
        const UInt32 token = handle.GetToken();
        JAM_LOG_WARN("FrameBuffer leak detected: Handle pos = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_shaders)
    {
        constexpr EnumArray<eShader, StringView> kNames = {
            "VertexShader", "PixelShader", "GeometryShader", "HullShader", "DomainShader", "ComputeShader"
        };

        const UInt32     index = handle.GetIndex();
        const UInt32     token = handle.GetToken();
        const StringView name  = kNames[value.stage];
        JAM_LOG_WARN("Shader leak detected: Handle pos = {}, Token = {}, Stage = {}, RefCount = {}", index, token, name, value.refCount);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_cbuffers)
    {
        const UInt32 index = handle.GetIndex();
        const UInt32 token = handle.GetToken();
        JAM_LOG_WARN("ConstantBuffer leak detected: Handle pos = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    for (const ProgramHandle handle: m_programs | std::views::keys)
    {
        const UInt32 index = handle.GetIndex();
        const UInt32 token = handle.GetToken();
        JAM_LOG_WARN("Program leak detected: Handle pos = {}, Token = {}", index, token);
        bLeakDetected = true;
    }

    for (const auto& [handle, value]: m_storageBuffers)
    {
        const UInt32 index = handle.GetIndex();
        const UInt32 token = handle.GetToken();
        JAM_LOG_WARN("StorageBuffer leak detected: Handle pos = {}, Token = {}, Name = '{}'", index, token, value.name);
        bLeakDetected = true;
    }

    // vertex layout is not gpu resource. so do not check.
    return bLeakDetected;
}

void Graphics::OnEvent(Event& _event)
{
    EventDispatcher dispatcher { _event };
    dispatcher.Dispatch<WindowResizedEvent>(this, &Graphics::OnWindowResizedEvent_);
    dispatcher.Dispatch<WindowMaximizedEvent>(this, &Graphics::OnWindowMaximizeEvent_);
}

void Graphics::Frame(const bool _bVSync)
{
    const UInt32 interval = (m_bAllowTearing || !_bVSync) ? 0 : 1;
    const UInt32 flags    = m_bAllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0;
    if (FAILED(m_pdxgiSwapchain->Present(interval, flags)))
    {
        JAM_CHECK_HRESULT(m_pd3d11Device->GetDeviceRemovedReason());
    }

    m_fbhBinding = kInvalidHandle;   // frame buffer must rebind after present (this is api demand)

    // stats
    m_lastStats = m_stats;
    m_stats.Clear();
}

VertexBufferHandle Graphics::CreateVertexBuffer(
    const MemoryView    _vertData,
    const VertexLayout& _vl)
{
    const Int32          byteWidth = static_cast<Int32>(_vertData.GetByteWidth());
    const HardwareBuffer hwBuffer  = CreateHardwareBuffer_(byteWidth, eBuffer::VertexBuffer, _vertData);

    VertexBuffer vb = {};
    vb.pBuffer      = hwBuffer.pBuffer;
    vb.byteWidth    = hwBuffer.byteWidth;
    vb.bDynamic     = hwBuffer.bDynamic;
    vb.stride       = _vl.GetStride();
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
    const HardwareBuffer hwBuffer = CreateHardwareBuffer_(_numVerts * stride, eBuffer::VertexBuffer, {});

    VertexBuffer vb = {};
    vb.pBuffer      = hwBuffer.pBuffer;
    vb.byteWidth    = hwBuffer.byteWidth;
    vb.bDynamic     = hwBuffer.bDynamic;
    vb.stride       = stride;
    vb.vlh          = GetOrCreateVertexLayout_(_vl);

    const VertexBufferHandle vbh = m_vertexBuffers.Alloc();
    m_vertexBuffers[vbh]         = std::move(vb);
    SetVertexBufferName(vbh, "");
    return vbh;
}

VertexBufferHandle Graphics::CreateInstanceBuffer(
    const Int32 _numInstances,
    const Int32 _stride)
{
    JAM_ASSERT(_numInstances > 0, "Instance count must be greater than 0");

    const Int32 stride = AlignUp(_stride, 16);
    JAM_ASSERT(stride / 16 <= kMaxNumInstanceAttribs, "Instance stride must be less than {} bytes", kMaxNumInstanceAttribs * 16);

    const HardwareBuffer hwBuffer = CreateHardwareBuffer_(_numInstances * stride, eBuffer::VertexBuffer, {});

    VertexBuffer vb = {};
    vb.pBuffer      = hwBuffer.pBuffer;
    vb.byteWidth    = hwBuffer.byteWidth;
    vb.bDynamic     = hwBuffer.bDynamic;
    vb.stride       = stride;
    vb.vlh          = kInvalidHandle;   // instance buffer doesn't use vertex layout

    const VertexBufferHandle vbh = m_vertexBuffers.Alloc();
    m_vertexBuffers[vbh]         = std::move(vb);
    SetVertexBufferName(vbh, "");
    return vbh;
}

IndexBufferHandle Graphics::CreateIndexBuffer(
    const MemoryView _indexData,
    const bool       _bUseU32)
{
    const Int32          byteWidth = static_cast<Int32>(_indexData.GetByteWidth());
    const HardwareBuffer hwBuffer  = CreateHardwareBuffer_(byteWidth, eBuffer::IndexBuffer, _indexData);

    IndexBuffer ib = {};
    ib.pBuffer     = hwBuffer.pBuffer;
    ib.byteWidth   = hwBuffer.byteWidth;
    ib.bDynamic    = hwBuffer.bDynamic;
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
    const HardwareBuffer hwBuffer = CreateHardwareBuffer_(_numIndices * stride, eBuffer::IndexBuffer, {});

    IndexBuffer ib = {};
    ib.pBuffer     = hwBuffer.pBuffer;
    ib.byteWidth   = hwBuffer.byteWidth;
    ib.bDynamic    = hwBuffer.bDynamic;
    ib.bUseU32     = _bUseU32;

    const IndexBufferHandle ibh = m_indexBuffers.Alloc();
    m_indexBuffers[ibh]         = std::move(ib);
    SetIndexBufferName(ibh, "");
    return ibh;
}

ConstantBufferHandle Graphics::CreateConstantBuffer(
    const Int32 _byteWidth)
{
    JAM_ASSERT(_byteWidth > 0 && _byteWidth <= 0xFFFF, "Constant buffer size must be between 1 and 65536 bytes");

    const HardwareBuffer hwBuffer = CreateHardwareBuffer_(_byteWidth, eBuffer::ConstantBuffer, {});

    ConstantBuffer cb = {};
    cb.pBuffer        = hwBuffer.pBuffer;
    cb.byteWidth      = hwBuffer.byteWidth;
    cb.bDynamic       = hwBuffer.bDynamic;

    const ConstantBufferHandle cbh = m_cbuffers.Alloc();
    m_cbuffers[cbh]                = std::move(cb);
    SetConstantBufferName(cbh, "");
    return cbh;
}

StorageBufferHandle Graphics::CreateRawBuffer(
    const Int32                            _byteWidth,
    const Flags<eStorageBufferCreateFlags> _flags,
    const MemoryView                       _initDataOrEmpty)
{
    JAM_ASSERT(_byteWidth > 0, "Storage buffer size must be greater than 0");
    const StorageBufferHandle sbh = m_storageBuffers.Alloc();
    m_storageBuffers[sbh]         = CreateStorageBuffer_(_byteWidth, 0, eStorageBuffer::Raw, eStorageBufferData::UInt8x1, _flags, false, false, _initDataOrEmpty);
    SetStorageBufferName(sbh, "");
    return sbh;
}

StorageBufferHandle Graphics::CreateStructuredBuffer(
    const Int32                            _numElements,
    const Int32                            _stride,
    const bool                             _bAppendConsume,
    const bool                             _bCounter,
    const Flags<eStorageBufferCreateFlags> _flags,
    const MemoryView                       _initDataOrEmpty)
{
    JAM_ASSERT(_numElements > 0, "Storage buffer element count must be greater than 0");
    JAM_ASSERT(!(_bAppendConsume && _bCounter), "Storage buffer cannot be both append/consume and counter");
    const StorageBufferHandle sbh = m_storageBuffers.Alloc();
    m_storageBuffers[sbh]         = CreateStorageBuffer_(_numElements * _stride, _stride, eStorageBuffer::Structured, eStorageBufferData::UInt8x1, _flags, _bAppendConsume, _bCounter, _initDataOrEmpty);
    SetStorageBufferName(sbh, "");
    return sbh;
}

StorageBufferHandle Graphics::CreateTypedBuffer(
    const Int32                            _numElements,
    const eStorageBufferData               _dataType,
    const Flags<eStorageBufferCreateFlags> _flags,
    const MemoryView                       _initDataOrEmpty)
{
    JAM_ASSERT(_numElements > 0, "Storage buffer element count must be greater than 0");

    const BufferDataTypeInfo  dataTypeInfo = BufferDataTypeInfoFrom_(_dataType);
    const StorageBufferHandle sbh          = m_storageBuffers.Alloc();
    m_storageBuffers[sbh]                  = CreateStorageBuffer_(_numElements * dataTypeInfo.stride, dataTypeInfo.stride, eStorageBuffer::Typed, _dataType, _flags, false, false, _initDataOrEmpty);
    SetStorageBufferName(sbh, "");
    return sbh;
}

StorageBufferHandle Graphics::CreateReadbackBuffer(
    const Int32      _byteWidth,
    const MemoryView _initDataOrEmpty)
{
    JAM_ASSERT(_byteWidth > 0, "Storage buffer size must be greater than 0");
    const StorageBufferHandle sbh = m_storageBuffers.Alloc();
    m_storageBuffers[sbh]         = CreateStorageBuffer_(_byteWidth, 0, eStorageBuffer::Readback, eStorageBufferData::UInt8x1, eStorageBufferCreateFlags::None, false, false, _initDataOrEmpty);
    SetStorageBufferName(sbh, "");
    return sbh;
}

VertexLayoutHandle Graphics::CreateVertexLayout(
    const VertexLayout& _vl)
{
    return GetOrCreateVertexLayout_(_vl);
}

void Graphics::UpdateBuffer(
    const AnyBufferHandle _bh,
    const MemoryView      _writeData,
    const Int32           _offset,
    const bool            _bDiscard)
{
    UpdateBuffer_(_bh, _writeData, _offset, _bDiscard);
}

MutableMemoryView Graphics::BeginUpdateBuffer(
    const AnyBufferHandle _bh) const
{
    return BeginUpdateBuffer_(_bh);
}

void Graphics::EndUpdateBuffer(
    const AnyBufferHandle _bh) const
{
    EndUpdateBuffer_(_bh);
}

void Graphics::CopyBuffer(
    const AnyBufferHandle _dst,
    const Int32           _dstOffset,
    const AnyBufferHandle _src,
    const Int32           _srcOffset,
    const Int32           _byteWidthOrZero) const
{
    CopyBuffer_(_dst, _dstOffset, _src, _srcOffset, _byteWidthOrZero);
}

Int32 Graphics::ReadBuffer(
    const StorageBufferHandle _sbh,
    const MutableMemoryView   _dst)
{
    const StorageBuffer& sb = m_storageBuffers[_sbh];
    JAM_ASSERT(sb.bReadback, "Buffer must be created with readback flag to read data");
    D3D11_MAPPED_SUBRESOURCE mapped;
    JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(sb.pBuffer, 0, D3D11_MAP_READ, 0, &mapped));
    const Int32 byteWidth = Min(static_cast<Int32>(_dst.GetByteWidth()), sb.byteWidth);
    ::memcpy(_dst.GetPtr(), mapped.pData, byteWidth);
    m_pd3d11DeviceContext->Unmap(sb.pBuffer, 0);
    return byteWidth;
}

TextureHandle Graphics::CreateTexture(
    const ImageContainer& _container,
    const bool            _bMutable)
{
    const DirectX::TexMetadata&      metadata  = _container.GetMetadata();
    const Int32                      numImages = static_cast<Int32>(_container.GetImageCount());
    const Int32                      w         = static_cast<Int32>(metadata.width);
    const Int32                      h         = static_cast<Int32>(metadata.height);
    const Int32                      d         = static_cast<Int32>(metadata.depth);
    const Int32                      arrSize   = static_cast<Int32>(metadata.arraySize);
    const Int32                      numMips   = static_cast<Int32>(metadata.mipLevels);
    const bool                       bHasMips  = numMips > 1;
    const Flags<eTextureCreateFlags> flags     = _bMutable ? eTextureCreateFlags::Mutable : eTextureCreateFlags::None;
    const eTextureFormat             format    = TextureFormatFrom(metadata.format);

    if (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE2D)
    {
        MemoryView* initData = static_cast<MemoryView*>(JAM_STACK_ALLOC(sizeof(MemoryView) * numImages));
        for (Int32 arr = 0; arr < arrSize; ++arr)
        {
            for (Int32 mip = 0; mip < numMips; ++mip)
            {
                const Int32           index  = arr * numMips + mip;
                const DirectX::Image& pImage = _container.GetImages()[index];
                initData[index]              = { pImage.pixels, static_cast<SizeT>(pImage.rowPitch * pImage.height) };
            }
        }

        if (metadata.IsCubemap())
        {
            return CreateTextureCube(w, h, format, bHasMips, arrSize / 6, flags, { initData, numImages });
        }
        else
        {
            return CreateTexture2D(w, h, format, bHasMips, arrSize, flags, { initData, numImages });
        }
    }
    else if (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D)
    {
        MemoryView* initData = static_cast<MemoryView*>(JAM_STACK_ALLOC(sizeof(MemoryView) * numImages));
        for (Int32 mip = 0; mip < numMips; ++mip)
        {
            const DirectX::Image* pImage    = _container.GetImage(mip, 0, 0);
            const Int32           numSlices = Max(1, d >> mip);
            initData[mip]                   = { pImage->pixels, static_cast<SizeT>(pImage->slicePitch * numSlices) };
        }

        return CreateTexture3D(w, h, d, format, bHasMips, flags, { initData, numImages });
    }
    else
    {
        JAM_UNREACHABLE;
    }
}

TextureHandle Graphics::CreateTexture2D(
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const bool                       _bHasMips,
    const Int32                      _numLayers,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initDataOrEmpty)
{
    const TextureHandle texh = m_textures.Alloc();
    m_textures[texh]         = CreateHardwareTexture_(_width, _height, 1, _format, _bHasMips, _numLayers, eTexture::Texture2D, _flags, _initDataOrEmpty);
    SetTextureName(texh, "");
    return texh;
}

TextureHandle Graphics::CreateTextureCube(
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const bool                       _bHasMips,
    const Int32                      _cubeCount,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initDataOrEmpty)
{
    const TextureHandle texh = m_textures.Alloc();
    m_textures[texh]         = CreateHardwareTexture_(_width, _height, 1, _format, _bHasMips, _cubeCount * 6, eTexture::TextureCube, _flags, _initDataOrEmpty);
    SetTextureName(texh, "");
    return texh;
}

TextureHandle Graphics::CreateTexture3D(
    const Int32                      _width,
    const Int32                      _height,
    const Int32                      _depth,
    const eTextureFormat             _format,
    const bool                       _bHasMips,
    const Flags<eTextureCreateFlags> _texFlags,
    const Span<const MemoryView>     _initDataOrEmpty)
{
    const TextureHandle texh = m_textures.Alloc();
    m_textures[texh]         = CreateHardwareTexture_(_width, _height, _depth, _format, _bHasMips, 1, eTexture::Texture3D, _texFlags, _initDataOrEmpty);
    SetTextureName(texh, "");
    return texh;
}

void Graphics::UpdateTexture2D(
    const TextureHandle _texh,
    const Int32         _mipIndex,
    const Int32         _layerIndex,
    const Int32         _x,
    const Int32         _y,
    const Int32         _width,
    const Int32         _height,
    const MemoryView    _data,
    const Int32         _rowPitch) const
{
    UpdateTexture_(_texh, _mipIndex, _layerIndex, _x, _y, 0, _width, _height, 1, _data, _rowPitch, 0);
}

void Graphics::UpdateTextureCube(
    const TextureHandle _texh,
    const Int32         _mipIndex,
    const eCubeFace     _face,
    const Int32         _layerIndex,
    const Int32         _x,
    const Int32         _y,
    const Int32         _width,
    const Int32         _height,
    const MemoryView    _data,
    const Int32         _rowPitch) const
{
    UpdateTexture_(_texh, _mipIndex, UnderlyingFrom(_face) * _layerIndex, _x, _y, 0, _width, _height, 1, _data, _rowPitch, 0);
}

void Graphics::UpdateTexture3D(
    const TextureHandle _texh,
    const Int32         _mipIndex,
    const Int32         _x,
    const Int32         _y,
    const Int32         _z,
    const Int32         _width,
    const Int32         _height,
    const Int32         _depth,
    const MemoryView    _data,
    const Int32         _rowPitch,
    const Int32         _depthPitch) const
{
    UpdateTexture_(_texh, _mipIndex, 0, _x, _y, _z, _width, _height, _depth, _data, _rowPitch, _depthPitch);
}

void Graphics::CopyTexture(
    const TextureHandle _dstTexh,
    const Int32         _dstMipIndex,
    const Int32         _dstArrayIndex,
    const Int32         _dstX,
    const Int32         _dstY,
    const Int32         _dstZ,
    const TextureHandle _srcTexh,
    const Int32         _srcMipIndex,
    const Int32         _srcArrayIndex,
    const Int32         _srcX,
    const Int32         _srcY,
    const Int32         _srcZ,
    const Int32         _srcWidthOrZero,
    const Int32         _srcHeightOrZero,
    const Int32         _srcDepthOrZero) const
{
    CopyTexture_(
        _dstTexh,
        _dstMipIndex,
        _dstArrayIndex,
        _dstX,
        _dstY,
        _dstZ,
        _srcTexh,
        _srcMipIndex,
        _srcArrayIndex,
        _srcX,
        _srcY,
        _srcZ,
        _srcWidthOrZero,
        _srcHeightOrZero,
        _srcDepthOrZero);
}

Int32 Graphics::ReadTexture(
    const TextureHandle     _texh,
    const Int32             _srcMipIndex,
    const Int32             _srcArrayIndex,
    const MutableMemoryView _dst) const
{
    return ReadTexture_(_texh, _srcMipIndex, _srcArrayIndex, _dst);
}

ImageContainer Graphics::Capture(
    const TextureHandle _texh)
{
    const HardwareTextureRef& texture   = m_textures[_texh];
    ImageContainer            container = {};
    JAM_CHECK_HRESULT(DirectX::CaptureTexture(m_pd3d11Device, m_pd3d11DeviceContext, texture.pResource, container));
    return container;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const Span<const Attachment> _attachments,
    const bool                   _bDestroyTexture)
{
    const FrameBufferHandle fbh = m_frameBuffers.Alloc();
    m_frameBuffers[fbh]         = CreateFrameBuffer_(_attachments, _bDestroyTexture);
    SetFrameBufferName(fbh, "");
    return fbh;
}

FrameBufferHandle Graphics::CreateFrameBuffer(
    const TextureHandle _texh,
    const bool          _bDestroyTexture)
{
    Attachment attachment[1] = {};
    attachment[0].texh       = _texh;
    return CreateFrameBuffer(attachment, _bDestroyTexture);
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

    Flags<eTextureCreateFlags> rtvFlags = eTextureCreateFlags::RenderTargetAttachment;
    if (_writeOnlyRenderTarget)
    {
        rtvFlags |= eTextureCreateFlags::DisableShaderRead;
    }

    Flags<eTextureCreateFlags> dsvFlags = eTextureCreateFlags::DepthStencilAttachment;
    if (_writeOnlyDepthStencil)
    {
        dsvFlags |= eTextureCreateFlags::DisableShaderRead;
    }

    Attachment attachments[2] = {};
    attachments[0].texh       = CreateTexture2D(_width, _height, _renderTargetFormat, false, 1, rtvFlags);
    attachments[1].texh       = CreateTexture2D(_width, _height, _depthFormat, false, 1, dsvFlags);
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
        flags |= eTextureCreateFlags::DisableShaderRead;
    }

    if (IsDepthFormat(_format))
    {
        flags |= eTextureCreateFlags::DepthStencilAttachment;
    }
    else
    {
        flags |= eTextureCreateFlags::RenderTargetAttachment;
    }

    Attachment attachment[1] = {};
    attachment[0].texh       = CreateTexture2D(_width, _height, _format, false, 1, flags);
    return CreateFrameBuffer(attachment, true);
}

ShaderHandle Graphics::CreateShader(
    const MemoryView _shaderByteCode)
{
    return GetOrCreateShader_(_shaderByteCode);
}

ProgramHandle Graphics::CreateProgram(
    const ShaderHandle _vsh,
    const ShaderHandle _psh,
    const ShaderHandle _gsh,
    const ShaderHandle _hsh,
    const ShaderHandle _dsh,
    const bool         _bDestroyShader)
{
    Program program = {};

    if (_vsh)
    {
        ShaderRef& shader = m_shaders[_vsh];
        JAM_ASSERT(shader.stage == eShader::VertexShader, "Shader handle does not match vertex shader stage");
        if (_bDestroyShader == false)
        {
            ++shader.refCount;
        }
        program.vsh = _vsh;
    }

    if (_psh)
    {
        ShaderRef& shader = m_shaders[_psh];
        JAM_ASSERT(shader.stage == eShader::PixelShader, "Shader handle does not match pixel shader stage");
        if (_bDestroyShader == false)
        {
            ++shader.refCount;
        }
        program.psh = _psh;
    }

    if (_gsh)
    {
        ShaderRef& shader = m_shaders[_gsh];
        JAM_ASSERT(shader.stage == eShader::GeometryShader, "Shader handle does not match geometry shader stage");
        if (_bDestroyShader == false)
        {
            ++shader.refCount;
        }
        program.gsh = _gsh;
    }

    if (_hsh)
    {
        ShaderRef& shader = m_shaders[_hsh];
        JAM_ASSERT(shader.stage == eShader::HullShader, "Shader handle does not match hull shader stage");
        if (_bDestroyShader == false)
        {
            ++shader.refCount;
        }
        program.hsh = _hsh;
    }

    if (_dsh)
    {
        ShaderRef& shader = m_shaders[_dsh];
        JAM_ASSERT(shader.stage == eShader::DomainShader, "Shader handle does not match domain shader stage");
        if (_bDestroyShader == false)
        {
            ++shader.refCount;
        }
        program.dsh = _dsh;
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
        kInvalidHandle,
        kInvalidHandle,
        kInvalidHandle,
        _bDestroyShader);
}

ProgramHandle Graphics::CreateProgram(
    const ShaderHandle _csh,
    const bool         _bDestroyShader)
{
    JAM_ASSERT(_csh, "Compute shader must be provided");

    Program    program = {};
    ShaderRef& shader  = m_shaders[_csh];
    JAM_ASSERT(shader.stage == eShader::ComputeShader, "Shader handle does not match compute shader stage");

    // add ref
    if (_bDestroyShader == false)
    {
        ++shader.refCount;
    }

    program.csh = _csh;

    const ProgramHandle ph = m_programs.Alloc();
    m_programs[ph]         = std::move(program);
    return ph;
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

    m_vertexBuffers.Free(_vbh);
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

void Graphics::DestroyVertexLayout(
    const VertexLayoutHandle _vlh)
{
    const VertexLayoutRef& vl = m_vertexLayouts[_vlh];
    if (m_vlCache.erase(vl.hash) == 0)   // 캐시에서 제거
    {
        JAM_ASSERT(0, "Vertex layout hash not found in cache.");
    }
    m_vertexLayouts.Free(_vlh);   // VertexLayout 핸들 해제
}

void Graphics::DestroyConstantBuffer(
    const ConstantBufferHandle _cbh)
{
    const ConstantBuffer& cbuffer = m_cbuffers[_cbh];

    // d3d11 resource release
    const UINT refCount = cbuffer.pBuffer->Release();
    if (refCount > 0)
    {
        JAM_ASSERT(0, "Constant buffer reference count is not zero. RefCount = {}", refCount);
    }
    m_cbuffers.Free(_cbh);   // 핸들 해제
}

void Graphics::DestroyStorageBuffer(
    const StorageBufferHandle _sbh)
{
    const StorageBuffer& sbuffer = m_storageBuffers[_sbh];

    if (sbuffer.pSrv)
    {
        const UINT srvRefCount = sbuffer.pSrv->Release();
        if (srvRefCount > 0)
        {
            JAM_ASSERT(0, "Storage buffer SRV reference count is not zero. RefCount = {}", srvRefCount);
        }
    }

    if (sbuffer.pUav)
    {
        const UINT uavRefCount = sbuffer.pUav->Release();
        if (uavRefCount > 0)
        {
            JAM_ASSERT(0, "Storage buffer UAV reference count is not zero. RefCount = {}", uavRefCount);
        }
    }

    const UINT refCount = sbuffer.pBuffer->Release();
    if (refCount > 0)
    {
        JAM_ASSERT(0, "Storage buffer reference count is not zero. RefCount = {}", refCount);
    }

    m_storageBuffers.Free(_sbh);   // 핸들 해제
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

        if (texture.pUav)   // UAV 파괴
        {
            if (texture.pUav->Release() != 0)
            {
                JAM_ASSERT(0, "Unordered access view reference count is not zero.");
            }
        }

        if (texture.pResource)   // 텍스처 리소스 파괴
        {
            if (texture.pResource->Release() != 0)
            {
                JAM_ASSERT(0, "Texture resource reference count is not zero.");
            }
        }

        if (texture.pMsaaTexture2D)   // MSAA 텍스처 리소스 파괴
        {
            if (texture.pMsaaTexture2D->Release() != 0)
            {
                JAM_ASSERT(0, "MSAA texture resource reference count is not zero.");
            }
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

        DestroyTexture(fb.attachments[i].texh);   // TextureHandle 해제
    }

    // DSV 파괴
    if (fb.bHasDepthStencil)
    {
        ID3D11DepthStencilView* pDSV = fb.pDsv;
        if (pDSV->Release() != 0)
        {
            JAM_ASSERT(0, "Depth stencil view reference count is not zero");
        }

        DestroyTexture(fb.attachments[kDepthStencilAttachmentIndex].texh);   // TextureHandle 해제
    }

    m_frameBuffers.Free(_fbh);   // FrameBuffer 핸들 해제
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

        m_shaderCache.erase(shader.hash);
        m_shaders.Free(_sh);
    }
}

void Graphics::DestroyProgram(
    const ProgramHandle _ph)
{
    Program& program = m_programs[_ph];

    if (program.vsh)
    {
        DestroyShader(program.vsh);
    }

    if (program.psh)
    {
        DestroyShader(program.psh);
    }

    if (program.hsh)
    {
        DestroyShader(program.hsh);
    }

    if (program.dsh)
    {
        DestroyShader(program.dsh);
    }

    if (program.gsh)
    {
        DestroyShader(program.gsh);
    }

    if (program.csh)
    {
        DestroyShader(program.csh);
    }

    m_programs.Free(_ph);   // Program 핸들 해제
}

void Graphics::SetVertexBufferName(
    const VertexBufferHandle _vbh,
    const StringView         _name)
{
    VertexBuffer& vb = m_vertexBuffers[_vbh];
    vb.name          = CreateName_("VertexBuffer", _vbh.GetIndex(), _vbh.GetToken(), _name);
    SetD3DDebugName_(vb.pBuffer, vb.name);
}

void Graphics::SetIndexBufferName(
    const IndexBufferHandle _ibh,
    const StringView        _name)
{
    IndexBuffer& ib = m_indexBuffers[_ibh];
    ib.name         = CreateName_("IndexBuffer", _ibh.GetIndex(), _ibh.GetToken(), _name);
    SetD3DDebugName_(ib.pBuffer, ib.name);
}

void Graphics::SetConstantBufferName(
    const ConstantBufferHandle _cbh,
    const StringView           _name)
{
    ConstantBuffer& cb = m_cbuffers[_cbh];
    cb.name            = CreateName_("ConstantBuffer", _cbh.GetIndex(), _cbh.GetToken(), _name);
    SetD3DDebugName_(cb.pBuffer, cb.name);
}

void Graphics::SetStorageBufferName(
    const StorageBufferHandle _sbh,
    const StringView          _name)
{
    StorageBuffer& sb = m_storageBuffers[_sbh];
    sb.name           = CreateName_("StorageBuffer", _sbh.GetIndex(), _sbh.GetToken(), _name);
    SetD3DDebugName_(sb.pBuffer, sb.name);

    if (sb.pSrv)
    {
        SetD3DDebugName_(sb.pSrv, CreateName_("StorageBuffer_SRV", _sbh.GetIndex(), _sbh.GetToken(), _name));
    }

    if (sb.pUav)
    {
        SetD3DDebugName_(sb.pUav, CreateName_("StorageBuffer_UAV", _sbh.GetIndex(), _sbh.GetToken(), _name));
    }
}

void Graphics::SetTextureName(
    const TextureHandle _texh,
    const StringView    _name)
{
    constexpr EnumArray<eTexture, StringView> kTexNames = { "Texture2D", "Texture3D", "TextureCube" };
    constexpr EnumArray<eTexture, StringView> kSrvNames = { "Texture2D_SRV", "Texture3D_SRV", "TextureCube_SRV" };

    HardwareTextureRef& texture = m_textures[_texh];

    // 리소스 이름 설정
    texture.name = CreateName_(kTexNames[texture.type], _texh.GetIndex(), _texh.GetToken(), _name);
    SetD3DDebugName_(texture.pResource, texture.name);

    // SRV 이름 설정
    if (texture.pSrv)
    {
        SetD3DDebugName_(texture.pSrv, CreateName_(kSrvNames[texture.type], _texh.GetIndex(), _texh.GetToken(), _name));
    }
}

void Graphics::SetFrameBufferName(
    const FrameBufferHandle _fbh,
    const StringView        _name)
{
    JAM_ASSERT(_fbh, "Invalid FrameBuffer handle");

    constexpr StringView kNames[] = {
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
            SetD3DDebugName_(fb.pRtvs[i], CreateName_(kNames[i], _fbh.GetIndex(), _fbh.GetToken(), _name));
        }
    }

    if (fb.pDsv)
    {
        SetD3DDebugName_(fb.pDsv, CreateName_("FrameBuffer_DSV", _fbh.GetIndex(), _fbh.GetToken(), _name));
    }
}

void Graphics::SetShaderName(
    const ShaderHandle _sh,
    const StringView   _name)
{
    constexpr EnumArray<eShader, StringView> kNames = {
        "VertexShader",
        "PixelShader",
        "GeometryShader",
        "HullShader",
        "DomainShader",
        "ComputeShader"
    };

    ShaderRef& shader = m_shaders[_sh];
    shader.name       = CreateName_(kNames[shader.stage], _sh.GetIndex(), _sh.GetToken(), _name);

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

const VertexLayout& Graphics::GetVertexLayout(
    const VertexLayoutHandle _vlh)
{
    const VertexLayoutRef& vl = m_vertexLayouts[_vlh];
    return vl.vl;
}

const VertexLayout& Graphics::GetVertexLayout(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer&    vb = m_vertexBuffers[_vbh];
    const VertexLayoutRef& vl = m_vertexLayouts[vb.vlh];
    return vl.vl;
}

VertexBufferInfo Graphics::GetVertexBufferInfo(const VertexBufferHandle _vbh)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];

    VertexBufferInfo info = {};
    info.byteWidth        = vb.byteWidth;
    info.stride           = vb.stride;
    info.vlh              = vb.vlh;
    info.bImmutable       = vb.bImmutable;
    info.bDynamic         = vb.bDynamic;
    return info;
}

IndexBufferInfo Graphics::GetIndexBufferInfo(const IndexBufferHandle _ibh)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];

    IndexBufferInfo info = {};
    info.byteWidth       = ib.byteWidth;
    info.bImmutable      = ib.bImmutable;
    info.bDynamic        = ib.bDynamic;
    info.bUseU32         = ib.bUseU32;
    return info;
}

ConstantBufferInfo Graphics::GetConstantBufferInfo(const ConstantBufferHandle _cbh)
{
    const ConstantBuffer& cb = m_cbuffers[_cbh];

    ConstantBufferInfo info = {};
    info.byteWidth          = cb.byteWidth;
    info.bImmutable         = cb.bImmutable;
    info.bDynamic           = cb.bDynamic;
    return info;
}

StorageBufferInfo Graphics::GetStorageBufferInfo(const StorageBufferHandle _sbh)
{
    const StorageBuffer& sb = m_storageBuffers[_sbh];

    StorageBufferInfo info = {};
    info.byteWidth         = sb.byteWidth;
    info.stride            = sb.stride;
    info.numElems          = sb.numElems;
    info.storageType       = sb.storageType;
    info.bImmutable        = sb.bImmutable;
    info.bDynamic          = sb.bDynamic;
    info.bReadback         = sb.bReadback;
    info.bIndirectArgs     = sb.bIndirectArgs;
    info.bAppendConsume    = sb.bAppendConsume;
    info.bCounter          = sb.bCounter;
    return info;
}

TextureInfo Graphics::GetTextureInfo(const TextureHandle _texh)
{
    const HardwareTextureRef& texture = m_textures[_texh];

    TextureInfo info    = {};
    info.width          = texture.width;
    info.height         = texture.height;
    info.depth          = texture.depth;
    info.type           = texture.type;
    info.format         = texture.format;
    info.numLayers      = texture.numLayers;
    info.samples        = texture.samples;
    info.numMips        = texture.numMips;
    info.bImmutable     = texture.bImmutable;
    info.bReadback      = texture.bReadback;
    info.bRtvAttachable = texture.bRtvAttachable;
    info.bDsvAttachable = texture.bDsvAttachable;
    return info;
}

FrameBufferInfo Graphics::GetFrameBufferInfo(const FrameBufferHandle _fbh)
{
    const FrameBufferHandle fbh = _fbh ? _fbh : m_backBufferFbh;
    const FrameBuffer&      fb  = m_frameBuffers[fbh];

    FrameBufferInfo info  = {};
    info.numRenderTargets = fb.numRenderTargets;
    info.bHasDepthStencil = fb.bHasDepthStencil;
    info.bMsaa            = fb.bMsaa;
    info.attachments      = fb.attachments;
    return info;
}

Vec3i Graphics::GetFrameBufferResolution(
    const FrameBufferHandle _fbh)
{
    const FrameBufferHandle   fbh     = _fbh ? _fbh : m_backBufferFbh;
    const FrameBuffer&        fb      = m_frameBuffers[fbh];
    const TextureHandle       texh    = fb.bHasDepthStencil ? fb.attachments[kDepthStencilAttachmentIndex].texh : fb.attachments[0].texh;
    const HardwareTextureRef& texture = m_textures[texh];
    return { texture.width, texture.height, texture.depth };
}

Vec2i Graphics::GetBackBufferResolution() const
{
    return m_backBufferResolution;
}

ShaderInfo Graphics::GetShaderInfo(const ShaderHandle _sh)
{
    const ShaderRef& shader = m_shaders[_sh];

    ShaderInfo info = {};
    info.stage      = shader.stage;
    return info;
}

ProgramInfo Graphics::GetProgramInfo(const ProgramHandle _ph)
{
    const Program& program = m_programs[_ph];

    ProgramInfo info = {};
    info.vsh         = program.vsh;
    info.psh         = program.psh;
    info.gsh         = program.gsh;
    info.hsh         = program.hsh;
    info.dsh         = program.dsh;
    info.csh         = program.csh;
    return info;
}

const GraphicsStats& Graphics::GetStats() const
{
    return m_lastStats;
}

void Graphics::ClearRenderTarget(
    const FrameBufferHandle _fbh,
    const RGBA              _color,
    const Int32             _renderTargetIndex)
{
    const FrameBufferHandle fbh = _fbh ? _fbh : m_backBufferFbh;
    const FrameBuffer&      fb  = m_frameBuffers[fbh];
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < fb.numRenderTargets, "Render target m_index out of range.");
    const FLOAT colors[4] = {
        static_cast<float>(_color.r) / 255.0f,
        static_cast<float>(_color.g) / 255.0f,
        static_cast<float>(_color.b) / 255.0f,
        static_cast<float>(_color.a) / 255.0f
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
    {
        flags |= D3D11_CLEAR_DEPTH;
    }
    if (_bClearStencil)
    {
        flags |= D3D11_CLEAR_STENCIL;
    }
    m_pd3d11DeviceContext->ClearDepthStencilView(fb.pDsv, flags, _depth, _stencil);
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _vbhOrInvalid,
    const Int32              _vertOffset,
    const Int32              _numVerts)
{
    if (_vbhOrInvalid)
    {
        const VertexStream bindings[] = {
            { _vbhOrInvalid, _vertOffset }
        };

        SetVertexBuffers(bindings, _numVerts, kInvalidHandle, 0, 0);
    }
    else
    {
        SetVertexBuffers({}, 0, kInvalidHandle, 0, 0);
    }
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _vbhOrInvalid,
    const Int32              _vertOffset,
    const Int32              _numVerts,
    const VertexBufferHandle _instanceVbhOrInvalid,
    const Int32              _instanceOffset,
    const Int32              _numInstances)
{
    if (_vbhOrInvalid)
    {
        const VertexStream bindings[] = {
            { _vbhOrInvalid, _vertOffset }
        };

        SetVertexBuffers(bindings, _numVerts, _instanceVbhOrInvalid, _instanceOffset, _numInstances);
    }
    else
    {
        SetVertexBuffers({}, 0, _instanceVbhOrInvalid, _instanceOffset, _numInstances);
    }
}

void Graphics::SetVertexBuffers(
    const Span<const VertexStream> _streams,
    const Int32                    _numVerts)
{
    SetVertexBuffers(_streams, _numVerts, kInvalidHandle, 0, 0);
}

void Graphics::SetVertexBuffers(
    const Span<const VertexStream> _streams,
    const Int32                    _numVerts,
    const VertexBufferHandle       _instanceVbhOrInvalid,
    const Int32                    _instanceOffset,
    const Int32                    _numInstances)
{
    const Int32 numBuffers = static_cast<Int32>(_streams.size());
    if (m_vbBindings.numBuffers != numBuffers)
    {
        m_vbBindings.numBuffers = numBuffers;
        m_dirtyFlags |= ePipelineDirtyFlags::InputLayout;
    }

    for (Int32 i = 0; i < _streams.size(); ++i)
    {
        const VertexStream binding = _streams[i];
        if (m_vbBindings.vbhs[i] != binding.vbh || NotEqual(m_vbBindings.offsets[i], binding.offset))
        {
            JAM_ASSERT(binding.vbh, "Vertex buffer handle cannot be invalid when setting vertex buffers.");   // 항상 packed 된 스트림을 사용
            const VertexBuffer& vb  = m_vertexBuffers[binding.vbh];
            m_vbBindings.vbhs[i]    = binding.vbh;
            m_vbBindings.binder[i]  = vb.pBuffer;
            m_vbBindings.strides[i] = vb.stride;
            m_vbBindings.offsets[i] = _streams[i].offset * vb.stride;
            m_dirtyFlags |= ePipelineDirtyFlags::VertexBuffer;

            if (i == 0)   // 첫 번째 버퍼를 기준으로 가용 버텍스 수 계산
            {
                m_vbBindings.numAvailVerts = vb.byteWidth / vb.stride - _streams[i].offset;
            }

            if (m_vbBindings.vlhs[i] != vb.vlh)
            {
                m_vbBindings.vlhs[i] = vb.vlh;
                m_dirtyFlags |= ePipelineDirtyFlags::InputLayout;
            }
        }
    }
    m_vbBindings.numVerts = Min(_numVerts, m_vbBindings.numAvailVerts);

    // instance buffer
    if (_instanceVbhOrInvalid != m_instanceBinding.vbh || _instanceOffset != m_instanceBinding.offset)
    {
        const Int32 oldStride = m_instanceBinding.stride;
        if (_instanceVbhOrInvalid)
        {
            const VertexBuffer& vb              = m_vertexBuffers[_instanceVbhOrInvalid];
            m_instanceBinding.binder            = vb.pBuffer;
            m_instanceBinding.stride            = vb.stride;
            m_instanceBinding.offset            = _instanceOffset * vb.stride;
            m_instanceBinding.vbh               = _instanceVbhOrInvalid;
            m_instanceBinding.numAvailInstances = vb.byteWidth / vb.stride - _instanceOffset;
        }
        else
        {
            m_instanceBinding.binder            = nullptr;
            m_instanceBinding.stride            = 0;
            m_instanceBinding.offset            = 0;
            m_instanceBinding.vbh               = kInvalidHandle;
            m_instanceBinding.numAvailInstances = 0;
        }
        m_dirtyFlags |= ePipelineDirtyFlags::InstanceBuffer;

        // input layout은 numInstanceData 에 의존적임. numInstanceData = stride / 16.
        if (oldStride != m_instanceBinding.stride)
        {
            m_dirtyFlags |= ePipelineDirtyFlags::InputLayout;
        }
    }
    m_instanceBinding.numInstances = Min(_numInstances, m_instanceBinding.numAvailInstances);
}

void Graphics::SetIndexBuffer(
    const IndexBufferHandle _ibhOrInvalid,
    const Int32             _indexOffset,
    const Int32             _numIndices)
{
    if (_ibhOrInvalid != m_ibBinding.ibh || _indexOffset != m_ibBinding.offset)
    {
        if (_ibhOrInvalid)
        {
            const IndexBuffer& ib       = m_indexBuffers[_ibhOrInvalid];
            m_ibBinding.ibh             = _ibhOrInvalid;
            m_ibBinding.binder          = ib.pBuffer;
            m_ibBinding.offset          = _indexOffset * (ib.bUseU32 ? 4 : 2);
            m_ibBinding.numAvailIndices = ib.byteWidth / (ib.bUseU32 ? 4 : 2) - _indexOffset;
            m_ibBinding.format          = ib.bUseU32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        }
        else
        {
            m_ibBinding.ibh             = kInvalidHandle;
            m_ibBinding.binder          = nullptr;
            m_ibBinding.offset          = 0;
            m_ibBinding.numAvailIndices = 0;
            m_ibBinding.format          = DXGI_FORMAT_UNKNOWN;
        }
        m_dirtyFlags |= ePipelineDirtyFlags::IndexBuffer;
    }
    m_ibBinding.numIndices = Min(_numIndices, m_ibBinding.numAvailIndices);
}

void Graphics::SetConstantBuffer(
    const ConstantBufferHandle _cbh,
    const eShader              _shader,
    const Int32                _slot)
{
    constexpr EnumArray<eShader, ePipelineDirtyFlags> kFlags = {
        ePipelineDirtyFlags::VSConstantBuffer,
        ePipelineDirtyFlags::PSConstantBuffer,
        ePipelineDirtyFlags::GSConstantBuffer,
        ePipelineDirtyFlags::HSConstantBuffer,
        ePipelineDirtyFlags::DSConstantBuffer,
        ePipelineDirtyFlags::CSConstantBuffer
    };

    ConstantBufferBindings& bindings = m_cbufferBindings[_shader];
    if (bindings.cbhs[_slot] != _cbh)
    {
        bindings.cbhs[_slot] = _cbh;
        bindings.dirtyBegin  = Min(bindings.dirtyBegin, _slot);
        bindings.dirtyEnd    = Max(bindings.dirtyEnd, _slot + 1);

        if (_cbh)
        {
            ID3D11Buffer* pBuffer  = m_cbuffers[_cbh].pBuffer;
            bindings.binder[_slot] = pBuffer;
        }
        else
        {
            bindings.binder[_slot] = nullptr;
        }

        m_dirtyFlags |= kFlags[_shader];
    }
}

void Graphics::SetTexture(
    const TextureHandle   _texh,
    const eShader         _shader,
    const Int32           _slot,
    const eResourceAccess _access)
{
    SetShaderResource_(_texh.GetValue(), eShaderResource::Texture, _shader, _slot, _access);
}

void Graphics::SetBuffer(
    const StorageBufferHandle _sbh,
    const eShader             _shader,
    const Int32               _slot,
    const eResourceAccess     _access)
{
    SetShaderResource_(_sbh.GetValue(), eShaderResource::Buffer, _shader, _slot, _access);
}

void Graphics::SetSampler(
    const Flags<eSamplerFlags> _flags,
    const eShader              _shader,
    const Int32                _slot)
{
    // border color를 사용하지 않는 샘플러가 동일한 설정 + 다른 border color로 의미없이 생성되는 것을 방지하기 위함
    SetSampler(_flags, RGBA::kBlack, _shader, _slot);
}

void Graphics::SetSampler(
    const Flags<eSamplerFlags> _flags,
    const RGBA                 _rgba,
    const eShader              _shader,
    const Int32                _slot)
{
    constexpr EnumArray<eShader, ePipelineDirtyFlags> kFlags = {
        ePipelineDirtyFlags::VSSamplerState,
        ePipelineDirtyFlags::PSSamplerState,
        ePipelineDirtyFlags::GSSamplerState,
        ePipelineDirtyFlags::HSSamplerState,
        ePipelineDirtyFlags::DSSamplerState,
        ePipelineDirtyFlags::CSSamplerState
    };

    SamplerBindings&          bindings = m_samplerBindings[_shader];
    SamplerBindings::Sampler& sampler  = bindings.samplers[_slot];
    if (sampler.flags != _flags || sampler.rgba != _rgba)
    {
        sampler.flags          = _flags;
        sampler.rgba           = _rgba;
        bindings.binder[_slot] = GetOrCreateD3D11SamplerState_(_flags, _rgba);
        bindings.dirtyBegin    = Min(bindings.dirtyBegin, _slot);
        bindings.dirtyEnd      = Max(bindings.dirtyEnd, _slot + 1);
        m_dirtyFlags |= kFlags[_shader];   // set flags
    }
}

void Graphics::SetRenderFlags(
    const Flags<eRenderFlags> _flags)
{
    if (m_renderFlagsBinding != _flags)
    {
        // depth stencil state
        if (FilterDepthFlags(m_renderFlagsBinding) != FilterDepthFlags(_flags))
        {
            m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
        }

        // rasterizer state
        if (FilterRasterizerFlags(m_renderFlagsBinding) != FilterRasterizerFlags(_flags))
        {
            m_dirtyFlags |= ePipelineDirtyFlags::RasterizerState;
        }

        // blend state
        if (FilterBlendFlags(m_renderFlagsBinding) != FilterBlendFlags(_flags))
        {
            m_dirtyFlags |= ePipelineDirtyFlags::BlendState;
        }

        // primitive
        if (FilterPrimitiveFlags(m_renderFlagsBinding) != FilterPrimitiveFlags(_flags))
        {
            m_dirtyFlags |= ePipelineDirtyFlags::PrimitiveTopology;
        }

        m_renderFlagsBinding = _flags;
    }
}

void Graphics::SetBlendFlags(
    const Flags<eBlendFlags> _flags,
    const Int32              _renderTargetIndex)
{
    const Flags<eBlendFlags> flags = m_blendFlagsBindings[_renderTargetIndex];
    if (_flags != flags)
    {
        m_blendFlagsBindings[_renderTargetIndex] = _flags;
        m_dirtyFlags |= ePipelineDirtyFlags::BlendState;
    }
}

void Graphics::SetBlendFactor(
    const RGBA _rgba)
{
    if (m_blendFactorBinding.rgba != _rgba.rgba)
    {
        m_blendFactorBinding = _rgba;
        m_dirtyFlags |= ePipelineDirtyFlags::BlendState;
    }
}

void Graphics::SetStencilFlags(
    const Flags<eStencilFlags> _fStencil,
    const Flags<eStencilFlags> _bStencil,
    const UInt8                _stencilRef)
{
    if (_fStencil != m_fstencilBinding)
    {
        m_fstencilBinding = _fStencil;
        m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
    }

    if (_bStencil != m_bstencilBinding)
    {
        m_bstencilBinding = _bStencil;
        m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
    }

    if (m_stencilRefBinding != _stencilRef)
    {
        m_stencilRefBinding = _stencilRef;
        m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
    }
}

void Graphics::SetProgram(
    const ProgramHandle _ph)
{
    if (m_phBinding != _ph)
    {
        m_phBinding = _ph;
        m_dirtyFlags |= ePipelineDirtyFlags::Program;
    }
}

void Graphics::SetComputeProgram(
    const ProgramHandle _ph)
{
    if (m_computePhBinding != _ph)
    {
        m_computePhBinding = _ph;
        m_dirtyFlags |= ePipelineDirtyFlags::ComputeProgram;
    }
}

void Graphics::SetFrameBuffer(
    const FrameBufferHandle _fbh)
{
    if (m_fbhBinding != _fbh)
    {
        m_fbhBinding = _fbh;
        m_dirtyFlags |= ePipelineDirtyFlags::FrameBuffer;
    }
}

void Graphics::SetViewport(
    const Viewport _vp)
{
    if (m_vpBinding != _vp)
    {
        m_vpBinding = _vp;
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
    if (m_scissorRectBinding != _rect)
    {
        m_scissorRectBinding = _rect;
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
    const eSubmitParameter _type,
    const Int32            _value)
{
    m_submitParamsBinding[_type] = _value;
}

void Graphics::Touch()
{
    BindPipeline_();
}

void Graphics::Reset()
{
    JAM_FUNCTION_PROFILE();

    // reset pipeline cache
    m_vbBindings      = {};
    m_ibBinding       = {};
    m_instanceBinding = {};

    for (ReadBindings& bindings: m_readBindings)
    {
        bindings = {};
    }

    for (SamplerBindings& bindings: m_samplerBindings)
    {
        bindings = {};
    }

    for (ConstantBufferBindings& bindings: m_cbufferBindings)
    {
        bindings = {};
    }

    m_csReadWriteBindings = {};
    m_psReadWriteBindings = {};

    m_fbhBinding          = kInvalidHandle;
    m_vpBinding           = {};
    m_scissorRectBinding  = {};
    m_phBinding           = kInvalidHandle;
    m_submitParamsBinding = {};
    m_computePhBinding    = kInvalidHandle;

    m_renderFlagsBinding = eRenderFlags::None;
    m_blendFlagsBindings = {};
    m_blendFactorBinding = {};
    m_fstencilBinding    = eStencilFlags::None;
    m_bstencilBinding    = eStencilFlags::None;
    m_stencilRefBinding  = {};

    m_dirtyFlags = ePipelineDirtyFlags::All;

    // reset api
    constexpr Array<ID3D11Buffer*, kMaxNumConstantBufferSlots>                   kZeroBuffers  = {};
    constexpr Array<UINT, kMaxNumVertexBufferSlots>                              kZeroStrides  = {};
    constexpr Array<UINT, kMaxNumVertexBufferSlots>                              kZeroOffsets  = {};
    constexpr Array<ID3D11ShaderResourceView*, kMaxNumShaderResourceViewSlots>   kZeroSrvs     = {};
    constexpr Array<ID3D11UnorderedAccessView*, kMaxNumUnorderedAccessViewSlots> kZeroUavs     = {};
    constexpr Array<ID3D11SamplerState*, kMaxNumSamplerSlots>                    kZeroSamplers = {};

    m_pd3d11DeviceContext->IASetVertexBuffers(0, kMaxNumVertexBufferSlots, kZeroBuffers.data(), kZeroStrides.data(), kZeroOffsets.data());
    m_pd3d11DeviceContext->IASetIndexBuffer(kZeroBuffers[0], DXGI_FORMAT_UNKNOWN, 0);
    m_pd3d11DeviceContext->IASetInputLayout(nullptr);
    m_pd3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED);
    m_pd3d11DeviceContext->VSSetConstantBuffers(0, kMaxNumConstantBufferSlots, kZeroBuffers.data());
    m_pd3d11DeviceContext->PSSetConstantBuffers(0, kMaxNumConstantBufferSlots, kZeroBuffers.data());
    m_pd3d11DeviceContext->GSSetConstantBuffers(0, kMaxNumConstantBufferSlots, kZeroBuffers.data());
    m_pd3d11DeviceContext->HSSetConstantBuffers(0, kMaxNumConstantBufferSlots, kZeroBuffers.data());
    m_pd3d11DeviceContext->DSSetConstantBuffers(0, kMaxNumConstantBufferSlots, kZeroBuffers.data());
    m_pd3d11DeviceContext->CSSetConstantBuffers(0, kMaxNumConstantBufferSlots, kZeroBuffers.data());
    m_pd3d11DeviceContext->VSSetShaderResources(0, kMaxNumShaderResourceViewSlots, kZeroSrvs.data());
    m_pd3d11DeviceContext->PSSetShaderResources(0, kMaxNumShaderResourceViewSlots, kZeroSrvs.data());
    m_pd3d11DeviceContext->GSSetShaderResources(0, kMaxNumShaderResourceViewSlots, kZeroSrvs.data());
    m_pd3d11DeviceContext->HSSetShaderResources(0, kMaxNumShaderResourceViewSlots, kZeroSrvs.data());
    m_pd3d11DeviceContext->DSSetShaderResources(0, kMaxNumShaderResourceViewSlots, kZeroSrvs.data());
    m_pd3d11DeviceContext->CSSetShaderResources(0, kMaxNumShaderResourceViewSlots, kZeroSrvs.data());
    m_pd3d11DeviceContext->CSSetUnorderedAccessViews(0, kMaxNumUnorderedAccessViewSlots, kZeroUavs.data(), nullptr);
    m_pd3d11DeviceContext->VSSetSamplers(0, kMaxNumSamplerSlots, kZeroSamplers.data());
    m_pd3d11DeviceContext->PSSetSamplers(0, kMaxNumSamplerSlots, kZeroSamplers.data());
    m_pd3d11DeviceContext->GSSetSamplers(0, kMaxNumSamplerSlots, kZeroSamplers.data());
    m_pd3d11DeviceContext->HSSetSamplers(0, kMaxNumSamplerSlots, kZeroSamplers.data());
    m_pd3d11DeviceContext->DSSetSamplers(0, kMaxNumSamplerSlots, kZeroSamplers.data());
    m_pd3d11DeviceContext->CSSetSamplers(0, kMaxNumSamplerSlots, kZeroSamplers.data());
    m_pd3d11DeviceContext->VSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->PSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->GSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->HSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->DSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->CSSetShader(nullptr, nullptr, 0);
    m_pd3d11DeviceContext->RSSetViewports(0, nullptr);
    m_pd3d11DeviceContext->RSSetScissorRects(0, nullptr);
    m_pd3d11DeviceContext->RSSetState(nullptr);
    m_pd3d11DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_pd3d11DeviceContext->OMSetDepthStencilState(nullptr, 0);
    m_pd3d11DeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 0, nullptr, nullptr);
}

void Graphics::Submit()
{
    BindPipeline_();

    if (m_ibBinding.ibh)
    {
        if (m_instanceBinding.vbh)
        {
            const Int32 startIdxLoc  = m_submitParamsBinding[eSubmitParameter::StartIndexLocation];
            const Int32 baseVtxLoc   = m_submitParamsBinding[eSubmitParameter::BaseVertexLocation];
            const Int32 startInstLoc = m_submitParamsBinding[eSubmitParameter::StartInstanceLocation];
            m_pd3d11DeviceContext->DrawIndexedInstanced(m_ibBinding.numIndices, m_instanceBinding.numInstances, startIdxLoc, baseVtxLoc, startInstLoc);
        }
        else
        {
            const Int32 startIdxLoc = m_submitParamsBinding[eSubmitParameter::StartIndexLocation];
            const Int32 baseVtxLoc  = m_submitParamsBinding[eSubmitParameter::BaseVertexLocation];
            m_pd3d11DeviceContext->DrawIndexed(m_ibBinding.numIndices, startIdxLoc, baseVtxLoc);
        }
    }
    else
    {
        if (m_instanceBinding.vbh)
        {
            const Int32 startVertLoc = m_submitParamsBinding[eSubmitParameter::StartVertexLocation];
            const Int32 startInstLoc = m_submitParamsBinding[eSubmitParameter::StartInstanceLocation];
            m_pd3d11DeviceContext->DrawInstanced(m_vbBindings.numVerts, m_instanceBinding.numInstances, startVertLoc, startInstLoc);
        }
        else
        {
            const Int32 startVertLoc = m_submitParamsBinding[eSubmitParameter::StartVertexLocation];
            m_pd3d11DeviceContext->Draw(m_vbBindings.numVerts, startVertLoc);
        }
    }

    ++m_stats.numDrawCalls;
}

void Graphics::Dispatch(
    const Int32 _numThreadGroupsX,
    const Int32 _numThreadGroupsY,
    const Int32 _numThreadGroupsZ)
{
    BindPipeline_();
    m_pd3d11DeviceContext->Dispatch(_numThreadGroupsX, _numThreadGroupsY, _numThreadGroupsZ);
    ++m_stats.numDispatchCalls;
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
        constexpr D3D_FEATURE_LEVEL kFeatureLevels[] = {
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
            kFeatureLevels,
            std::size(kFeatureLevels),
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
        swapChainDesc.Scaling               = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags                 = 0;
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
        if (pBlendState)
        {
            pBlendState->Release();
        }
    }

    for (ID3D11DepthStencilState* pDepthStencilState: m_d3d11DepthStencilStateCache | std::views::values)
    {
        if (pDepthStencilState)
        {
            pDepthStencilState->Release();
        }
    }

    for (ID3D11RasterizerState* pRasterizerState: m_d3d11RasterizerStateCache | std::views::values)
    {
        if (pRasterizerState)
        {
            pRasterizerState->Release();
        }
    }

    for (ID3D11SamplerState* pSamplerState: m_d3d11SamplerStateCache | std::views::values)
    {
        if (pSamplerState)
        {
            pSamplerState->Release();
        }
    }

    for (ID3D11InputLayout* pInputLayout: m_d3d11InputLayoutCache | std::views::values)
    {
        if (pInputLayout)
        {
            pInputLayout->Release();
        }
    }

    // 내부 리소스 해제
    if (m_pUploadBufferCache)
    {
        if (m_pUploadBufferCache->Release() != 0)
        {
            JAM_ASSERT(0, "Upload buffer cache is still in use. All buffers from the cache must be released before destroying the graphics instance.");
        }
    }

    // d3d11 device 해제
    m_pdxgiSwapchain->Release();
    m_pd3d11DeviceContext->Release();
    m_pd3d11Device->Release();

    // 릭 검사
    if (DetectLeak())
    {
        JAM_ASSERT(0, "Graphics instance is being destroyed while there are still live resources. Please check for resource leaks.");
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
    ConstantBufferBindings& bindings = m_cbufferBindings[_stage];

    // dirty range
    const Int32 numDirties = bindings.dirtyEnd - bindings.dirtyBegin;
    if (numDirties <= 0)
    {
        return;
    }

    ID3D11Buffer** ppBinder = bindings.binder.data() + bindings.dirtyBegin;

    // bind
    switch (_stage)
    {
        case eShader::VertexShader:
            m_pd3d11DeviceContext->VSSetConstantBuffers(bindings.dirtyBegin, numDirties, ppBinder);
            break;
        case eShader::PixelShader:
            m_pd3d11DeviceContext->PSSetConstantBuffers(bindings.dirtyBegin, numDirties, ppBinder);
            break;
        case eShader::GeometryShader:
            m_pd3d11DeviceContext->GSSetConstantBuffers(bindings.dirtyBegin, numDirties, ppBinder);
            break;
        case eShader::ComputeShader:
            m_pd3d11DeviceContext->CSSetConstantBuffers(bindings.dirtyBegin, numDirties, ppBinder);
            break;
        case eShader::HullShader:
            m_pd3d11DeviceContext->HSSetConstantBuffers(bindings.dirtyBegin, numDirties, ppBinder);
            break;
        case eShader::DomainShader:
            m_pd3d11DeviceContext->DSSetConstantBuffers(bindings.dirtyBegin, numDirties, ppBinder);
            break;
        default:
            JAM_UNREACHABLE;
    }

    // reset dirty range
    bindings.dirtyBegin = kMaxNumConstantBufferSlots;
    bindings.dirtyEnd   = 0;
}

void Graphics::BindShaderResourceViews_(
    const eShader _shader)
{
    ReadBindings& binds = m_readBindings[_shader];

    const Int32 numDirtySlots = binds.dirtyEnd - binds.dirtyBegin;
    if (numDirtySlots <= 0)
    {
        return;
    }

    ID3D11ShaderResourceView** ppBinder = binds.binder.data() + binds.dirtyBegin;

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            m_pd3d11DeviceContext->VSSetShaderResources(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::PixelShader:
            m_pd3d11DeviceContext->PSSetShaderResources(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::HullShader:
            m_pd3d11DeviceContext->HSSetShaderResources(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::DomainShader:
            m_pd3d11DeviceContext->DSSetShaderResources(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::GeometryShader:
            m_pd3d11DeviceContext->GSSetShaderResources(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::ComputeShader:
            m_pd3d11DeviceContext->CSSetShaderResources(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        default:
            JAM_UNREACHABLE;
    }

    // reset dirty range
    binds.dirtyBegin = kMaxNumShaderResourceViewSlots;
    binds.dirtyEnd   = 0;
}

void Graphics::BindSamplerStates_(
    const eShader _shader)
{
    SamplerBindings& binds         = m_samplerBindings[_shader];
    const Int32      numDirtySlots = binds.dirtyEnd - binds.dirtyBegin;
    if (numDirtySlots <= 0)
    {
        return;
    }

    ID3D11SamplerState** ppBinder = binds.binder.data() + binds.dirtyBegin;

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            m_pd3d11DeviceContext->VSSetSamplers(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::PixelShader:
            m_pd3d11DeviceContext->PSSetSamplers(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::GeometryShader:
            m_pd3d11DeviceContext->GSSetSamplers(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::ComputeShader:
            m_pd3d11DeviceContext->CSSetSamplers(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::HullShader:
            m_pd3d11DeviceContext->HSSetSamplers(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        case eShader::DomainShader:
            m_pd3d11DeviceContext->DSSetSamplers(binds.dirtyBegin, numDirtySlots, ppBinder);
            break;
        default:
            JAM_UNREACHABLE;
    }

    // reset dirty range
    binds.dirtyBegin = kMaxNumSamplerSlots;
    binds.dirtyEnd   = 0;
}

void Graphics::BindPipeline_()
{
    if (m_dirtyFlags == ePipelineDirtyFlags::None)
    {
        return;
    }

    // topology
    if (m_dirtyFlags & ePipelineDirtyFlags::PrimitiveTopology)
    {
        const UInt32 primitive = ResolvePrimitiveFlags(m_renderFlagsBinding);
        m_pd3d11DeviceContext->IASetPrimitiveTopology(D3D11PrimitiveTopologyFrom_(primitive));
    }

    // vertex buffer
    if (m_dirtyFlags & ePipelineDirtyFlags::VertexBuffer)
    {
        if (m_vbBindings.numBuffers > 0)
        {
            ID3D11Buffer** ppBuffer = m_vbBindings.binder.data();
            UInt32*        pStride  = m_vbBindings.strides.data();
            UInt32*        pOffset  = m_vbBindings.offsets.data();
            m_pd3d11DeviceContext->IASetVertexBuffers(0, m_vbBindings.numBuffers, ppBuffer, pStride, pOffset);
        }
        else
        {
            ID3D11Buffer* pBuffer = nullptr;
            UInt32        stride  = 0;
            UInt32        offset  = 0;
            m_pd3d11DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &stride, &offset);
        }

        ++m_stats.numVertexBufferChanges;
    }

    // instance buffer
    if (m_dirtyFlags & ePipelineDirtyFlags::InstanceBuffer)
    {
        if (m_instanceBinding.vbh)
        {
            m_pd3d11DeviceContext->IASetVertexBuffers(kMaxNumVertexBufferSlots, 1, &m_instanceBinding.binder, &m_instanceBinding.stride, &m_instanceBinding.offset);
        }
        else
        {
            ID3D11Buffer* pBuffer = nullptr;
            UInt32        stride  = 0;
            UInt32        offset  = 0;
            m_pd3d11DeviceContext->IASetVertexBuffers(kMaxNumVertexBufferSlots, 1, &pBuffer, &stride, &offset);
        }

        ++m_stats.numInstanceBufferChanges;
    }

    // input layout
    if (m_dirtyFlags & ePipelineDirtyFlags::InputLayout)
    {
        ID3D11InputLayout* pInputLayout = GetOrCreateD3D11InputLayoutFromCurrentBind_();
        m_pd3d11DeviceContext->IASetInputLayout(pInputLayout);
    }

    // index buffer
    if (m_dirtyFlags & ePipelineDirtyFlags::IndexBuffer)
    {
        if (m_ibBinding.ibh)
        {
            m_pd3d11DeviceContext->IASetIndexBuffer(m_ibBinding.binder, m_ibBinding.format, m_ibBinding.offset);
        }
        else
        {
            m_pd3d11DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
        }

        ++m_stats.numIndexBufferChanges;
    }

    // shader program
    if (m_dirtyFlags & ePipelineDirtyFlags::Program)
    {
        if (m_phBinding)
        {
            const Program& program = m_programs[m_phBinding];
            if (program.vsh)
            {
                ShaderRef& shader = m_shaders[program.vsh];
                m_pd3d11DeviceContext->VSSetShader(shader.pVertexShader, nullptr, 0);
            }
            else
            {
                m_pd3d11DeviceContext->VSSetShader(nullptr, nullptr, 0);
            }

            if (program.psh)
            {
                ShaderRef& shader = m_shaders[program.psh];
                m_pd3d11DeviceContext->PSSetShader(shader.pPixelShader, nullptr, 0);
            }
            else
            {
                m_pd3d11DeviceContext->PSSetShader(nullptr, nullptr, 0);
            }

            if (program.gsh)
            {
                ShaderRef& shader = m_shaders[program.gsh];
                m_pd3d11DeviceContext->GSSetShader(shader.pGeometryShader, nullptr, 0);
            }
            else
            {
                m_pd3d11DeviceContext->GSSetShader(nullptr, nullptr, 0);
            }

            if (program.hsh)
            {
                ShaderRef& shader = m_shaders[program.hsh];
                m_pd3d11DeviceContext->HSSetShader(shader.pHullShader, nullptr, 0);
            }
            else
            {
                m_pd3d11DeviceContext->HSSetShader(nullptr, nullptr, 0);
            }

            if (program.dsh)
            {
                ShaderRef& shader = m_shaders[program.dsh];
                m_pd3d11DeviceContext->DSSetShader(shader.pDomainShader, nullptr, 0);
            }
            else
            {
                m_pd3d11DeviceContext->DSSetShader(nullptr, nullptr, 0);
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

        ++m_stats.numProgramChanges;
    }

    // compute shader program
    if (m_dirtyFlags & ePipelineDirtyFlags::ComputeProgram)
    {
        if (m_computePhBinding)
        {
            const Program&   program = m_programs[m_computePhBinding];
            const ShaderRef& shader  = m_shaders[program.csh];
            m_pd3d11DeviceContext->CSSetShader(shader.pComputeShader, nullptr, 0);
        }
        else
        {
            m_pd3d11DeviceContext->CSSetShader(nullptr, nullptr, 0);
        }

        ++m_stats.numComputeProgramChanges;
    }

    // constant buffer
    if (m_dirtyFlags & ePipelineDirtyFlags::ConstantBufferMask)
    {
        if (m_dirtyFlags & ePipelineDirtyFlags::VSConstantBuffer)
        {
            BindConstantBuffers_(eShader::VertexShader);
            ++m_stats.numVSConstantBufferChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::PSConstantBuffer)
        {
            BindConstantBuffers_(eShader::PixelShader);
            ++m_stats.numPSConstantBufferChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::GSConstantBuffer)
        {
            BindConstantBuffers_(eShader::GeometryShader);
            ++m_stats.numGSConstantBufferChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::HSConstantBuffer)
        {
            BindConstantBuffers_(eShader::HullShader);
            ++m_stats.numHSConstantBufferChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::DSConstantBuffer)
        {
            BindConstantBuffers_(eShader::DomainShader);
            ++m_stats.numDSConstantBufferChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::CSConstantBuffer)
        {
            BindConstantBuffers_(eShader::ComputeShader);
            ++m_stats.numCSConstantBufferChanges;
        }
    }

    // sampler state
    if (m_dirtyFlags & ePipelineDirtyFlags::SamplerStateMask)
    {
        if (m_dirtyFlags & ePipelineDirtyFlags::VSSamplerState)
        {
            BindSamplerStates_(eShader::VertexShader);
            ++m_stats.numVSSamplerStateChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::PSSamplerState)
        {
            BindSamplerStates_(eShader::PixelShader);
            ++m_stats.numPSSamplerStateChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::GSSamplerState)
        {
            BindSamplerStates_(eShader::GeometryShader);
            ++m_stats.numGSSamplerStateChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::HSSamplerState)
        {
            BindSamplerStates_(eShader::HullShader);
            ++m_stats.numHSSamplerStateChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::DSSamplerState)
        {
            BindSamplerStates_(eShader::DomainShader);
            ++m_stats.numDSSamplerStateChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::CSSamplerState)
        {
            BindSamplerStates_(eShader::ComputeShader);
            ++m_stats.numCSSamplerStateChanges;
        }
    }

    // frame buffer + unordered access view
    // Q: why frame buffer bind before srv?
    // A: because msaa render target resolve.
    // automatically resolve msaa render target when bind as shader resource view.
    const bool bFrameBufferDirty = m_dirtyFlags.IsEnabled(ePipelineDirtyFlags::FrameBuffer);
    const bool bPSUavDirty       = m_dirtyFlags.IsEnabled(ePipelineDirtyFlags::PSUnorderedAccessView);
    if (bFrameBufferDirty || bPSUavDirty)
    {
        const FrameBufferHandle fbh = m_fbhBinding ? m_fbhBinding : m_backBufferFbh;
        const FrameBuffer&      fb  = m_frameBuffers[fbh];

        // resolve
        if (fb.bMsaa)
        {
            for (Int32 i = 0; i < fb.numRenderTargets; ++i)
            {
                const Attachment          attachment = fb.attachments[i];
                const HardwareTextureRef& texture    = m_textures[attachment.texh];
                const TextureFormatInfo   format     = TextureFormatInfoFrom(texture.format);
                JAM_ASSERT(texture.samples > 1, "Texture must be multisampled to resolve.");

                for (Int32 j = 0; j < texture.numLayers; ++j)
                {
                    const UInt32 index = CalcTextureIndex(attachment.mipIndex, j, texture.numMips);
                    m_pd3d11DeviceContext->ResolveSubresource(
                        texture.pResource,
                        index,
                        texture.pMsaaResource,
                        index,
                        format.texFormat);

                    ++m_stats.numResolve;
                }

                if (texture.numMips > 1)
                {
                    JAM_ASSERT(texture.bRtvAttachable, "Texture must be created with RenderTarget flag to generate mips.");
                    m_pd3d11DeviceContext->GenerateMips(texture.pSrv);
                }
            }
        }

        if (m_psReadWriteBindings.useMask)
        {
            ID3D11RenderTargetView* const* pRtvs   = nullptr;
            ID3D11DepthStencilView*        pDsv    = nullptr;
            ID3D11UnorderedAccessView**    ppUavs  = nullptr;
            UInt32                         numRtvs = D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL;
            UInt32                         numUavs = D3D11_KEEP_UNORDERED_ACCESS_VIEWS;

            if (bFrameBufferDirty)
            {
                pRtvs   = fb.pRtvs.data();
                pDsv    = fb.pDsv;
                numRtvs = fb.numRenderTargets;
                ++m_stats.numFrameBufferChanges;
            }

            if (bPSUavDirty)
            {
                const Int32 end = 64 - UInt64CountLZero(m_psReadWriteBindings.useMask);
                numUavs         = end - fb.numRenderTargets;
                ppUavs          = m_psReadWriteBindings.binder.data();
                ++m_stats.numPSReadWriteResourceChanges;
            }

            m_pd3d11DeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
                numRtvs,
                pRtvs,
                pDsv,
                numRtvs,
                numUavs,
                ppUavs,
                nullptr);
        }
        else
        {
            m_pd3d11DeviceContext->OMSetRenderTargets(fb.numRenderTargets, fb.pRtvs.data(), fb.pDsv);
            ++m_stats.numFrameBufferChanges;
        }
    }

    // shader resource
    if (m_dirtyFlags & ePipelineDirtyFlags::ShaderResourceViewMask)
    {
        if (m_dirtyFlags & ePipelineDirtyFlags::VSShaderResourceView)
        {
            BindShaderResourceViews_(eShader::VertexShader);
            ++m_stats.numVSReadResourceChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::PSShaderResourceView)
        {
            BindShaderResourceViews_(eShader::PixelShader);
            ++m_stats.numPSReadResourceChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::GSShaderResourceView)
        {
            BindShaderResourceViews_(eShader::GeometryShader);
            ++m_stats.numGSReadResourceChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::HSShaderResourceView)
        {
            BindShaderResourceViews_(eShader::HullShader);
            ++m_stats.numHSReadResourceChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::DSShaderResourceView)
        {
            BindShaderResourceViews_(eShader::DomainShader);
            ++m_stats.numDSReadResourceChanges;
        }

        if (m_dirtyFlags & ePipelineDirtyFlags::CSShaderResourceView)
        {
            BindShaderResourceViews_(eShader::ComputeShader);
            ++m_stats.numCSReadResourceChanges;
        }
    }

    // unordered access
    if (m_dirtyFlags & ePipelineDirtyFlags::CSUnorderedAccessView)
    {
        ReadWriteBindings& binds         = m_csReadWriteBindings;
        const Int32        numDirtySlots = binds.dirtyEnd - binds.dirtyBegin;
        if (numDirtySlots > 0)
        {
            ID3D11UnorderedAccessView** ppBinder = binds.binder.data() + binds.dirtyBegin;
            m_pd3d11DeviceContext->CSSetUnorderedAccessViews(binds.dirtyBegin, numDirtySlots, ppBinder, nullptr);

            // reset dirty range
            binds.dirtyBegin = kMaxNumUnorderedAccessViewSlots;
            binds.dirtyEnd   = 0;
        }

        ++m_stats.numCSReadWriteResourceChanges;
    }

    // blend state
    if (m_dirtyFlags & ePipelineDirtyFlags::BlendState)
    {
        const FLOAT blendFactor[4] = {
            static_cast<float>(m_blendFactorBinding.r) / 255.f,
            static_cast<float>(m_blendFactorBinding.g) / 255.f,
            static_cast<float>(m_blendFactorBinding.b) / 255.f,
            static_cast<float>(m_blendFactorBinding.a) / 255.f
        };

        ID3D11BlendState* pBs = GetOrCreateD3D11BlendStateFromCurrentBind_();
        m_pd3d11DeviceContext->OMSetBlendState(pBs, blendFactor, 0xFFFFFFFF);
        ++m_stats.numBlendStateChanges;
    }

    // depth stencil state
    if (m_dirtyFlags & ePipelineDirtyFlags::DepthStencilState)
    {
        ID3D11DepthStencilState* pBss = GetOrCreateD3D11DepthStencilStateFromCurrentBind_();
        m_pd3d11DeviceContext->OMSetDepthStencilState(pBss, m_stencilRefBinding);
        ++m_stats.numDepthStencilStateChanges;
    }

    // rasterizer state
    if (m_dirtyFlags & ePipelineDirtyFlags::RasterizerState)
    {
        ID3D11RasterizerState* pRs = GetOrCreateD3D11RasterizerStateFromCurrentBind_();
        m_pd3d11DeviceContext->RSSetState(pRs);
        ++m_stats.numRasterizerStateChanges;
    }

    // viewport
    if (m_dirtyFlags & ePipelineDirtyFlags::Viewport)
    {
        const D3D11_VIEWPORT vp = {
            static_cast<float>(m_vpBinding.x),
            static_cast<float>(m_vpBinding.y),
            static_cast<float>(m_vpBinding.w),
            static_cast<float>(m_vpBinding.h),
            0.f,
            1.f
        };
        m_pd3d11DeviceContext->RSSetViewports(1, &vp);
        ++m_stats.numViewportChanges;
    }

    // scissor rect
    if (m_dirtyFlags & ePipelineDirtyFlags::ScissorRect)
    {
        const D3D11_RECT rect = {
            m_scissorRectBinding.left,
            m_scissorRectBinding.top,
            m_scissorRectBinding.right,
            m_scissorRectBinding.bottom
        };
        m_pd3d11DeviceContext->RSSetScissorRects(1, &rect);
        ++m_stats.numScissorRectChanges;
    }

    // clear dirty flags
    m_dirtyFlags = ePipelineDirtyFlags::None;
}

void Graphics::OnResize_(
    const Int32 _width,
    const Int32 _height)
{
    if (_width == m_backBufferResolution.x && _height == m_backBufferResolution.y)
    {
        return;
    }

    m_backBufferResolution.x = _width;
    m_backBufferResolution.y = _height;

    // resize back buffer
    if (m_pdxgiSwapchain)
    {
        // release framebuffer binding
        m_pd3d11DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
        m_fbhBinding = kInvalidHandle;
        m_dirtyFlags |= ePipelineDirtyFlags::FrameBuffer;

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
            texture.width              = static_cast<Int32>(desc.Width);
            texture.height             = static_cast<Int32>(desc.Height);
            texture.depth              = 1;
            texture.type               = eTexture::Texture2D;
            texture.format             = eTextureFormat::RGBA8_UNorm;
            texture.numLayers          = 1;
            texture.samples            = 1;
            texture.numMips            = 1;
            texture.refCount           = 1;
            texture.bRtvAttachable     = true;
            texture.bDsvAttachable     = false;
            texture.bImmutable         = false;
            texture.bReadback          = false;
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

HardwareBuffer Graphics::CreateHardwareBuffer_(
    const Int32      _byteWidth,
    const eBuffer    _type,
    const MemoryView _initDataOrEmpty) const
{
    JAM_ASSERT(_byteWidth > 0, "Buffer size must be greater than zero.");

    if (_type == eBuffer::ConstantBuffer)
    {
        JAM_ASSERT(_initDataOrEmpty.IsEmpty(), "Constant buffers cannot have initial data. Use UpdateBuffer to set initial data for constant buffers.");
        JAM_ASSERT(_byteWidth % 16 == 0, "Byte width of constant buffer must be a multiple of 16.");
    }

    D3D11_BUFFER_DESC desc   = {};
    desc.ByteWidth           = static_cast<UINT>(_byteWidth);
    desc.StructureByteStride = 0;
    desc.MiscFlags           = 0;
    desc.BindFlags           = _type == eBuffer::VertexBuffer   ? D3D11_BIND_VERTEX_BUFFER
                             : _type == eBuffer::IndexBuffer    ? D3D11_BIND_INDEX_BUFFER
                             : _type == eBuffer::ConstantBuffer ? D3D11_BIND_CONSTANT_BUFFER
                                                                : 0;

    const bool bHasInitData = !_initDataOrEmpty.IsEmpty();
    desc.Usage              = bHasInitData ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags     = bHasInitData ? 0 : D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = bHasInitData ? _initDataOrEmpty.GetPtr() : nullptr;

    HardwareBuffer buffer = {};
    buffer.type           = _type;
    buffer.byteWidth      = _byteWidth;
    buffer.bDynamic       = desc.Usage == D3D11_USAGE_DYNAMIC;
    buffer.bImmutable     = desc.Usage == D3D11_USAGE_IMMUTABLE;

    JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, bHasInitData ? &initData : nullptr, &buffer.pBuffer));

    return buffer;
}

StorageBuffer Graphics::CreateStorageBuffer_(
    const Int32                            _byteWidth,
    const Int32                            _stride,   // effective only when _type is eStorageBuffer::Structured
    const eStorageBuffer                   _type,
    const eStorageBufferData               _dataType,   // effective only when _type is eStorageBuffer::Typed
    const Flags<eStorageBufferCreateFlags> _flags,
    const bool                             _bAppendConsumer,   // effective only when _type is eStorageBuffer::Structured
    const bool                             _bCounter,          // effective only when _type is eStorageBuffer::Structured
    const MemoryView                       _initDataOrEmpty) const
{
    JAM_ASSERT(_byteWidth > 0, "Buffer size must be greater than zero.");
    JAM_ASSERT(!(_bAppendConsumer && _bCounter), "Buffer cannot be both append/consume and counter.");

    if (_type == eStorageBuffer::Readback)
    {
        JAM_ASSERT(_stride == 0, "Stride must be zero for readback buffer.");
        JAM_ASSERT(_flags == eStorageBufferCreateFlags::None, "No flags can be set for readback buffer.");

        D3D11_BUFFER_DESC desc   = {};
        desc.ByteWidth           = static_cast<UINT>(_byteWidth);
        desc.Usage               = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
        desc.BindFlags           = 0;
        desc.MiscFlags           = 0;
        desc.StructureByteStride = 0;

        StorageBuffer buffer  = {};
        buffer.type           = eBuffer::StorageBuffer;
        buffer.byteWidth      = _byteWidth;
        buffer.bImmutable     = false;
        buffer.bDynamic       = false;
        buffer.storageType    = _type;
        buffer.numElems       = 0;
        buffer.stride         = 0;
        buffer.bReadback      = true;
        buffer.bIndirectArgs  = false;
        buffer.bAppendConsume = false;
        buffer.bCounter       = false;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem                = _initDataOrEmpty.IsEmpty() ? nullptr : _initDataOrEmpty.GetPtr();
        JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, initData.pSysMem ? &initData : nullptr, &buffer.pBuffer));
        return buffer;
    }

    const bool  bHasInitData = !_initDataOrEmpty.IsEmpty();
    Int32       stride       = 0;
    Int32       numElements  = 0;
    DXGI_FORMAT format       = DXGI_FORMAT_UNKNOWN;

    switch (_type)
    {
        case eStorageBuffer::Raw:
        {
            JAM_ASSERT(_byteWidth % 4 == 0, "Byte width of raw buffer must be a multiple of 4.");
            stride      = 4;
            format      = DXGI_FORMAT_R32_TYPELESS;
            numElements = _byteWidth / stride;
            break;
        }
        case eStorageBuffer::Structured:
        {
            JAM_ASSERT(_byteWidth % _stride == 0, "Byte width must be a multiple of stride for structured buffer.");
            stride      = _stride;
            format      = DXGI_FORMAT_UNKNOWN;
            numElements = _byteWidth / stride;
            break;
        }
        case eStorageBuffer::Typed:
        {
            const BufferDataTypeInfo info = BufferDataTypeInfoFrom_(_dataType);
            stride                        = info.stride;
            format                        = info.format;
            JAM_ASSERT(_byteWidth % stride == 0, "Byte width must be a multiple of stride for typed buffer.");
            numElements = _byteWidth / stride;
            break;
        }
        default:
            JAM_UNREACHABLE;
    }

    D3D11_BUFFER_DESC desc   = {};
    desc.ByteWidth           = static_cast<UINT>(_byteWidth);
    desc.StructureByteStride = _type == eStorageBuffer::Raw ? 0 : static_cast<UINT>(stride);
    desc.BindFlags           = 0;
    desc.MiscFlags           = 0;

    if (_flags & eStorageBufferCreateFlags::ShaderRead)
    {
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }

    if (_flags & eStorageBufferCreateFlags::ShaderReadWrite)
    {
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    if (_flags & eStorageBufferCreateFlags::IndirectArgs)
    {
        desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    }

    if (_type == eStorageBuffer::Raw)
    {
        desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    }

    if (_type == eStorageBuffer::Structured)
    {
        desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    }

    if (_flags & eStorageBufferCreateFlags::ShaderReadWrite)
    {
        desc.Usage          = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
    }
    else if (!bHasInitData || _flags & eStorageBufferCreateFlags::Dynamic)
    {
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        desc.Usage          = D3D11_USAGE_IMMUTABLE;
        desc.CPUAccessFlags = 0;
    }

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = bHasInitData ? _initDataOrEmpty.GetPtr() : nullptr;

    StorageBuffer buffer  = {};
    buffer.type           = eBuffer::StorageBuffer;
    buffer.byteWidth      = _byteWidth;
    buffer.bImmutable     = desc.Usage == D3D11_USAGE_IMMUTABLE;
    buffer.bDynamic       = desc.Usage == D3D11_USAGE_DYNAMIC;
    buffer.storageType    = _type;
    buffer.stride         = stride;
    buffer.numElems       = numElements;
    buffer.bReadback      = false;
    buffer.bIndirectArgs  = _flags.IsEnabled(eStorageBufferCreateFlags::IndirectArgs);
    buffer.bAppendConsume = _bAppendConsumer;
    buffer.bCounter       = _bCounter;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, bHasInitData ? &initData : nullptr, &buffer.pBuffer));

    // SRV
    if (_flags & eStorageBufferCreateFlags::ShaderRead)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = format;

        if (_type == eStorageBuffer::Raw)
        {
            srvDesc.ViewDimension         = D3D11_SRV_DIMENSION_BUFFEREX;
            srvDesc.BufferEx.FirstElement = 0;
            srvDesc.BufferEx.NumElements  = numElements;
            srvDesc.BufferEx.Flags        = D3D11_BUFFEREX_SRV_FLAG_RAW;
        }
        else
        {
            srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements  = numElements;
        }

        JAM_CHECK_HRESULT(m_pd3d11Device->CreateShaderResourceView(buffer.pBuffer, &srvDesc, &buffer.pSrv));
    }

    // UAV
    if (_flags & eStorageBufferCreateFlags::ShaderReadWrite)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format                           = format;
        uavDesc.ViewDimension                    = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement              = 0;
        uavDesc.Buffer.NumElements               = numElements;
        uavDesc.Buffer.Flags                     = 0;

        if (_type == eStorageBuffer::Raw)
        {
            uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
        }

        if (_type == eStorageBuffer::Structured && _bAppendConsumer)
        {
            uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
        }

        if (_type == eStorageBuffer::Structured && _bCounter)
        {
            uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_COUNTER;
        }

        JAM_CHECK_HRESULT(m_pd3d11Device->CreateUnorderedAccessView(buffer.pBuffer, &uavDesc, &buffer.pUav));
    }

    return buffer;
}

const HardwareBuffer& Graphics::GetHardwareBufferFromAnyBufferHandle_(
    const AnyBufferHandle _handle) const
{
    switch (_handle.GetType())
    {
        case eBuffer::VertexBuffer:
            return m_vertexBuffers[_handle.AsVertexBufferHandle()];
        case eBuffer::IndexBuffer:
            return m_indexBuffers[_handle.AsIndexBufferHandle()];
        case eBuffer::ConstantBuffer:
            return m_cbuffers[_handle.AsConstantBufferHandle()];
        case eBuffer::StorageBuffer:
            return m_storageBuffers[_handle.AsStorageBufferHandle()];
        default:
            JAM_UNREACHABLE;
    }
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
    desc.Usage               = D3D11_USAGE_STAGING;
    desc.ByteWidth           = _byteWidth;
    desc.BindFlags           = 0;
    desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, nullptr, &m_pUploadBufferCache));
    m_pUploadBufferCacheSize = _byteWidth;

    JAM_LOG_DEBUG("Created upload buffer of size {} bytes", _byteWidth);
    return m_pUploadBufferCache;
}

MutableMemoryView Graphics::BeginUpdateBuffer_(
    const AnyBufferHandle _bh) const
{
    const HardwareBuffer& buffer = GetHardwareBufferFromAnyBufferHandle_(_bh);
    JAM_ASSERT(buffer.bDynamic, "Only dynamic buffers can be updated with BeginUpdateBuffer/EndUpdateBuffer.");
    D3D11_MAPPED_SUBRESOURCE mapped;
    JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(buffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    return { mapped.pData, buffer.byteWidth };
}

void Graphics::EndUpdateBuffer_(
    const AnyBufferHandle _bh) const
{
    const HardwareBuffer& buffer = GetHardwareBufferFromAnyBufferHandle_(_bh);
    JAM_ASSERT(buffer.bDynamic, "Only dynamic buffers can be updated with BeginUpdateBuffer/EndUpdateBuffer.");
    m_pd3d11DeviceContext->Unmap(buffer.pBuffer, 0);
}

void Graphics::UpdateBuffer_(
    const AnyBufferHandle _bh,
    const MemoryView      _uploadData,
    const Int32           _offset,
    const bool            _bDiscard)
{
    const HardwareBuffer& buffer = GetHardwareBufferFromAnyBufferHandle_(_bh);
    JAM_ASSERT(_offset + _uploadData.GetByteWidth() <= buffer.byteWidth, "Buffer update range exceeds buffer size.");

    const Byte* pData     = _uploadData.GetPtr();
    const Int32 byteWidth = static_cast<Int32>(_uploadData.GetByteWidth());

    if (buffer.bDynamic)
    {
        if (_bDiscard)
        {
            D3D11_MAPPED_SUBRESOURCE mapped;
            JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(buffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
            std::memcpy(static_cast<Byte*>(mapped.pData) + _offset, pData, byteWidth);
            m_pd3d11DeviceContext->Unmap(buffer.pBuffer, 0);
        }
        else
        {
            ID3D11Buffer* pUploadBuffer = GetOrCreateUploadBuffer_(byteWidth);

            D3D11_MAPPED_SUBRESOURCE mapped;
            JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(pUploadBuffer, 0, D3D11_MAP_WRITE, 0, &mapped));
            std::memcpy(mapped.pData, _uploadData.GetPtr(), byteWidth);
            m_pd3d11DeviceContext->Unmap(pUploadBuffer, 0);

            D3D11_BOX box;
            box.left   = 0;
            box.right  = byteWidth;
            box.top    = 0;
            box.bottom = 1;
            box.front  = 0;
            box.back   = 1;

            m_pd3d11DeviceContext->CopySubresourceRegion(buffer.pBuffer, 0, _offset, 0, 0, pUploadBuffer, 0, &box);
        }
    }
    else
    {
        JAM_ASSERT(!_bDiscard, "Discard flag cannot be used for non-dynamic buffers.");

        D3D11_BOX box;
        box.left   = _offset;
        box.right  = _offset + byteWidth;
        box.top    = 0;
        box.bottom = 1;
        box.front  = 0;
        box.back   = 1;

        m_pd3d11DeviceContext->UpdateSubresource(buffer.pBuffer, 0, &box, pData, 0, 0);
    }
}

void Graphics::CopyBuffer_(
    const AnyBufferHandle _dstBh,
    const Int32           _dstOffset,
    const AnyBufferHandle _srcBh,
    const Int32           _srcOffset,
    const Int32           _byteWidthOrZero) const
{
    const HardwareBuffer& srcBuffer = GetHardwareBufferFromAnyBufferHandle_(_srcBh);
    const HardwareBuffer& dstBuffer = GetHardwareBufferFromAnyBufferHandle_(_dstBh);

    JAM_ASSERT(!dstBuffer.bImmutable, "Destination buffer must be mutable for CopyBuffer.");

    const Int32 byteWidth = _byteWidthOrZero == 0 ? srcBuffer.byteWidth - _srcOffset : _byteWidthOrZero;
    JAM_ASSERT(_dstOffset + byteWidth <= dstBuffer.byteWidth, "Destination buffer copy range exceeds buffer size.");
    JAM_ASSERT(_srcOffset + byteWidth <= srcBuffer.byteWidth, "Source buffer copy range exceeds buffer size.");

    D3D11_BOX box;
    box.left   = _srcOffset;
    box.right  = _srcOffset + byteWidth;
    box.top    = 0;
    box.bottom = 1;
    box.front  = 0;
    box.back   = 1;
    m_pd3d11DeviceContext->CopySubresourceRegion(dstBuffer.pBuffer, 0, _dstOffset, 0, 0, srcBuffer.pBuffer, 0, &box);
}

HardwareTextureRef Graphics::CreateHardwareTexture_(
    const Int32                      _width,
    const Int32                      _height,
    const Int32                      _depth,
    const eTextureFormat             _format,
    const bool                       _bHasMips,
    const Int32                      _numLayers,
    const eTexture                   _type,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initDataOrEmpty) const
{
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "Texture dimensions must be greater than zero.");
    JAM_ASSERT(_numLayers > 0, "Texture array size must be greater than zero.");
    JAM_ASSERT(_format != eTextureFormat::Unknown, "Texture format must be specified.");

    const bool              bHasInitData = !_initDataOrEmpty.empty();
    const TextureFormatInfo formatInfo   = TextureFormatInfoFrom(_format);

    if (_flags & eTextureCreateFlags::Readback)
    {
        JAM_ASSERT(!(_flags & eTextureCreateFlags::ShaderReadWrite), "Readback texture cannot be a UAV.");
        JAM_ASSERT(!(_flags & eTextureCreateFlags::RenderTargetAttachment), "Readback texture cannot be a render target.");
        JAM_ASSERT(!(_flags & eTextureCreateFlags::DepthStencilAttachment), "Readback texture cannot be a depth stencil.");

        HardwareTextureRef texture = {};
        texture.width              = _width;
        texture.height             = _height;
        texture.depth              = _depth;
        texture.type               = _type;
        texture.format             = _format;
        texture.numLayers          = _numLayers;
        texture.numMips            = 1;
        texture.samples            = 1;
        texture.refCount           = 1;
        texture.bRtvAttachable     = false;
        texture.bDsvAttachable     = false;
        texture.bImmutable         = false;
        texture.bReadback          = true;

        if (_type == eTexture::Texture2D || _type == eTexture::TextureCube)
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width                = static_cast<UINT>(_width);
            desc.Height               = static_cast<UINT>(_height);
            desc.MipLevels            = 1;
            desc.ArraySize            = static_cast<UINT>(_numLayers);
            desc.Format               = formatInfo.texFormat;
            desc.SampleDesc.Count     = 1;
            desc.SampleDesc.Quality   = 0;
            desc.Usage                = D3D11_USAGE_STAGING;
            desc.BindFlags            = 0;
            desc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;
            desc.MiscFlags            = (_type == eTexture::TextureCube) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture2D(&desc, nullptr, &texture.pTexture2D));
        }
        else
        {
            D3D11_TEXTURE3D_DESC desc = {};
            desc.Width                = static_cast<UINT>(_width);
            desc.Height               = static_cast<UINT>(_height);
            desc.Depth                = static_cast<UINT>(_depth);
            desc.MipLevels            = 1;
            desc.Format               = formatInfo.texFormat;
            desc.Usage                = D3D11_USAGE_STAGING;
            desc.BindFlags            = 0;
            desc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture3D(&desc, nullptr, &texture.pTexture3D));
        }
        return texture;
    }

    const Flags<eTextureCreateFlags> msaaFlags               = FilterMsaaFlags(_flags);
    const bool                       bIsMsaa                 = (msaaFlags != eTextureCreateFlags::None);
    const bool                       bRenderTargetAttachment = _flags.IsEnabled(eTextureCreateFlags::RenderTargetAttachment);
    const bool                       bDepthStencilAttachment = _flags.IsEnabled(eTextureCreateFlags::DepthStencilAttachment);
    const bool                       bWriteOnly              = _flags.IsEnabled(eTextureCreateFlags::DisableShaderRead);
    const bool                       bNeedUav                = _flags.IsEnabled(eTextureCreateFlags::ShaderReadWrite);
    const bool                       bNeedSrv                = !bWriteOnly;
    const bool                       bImmutable              = bHasInitData && _flags.IsDisabled(eTextureCreateFlags::Mutable);
    const bool                       bIsCubemap              = (_type == eTexture::TextureCube);
    const Int32                      numMips                 = _bHasMips ? CalcNumMips(_width, _height, _depth) : 1;
    const Int32                      samples                 = (msaaFlags == eTextureCreateFlags::None)    ? 1
                                                             : (msaaFlags == eTextureCreateFlags::MSAAx4)  ? 4
                                                             : (msaaFlags == eTextureCreateFlags::MSAAx8)  ? 8
                                                             : (msaaFlags == eTextureCreateFlags::MSAAx16) ? 16
                                                                                                           : 1;

    JAM_ASSERT(!(bIsCubemap && bIsMsaa), "Cubemap texture cannot be multisampled.");
    JAM_ASSERT(!(_bHasMips && bIsMsaa), "MSAA texture cannot have mipmaps.");

    // format
    const bool        bWriteOnlyDsvTex = bWriteOnly && bDepthStencilAttachment && !bRenderTargetAttachment && !bNeedUav;
    const DXGI_FORMAT dxgiFormat       = bWriteOnlyDsvTex ? formatInfo.depthFormat : formatInfo.texFormat;

    HardwareTextureRef texture = {};
    texture.width              = _width;
    texture.height             = _height;
    texture.depth              = _depth;
    texture.type               = _type;
    texture.format             = _format;
    texture.numLayers          = _numLayers;
    texture.samples            = samples;
    texture.numMips            = numMips;
    texture.bImmutable         = bImmutable;
    texture.refCount           = 1;
    texture.bRtvAttachable     = bRenderTargetAttachment;
    texture.bDsvAttachable     = bDepthStencilAttachment;
    texture.bReadback          = false;

    if (_type == eTexture::Texture2D || _type == eTexture::TextureCube)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width                = static_cast<UINT>(_width);
        desc.Height               = static_cast<UINT>(_height);
        desc.MipLevels            = static_cast<UINT>(numMips);
        desc.ArraySize            = static_cast<UINT>(_numLayers);
        desc.Format               = dxgiFormat;
        desc.SampleDesc.Count     = static_cast<UINT>(samples);
        desc.SampleDesc.Quality   = bIsMsaa ? QueryMaxMsaaQualityLevel_(desc.Format, samples) : 0;
        desc.Usage                = bImmutable ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags       = 0;

        desc.BindFlags = 0;
        if (!bWriteOnly)
        {
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if (bRenderTargetAttachment)
        {
            desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if (bDepthStencilAttachment)
        {
            desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        }
        if (bNeedUav)
        {
            desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            desc.Usage         = D3D11_USAGE_DEFAULT;
            texture.bImmutable = false;   // UAV textures cannot be immutable
        }

        desc.MiscFlags = 0;
        if (bIsCubemap)
        {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }
        if (bRenderTargetAttachment && _bHasMips)
        {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        ID3D11Texture2D* pTexture = nullptr;
        if (bHasInitData)
        {
            JAM_ASSERT(_initDataOrEmpty.size() == _numLayers * numMips, "Texture data count mismatch");

            D3D11_SUBRESOURCE_DATA* pInitData = static_cast<D3D11_SUBRESOURCE_DATA*>(JAM_STACK_ALLOC(sizeof(D3D11_SUBRESOURCE_DATA) * _numLayers * numMips));
            for (Int32 arr = 0; arr < _numLayers; arr++)
            {
                for (Int32 mip = 0; mip < numMips; mip++)
                {
                    const Vec3i      resolution = CalcTextureResolution(_width, _height, 1, mip);
                    const Int32      index      = CalcTextureIndex(mip, arr, numMips);
                    const MemoryView mem        = _initDataOrEmpty[index];
                    const Int32      rowPitch   = resolution.width * BitPerPixelOf(_format) / 8;
                    JAM_ASSERT(mem.GetByteWidth() == rowPitch * resolution.height, "Texture data size mismatch");
                    pInitData[index] = { mem.GetPtr(), static_cast<UInt32>(rowPitch), 0 };
                }
            }

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture2D(&desc, pInitData, &pTexture));
        }
        else
        {
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture2D(&desc, nullptr, &pTexture));
        }

        if (bIsMsaa && bNeedSrv)
        {
            // resolved texture
            desc.SampleDesc.Count   = 1;
            desc.SampleDesc.Quality = 0;
            desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture2D(&desc, nullptr, &texture.pTexture2D));

            texture.pMsaaTexture2D = pTexture;
        }
        else
        {
            texture.pTexture2D = pTexture;
        }
    }
    else
    {
        JAM_ASSERT(_numLayers == 1, "3D texture array size must be 1");
        JAM_ASSERT(!bIsCubemap, "3D texture cannot be a cubemap");
        JAM_ASSERT(!bDepthStencilAttachment, "3D texture cannot be a Depth/Stencil view.");
        JAM_ASSERT(!bIsMsaa, "3D texture cannot be multisampled.");

        D3D11_TEXTURE3D_DESC desc = {};
        desc.Width                = static_cast<UINT>(_width);
        desc.Height               = static_cast<UINT>(_height);
        desc.Depth                = static_cast<UINT>(_depth);
        desc.MipLevels            = static_cast<UINT>(numMips);
        desc.Format               = dxgiFormat;
        desc.Usage                = bImmutable ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags       = 0;

        desc.BindFlags = 0;
        if (!bWriteOnly)
        {
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        if (bRenderTargetAttachment)
        {
            desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if (bDepthStencilAttachment)
        {
            desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        }
        if (bNeedUav)
        {
            desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            desc.Usage         = D3D11_USAGE_DEFAULT;
            texture.bImmutable = false;
        }

        desc.MiscFlags = 0;
        if (bRenderTargetAttachment && _bHasMips)
        {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        if (bHasInitData)
        {
            JAM_ASSERT(_initDataOrEmpty.size() == numMips, "Texture data count mismatch");

            D3D11_SUBRESOURCE_DATA* pInitData = static_cast<D3D11_SUBRESOURCE_DATA*>(JAM_STACK_ALLOC(sizeof(D3D11_SUBRESOURCE_DATA) * numMips));
            for (Int32 index = 0; index < numMips; index++)
            {
                const Vec3i      resolution = CalcTextureResolution(_width, _height, _depth, index);
                const MemoryView mem        = _initDataOrEmpty[index];
                const Int32      rowPitch   = resolution.width * BitPerPixelOf(_format) / 8;
                const Int32      slicePitch = rowPitch * resolution.height;
                JAM_ASSERT(mem.GetByteWidth() == slicePitch * resolution.depth, "Texture data size mismatch");
                pInitData[index] = { mem.GetPtr(), static_cast<UInt32>(rowPitch), static_cast<UInt32>(slicePitch) };
            }

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture3D(&desc, pInitData, &texture.pTexture3D));
        }
        else
        {
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture3D(&desc, nullptr, &texture.pTexture3D));
        }
    }

    if (bNeedSrv)
    {
        const bool bIsArray = _numLayers > 1;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = TextureFormatInfoFrom(_format).colorFormat;
        if (_type == eTexture::Texture2D || _type == eTexture::TextureCube)
        {
            if (bIsCubemap)
            {
                JAM_ASSERT(_numLayers % 6 == 0, "Cubemap array size must be a multiple of 6.");

                if (_numLayers > 6)
                {
                    srvDesc.ViewDimension                     = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                    srvDesc.TextureCubeArray.MostDetailedMip  = static_cast<UINT>(numMips);
                    srvDesc.TextureCubeArray.MipLevels        = 1;
                    srvDesc.TextureCubeArray.First2DArrayFace = 0;
                    srvDesc.TextureCubeArray.NumCubes         = static_cast<UINT>(_numLayers / 6);
                }
                else
                {
                    srvDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
                    srvDesc.TextureCube.MostDetailedMip = static_cast<UINT>(numMips);
                    srvDesc.TextureCube.MipLevels       = 1;
                }
            }
            else
            {
                if (bIsArray)
                {
                    srvDesc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    srvDesc.Texture2DArray.MostDetailedMip = static_cast<UINT>(numMips);
                    srvDesc.Texture2DArray.MipLevels       = 1;
                    srvDesc.Texture2DArray.FirstArraySlice = 0;
                    srvDesc.Texture2DArray.ArraySize       = static_cast<UINT>(_numLayers);
                }
                else
                {
                    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MostDetailedMip = static_cast<UINT>(numMips);
                    srvDesc.Texture2D.MipLevels       = 1;
                }
            }
        }
        else
        {
            srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MostDetailedMip = static_cast<UINT>(numMips);
            srvDesc.Texture3D.MipLevels       = 1;
        }

        JAM_CHECK_HRESULT(m_pd3d11Device->CreateShaderResourceView(texture.pResource, &srvDesc, &texture.pSrv));
    }

    if (bNeedUav)
    {
        JAM_CHECK_HRESULT(m_pd3d11Device->CreateUnorderedAccessView(texture.pResource, nullptr, &texture.pUav));
    }

    return texture;
}

void Graphics::UpdateTexture_(
    const TextureHandle _dstTexh,
    const Int32         _dstMipIndex,
    const Int32         _dstArrayIndex,
    const Int32         _offsetX,
    const Int32         _offsetY,
    const Int32         _offsetZ,
    const Int32         _width,
    const Int32         _height,
    const Int32         _depth,
    const MemoryView    _uploadData,
    const Int32         _rowPitch,
    const Int32         _slicePitch) const
{
    const HardwareTextureRef& dstTexture = m_textures[_dstTexh];
    JAM_ASSERT(dstTexture.pResource, "Destination texture resource is null!");
    JAM_ASSERT(!dstTexture.bImmutable, "Destination texture must be mutable for UpdateTexture.");
    JAM_ASSERT(_dstMipIndex >= 0 && _dstMipIndex < dstTexture.numMips, "Destination mip index is out of range.");
    JAM_ASSERT(_dstArrayIndex >= 0 && _dstArrayIndex < dstTexture.numLayers, "Destination array index is out of range.");

    if (JAM_DEBUG_ENABLED)
    {
        const Vec3i resolution = CalcTextureResolution(dstTexture.width, dstTexture.height, dstTexture.depth, _dstMipIndex);
        JAM_ASSERT(_offsetX >= 0 && _offsetX + _width <= resolution.width, "Update region exceeds destination mip width.");
        JAM_ASSERT(_offsetY >= 0 && _offsetY + _height <= resolution.height, "Update region exceeds destination mip height.");
        JAM_ASSERT(_offsetZ >= 0 && _offsetZ + _depth <= resolution.depth, "Update region exceeds destination mip depth.");
        JAM_ASSERT(_width > 0, "Update region width must be positive.");
        JAM_ASSERT(_height > 0, "Update region height must be positive.");
        JAM_ASSERT(_depth > 0, "Update region depth must be positive.");
    }

    D3D11_BOX box;
    box.left   = _offsetX;
    box.top    = _offsetY;
    box.front  = _offsetZ;
    box.right  = _offsetX + _width;
    box.bottom = _offsetY + _height;
    box.back   = _offsetZ + _depth;

    const UINT index = CalcTextureIndex(_dstMipIndex, _dstArrayIndex, dstTexture.numMips);
    m_pd3d11DeviceContext->UpdateSubresource(
        dstTexture.pResource,
        index,
        &box,
        _uploadData.GetPtr(),
        _rowPitch,
        _slicePitch);
}
Int32 Graphics::ReadTexture_(
    const TextureHandle     _texh,
    const Int32             _mipIndex,
    const Int32             _layerIndex,
    const MutableMemoryView _dst) const
{
    const HardwareTextureRef& texture = m_textures[_texh];
    JAM_ASSERT(texture.bReadback, "Texture must be created with CpuRead flag to read with ReadTexture.");

    JAM_ASSERT(_mipIndex >= 0 && _mipIndex < texture.numMips, "Mip index is out of range.");
    JAM_ASSERT(_layerIndex >= 0 && _layerIndex < texture.numLayers, "Layer index is out of range.");

    const Vec3i resolution = CalcTextureResolution(texture.width, texture.height, texture.depth, _mipIndex);
    const Int32 index      = CalcTextureIndex(_mipIndex, _layerIndex, texture.numMips);
    const Int32 rowPitch   = resolution.width * BitPerPixelOf(texture.format) / 8;

    D3D11_MAPPED_SUBRESOURCE mapped;
    JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(texture.pResource, index, D3D11_MAP_READ, 0, &mapped));

    Int32       written = 0;
    const Int32 cap     = static_cast<Int32>(_dst.GetByteWidth());
    for (Int32 z = 0; z < resolution.depth; ++z)
    {
        const Byte* pSlice = static_cast<const Byte*>(mapped.pData) + z * mapped.DepthPitch;
        for (Int32 y = 0; y < resolution.height; ++y)
        {
            const Byte* pRow = pSlice + y * mapped.RowPitch;
            if (written + rowPitch > cap)
            {
                const Int32 remain = cap - written;
                if (remain > 0)
                {
                    std::memcpy(_dst.GetPtr() + written, pRow, remain);
                    written += remain;
                }
                goto lb_exit;
            }
            else
            {
                std::memcpy(_dst.GetPtr() + written, pRow, rowPitch);
                written += rowPitch;
            }
        }
    }

lb_exit:
    m_pd3d11DeviceContext->Unmap(texture.pResource, index);
    return written;
}

void Graphics::CopyTexture_(
    const TextureHandle _dstTexh,
    const Int32         _dstMipIndex,
    const Int32         _dstArrayIndex,
    const Int32         _dstOffsetX,
    const Int32         _dstOffsetY,
    const Int32         _dstOffsetZ,
    const TextureHandle _srcTexh,
    const Int32         _srcMipIndex,
    const Int32         _srcArrayIndex,
    const Int32         _srcOffsetX,
    const Int32         _srcOffsetY,
    const Int32         _srcOffsetZ,
    const Int32         _srcWidth,
    const Int32         _srcHeight,
    const Int32         _srcDepth) const
{
    const HardwareTextureRef& srcTexture = m_textures[_srcTexh];
    const HardwareTextureRef& dstTexture = m_textures[_dstTexh];
    JAM_ASSERT(!dstTexture.bImmutable, "Destination texture must be mutable for CopyTexture.");
    JAM_ASSERT(srcTexture.pResource, "Source texture resource is null.");
    JAM_ASSERT(dstTexture.pResource, "Destination texture resource is null.");
    JAM_ASSERT(_srcMipIndex >= 0 && _srcMipIndex < srcTexture.numMips, "Source mip index is out of range.");
    JAM_ASSERT(_srcArrayIndex >= 0 && _srcArrayIndex < srcTexture.numLayers, "Source array index is out of range.");
    JAM_ASSERT(_dstMipIndex >= 0 && _dstMipIndex < dstTexture.numMips, "Destination mip index is out of range.");
    JAM_ASSERT(_dstArrayIndex >= 0 && _dstArrayIndex < dstTexture.numLayers, "Destination array index is out of range.");

    D3D11_BOX box;
    box.left   = _srcOffsetX;
    box.top    = _srcOffsetY;
    box.front  = _srcOffsetZ;
    box.right  = _srcOffsetX + _srcWidth;
    box.bottom = _srcOffsetY + _srcHeight;
    box.back   = _srcOffsetZ + _srcDepth;

    if (JAM_DEBUG_ENABLED)
    {
        const Vec3i srcResolution = CalcTextureResolution(srcTexture.width, srcTexture.height, srcTexture.depth, _srcMipIndex);
        const Vec3i dstResolution = CalcTextureResolution(dstTexture.width, dstTexture.height, dstTexture.depth, _dstMipIndex);
        JAM_ASSERT(_srcOffsetX >= 0 && _srcOffsetX + _srcWidth <= srcResolution.width, "Source copy region exceeds source mip level dimensions.");
        JAM_ASSERT(_srcOffsetY >= 0 && _srcOffsetY + _srcHeight <= srcResolution.height, "Source copy region exceeds source mip level dimensions.");
        JAM_ASSERT(_srcOffsetZ >= 0 && _srcOffsetZ + _srcDepth <= srcResolution.depth, "Source copy region exceeds source mip level dimensions.");
        JAM_ASSERT(_dstOffsetX >= 0 && _dstOffsetX + _srcWidth <= dstResolution.width, "Destination copy region exceeds destination mip level dimensions.");
        JAM_ASSERT(_dstOffsetY >= 0 && _dstOffsetY + _srcHeight <= dstResolution.height, "Destination copy region exceeds destination mip level dimensions.");
        JAM_ASSERT(_dstOffsetZ >= 0 && _dstOffsetZ + _srcDepth <= dstResolution.depth, "Destination copy region exceeds destination mip level dimensions.");
        JAM_ASSERT(srcTexture.format == dstTexture.format, "Source and destination texture formats must match for CopyTexture.");
    }

    const UINT srcIndex = CalcTextureIndex(_srcMipIndex, _srcArrayIndex, srcTexture.numMips);
    const UINT dstIndex = CalcTextureIndex(_dstMipIndex, _dstArrayIndex, dstTexture.numMips);

    m_pd3d11DeviceContext->CopySubresourceRegion(
        dstTexture.pResource,
        dstIndex,
        _dstOffsetX,
        _dstOffsetY,
        _dstOffsetZ,
        srcTexture.pResource,
        srcIndex,
        &box);
}

FrameBuffer Graphics::CreateFrameBuffer_(
    const Span<const Attachment> _attachments,
    const bool                   _bDestroyTexture)
{
    JAM_ASSERT(!_attachments.empty(), "Framebuffer must have at least one attachment.");

    FrameBuffer fb = {};

    Int32 numRenderTargets = 0;
    bool  bHasDepthStencil = false;

    bool  bIsMsaaFb           = false;
    bool  bIsCubemapFb        = false;
    bool  bFbFlagsInitialized = false;
    Int32 numLayersFb         = 0;

    for (const Attachment attachment: _attachments)
    {
        JAM_ASSERT(attachment.texh, "Invalid texture handle in render target m_textures.");

        HardwareTextureRef& texture = m_textures[attachment.texh];
        JAM_ASSERT(texture.pResource, "Texture resource is null.");
        JAM_ASSERT(!_bDestroyTexture || texture.refCount == 1, "Cannot transfer ownership of a texture that is referenced elsewhere.");

        const bool bIsMsaa    = texture.samples > 1;
        const bool bIsCubemap = texture.type == eTexture::TextureCube;

        if (bFbFlagsInitialized)
        {
            JAM_ASSERT(bIsMsaaFb == bIsMsaa, "All attachments in a framebuffer must have the same MSAA setting.");
            JAM_ASSERT(bIsCubemapFb == bIsCubemap, "All attachments in a framebuffer must be either cubemap textures or non-cubemap textures.");
        }
        else
        {
            bIsMsaaFb           = bIsMsaa;
            bIsCubemapFb        = bIsCubemap;
            bFbFlagsInitialized = true;
        }

        JAM_ASSERT(numLayersFb == 0 || numLayersFb == attachment.numLayers, "All attachments in a framebuffer must have the same number of array slices.");
        JAM_ASSERT(texture.numLayers >= attachment.layerOffset + attachment.numLayers, "Attachment array slice range exceeds texture array size.");
        JAM_ASSERT(attachment.numLayers >= 0, "Number of array slices in attachment must be non-negative.");

        const bool bIsArray = attachment.numLayers > 1;
        numLayersFb         = attachment.numLayers;

        // 텍스처 참조 관리
        if (!_bDestroyTexture)
        {
            ++texture.refCount;
        }

        if (IsDepthFormat(texture.format))
        {
            JAM_ASSERT(!bHasDepthStencil, "Only one depth stencil texture can be attached to a framebuffer.");
            JAM_ASSERT(texture.bDsvAttachable, "Depth stencil texture must be created with Depth/Stencil flag.");
            JAM_ASSERT(texture.type != eTexture::Texture3D, "3D texture depth stencil view is not supported.");

            D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
            desc.Format                        = TextureFormatInfoFrom(texture.format).depthFormat;

            if (bIsArray)
            {
                if (bIsMsaa)
                {
                    desc.ViewDimension                    = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    desc.Texture2DMSArray.FirstArraySlice = attachment.layerOffset;
                    desc.Texture2DMSArray.ArraySize       = attachment.numLayers;
                }
                else
                {
                    desc.ViewDimension                  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.ArraySize       = attachment.numLayers;
                    desc.Texture2DArray.FirstArraySlice = attachment.layerOffset;
                    desc.Texture2DArray.MipSlice        = attachment.mipIndex;
                }
            }
            else
            {
                if (bIsMsaa)
                {
                    desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MipSlice = attachment.mipIndex;
                }
            }

            if (bIsMsaa)
            {
                JAM_CHECK_HRESULT(m_pd3d11Device->CreateDepthStencilView(texture.pMsaaResource, &desc, &fb.pDsv));
            }
            else
            {
                JAM_CHECK_HRESULT(m_pd3d11Device->CreateDepthStencilView(texture.pResource, &desc, &fb.pDsv));
            }

            fb.attachments[kDepthStencilAttachmentIndex] = attachment;
            bHasDepthStencil                            = true;
        }
        else   // 렌더 타겟
        {
            JAM_ASSERT(numRenderTargets < kMaxNumRenderTargetViewSlots, "Exceeded maximum render target count.");
            JAM_ASSERT(texture.bRtvAttachable, "Render target texture must be created with RenderTarget flag.");
            JAM_ASSERT(!bIsCubemap, "Cubemap render target view is not supported.");

            D3D11_RENDER_TARGET_VIEW_DESC desc = {};
            desc.Format                        = TextureFormatInfoFrom(texture.format).texFormat;

            if (texture.type == eTexture::Texture2D || texture.type == eTexture::TextureCube)
            {
                if (bIsArray)
                {
                    if (bIsMsaa)
                    {
                        desc.ViewDimension                    = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                        desc.Texture2DMSArray.FirstArraySlice = attachment.layerOffset;
                        desc.Texture2DMSArray.ArraySize       = attachment.numLayers;
                    }
                    else
                    {
                        desc.ViewDimension                  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                        desc.Texture2DArray.ArraySize       = attachment.numLayers;
                        desc.Texture2DArray.FirstArraySlice = attachment.layerOffset;
                        desc.Texture2DArray.MipSlice        = attachment.mipIndex;
                    }
                }
                else
                {
                    if (bIsMsaa)
                    {
                        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        desc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
                        desc.Texture2D.MipSlice = attachment.mipIndex;
                    }
                }
            }
            else
            {
                desc.ViewDimension         = D3D11_RTV_DIMENSION_TEXTURE3D;
                desc.Texture3D.MipSlice    = attachment.mipIndex;
                desc.Texture3D.FirstWSlice = 0;
                desc.Texture3D.WSize       = Limits<UInt32>::kHighest;
            }

            if (bIsMsaa)
            {
                JAM_CHECK_HRESULT(m_pd3d11Device->CreateRenderTargetView(texture.pMsaaResource, &desc, &fb.pRtvs[numRenderTargets]));
            }
            else
            {
                JAM_CHECK_HRESULT(m_pd3d11Device->CreateRenderTargetView(texture.pResource, &desc, &fb.pRtvs[numRenderTargets]));
            }

            fb.attachments[numRenderTargets] = attachment;
            ++numRenderTargets;
        }
    }

    fb.numRenderTargets = numRenderTargets;
    fb.bHasDepthStencil = bHasDepthStencil;
    fb.bMsaa            = bIsMsaaFb;
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
    shader.hash      = key;

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
    const UInt64 hash = _vl.GetHash();

    // check cache
    const auto it = m_vlCache.find(hash);
    if (it != m_vlCache.end())
    {
        const VertexLayoutHandle vlh = it->second;
        return vlh;
    }

    // allocate handle and initialize resource
    const VertexLayoutHandle vlh = m_vertexLayouts.Alloc();
    VertexLayoutRef&         vl  = m_vertexLayouts[vlh];
    vl.hash                      = hash;
    vl.vl                        = _vl;
    m_vlCache[hash]              = vlh;
    return vlh;
}

ID3D11InputLayout* Graphics::GetOrCreateD3D11InputLayoutFromCurrentBind_()
{
    Murmur3Hasher64 hasher {};

    for (Int32 i = 0; i < m_vbBindings.numBuffers; ++i)
    {
        const VertexLayoutHandle vlh = m_vbBindings.vlhs[i];
        hasher.Mix(AsMemoryView(vlh));
    }

    Int32 numInstanceData = 0;
    if (m_instanceBinding.vbh)
    {
        numInstanceData = m_instanceBinding.stride / 16;
        hasher.Mix(AsMemoryView(numInstanceData));
    }

    const UInt64 hash = hasher.Finish();   // vertex layout + num instance data
    const auto   it   = m_d3d11InputLayoutCache.find(hash);
    if (it != m_d3d11InputLayoutCache.end())
    {
        return it->second;
    }

    Array<D3D11_INPUT_ELEMENT_DESC, kMaxNumVertexShaderInputSlots> descs;
    Int32                                                          numDescs = 0;

    String hlsl = "struct VSInput {";
    for (Int32 i = 0; i < m_vbBindings.numBuffers; ++i)
    {
        const VertexBufferHandle vbh = m_vbBindings.vbhs[i];
        const VertexLayoutHandle vlh = m_vertexBuffers[vbh].vlh;
        const VertexLayoutRef&   vl  = m_vertexLayouts[vlh];
        for (const VertexLayoutElement& vle: vl.vl.GetElems())
        {
            const HLSLDataTypeInfo& dataType = HLSLDataTypeInfoFrom_(vle.format);
            const HLSLSemanticInfo& semantic = HLSLSemanticInfoFrom_(vle.attrib);
            hlsl.AppendFormat("{} Element{} : {}{};", dataType.name, numDescs, semantic.name, semantic.index);

            D3D11_INPUT_ELEMENT_DESC& desc = descs[numDescs];
            desc.SemanticName              = semantic.name.GetPtr();
            desc.SemanticIndex             = semantic.index;
            desc.Format                    = dataType.format;
            desc.InputSlot                 = i;
            desc.AlignedByteOffset         = D3D11_APPEND_ALIGNED_ELEMENT;
            desc.InputSlotClass            = D3D11_INPUT_PER_VERTEX_DATA;
            desc.InstanceDataStepRate      = 0;
            ++numDescs;
        }
    }

    if (m_instanceBinding.vbh)
    {
        for (Int32 i = 0; i < numInstanceData; ++i)
        {
            hlsl.AppendFormat("float4 Instance{} : INSTANCE{};", i, i);

            D3D11_INPUT_ELEMENT_DESC& desc = descs[numDescs];
            desc.SemanticName              = "INSTANCE";
            desc.SemanticIndex             = i;
            desc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
            desc.InputSlot                 = kMaxNumVertexBufferSlots;
            desc.AlignedByteOffset         = D3D11_APPEND_ALIGNED_ELEMENT;
            desc.InputSlotClass            = D3D11_INPUT_PER_INSTANCE_DATA;
            desc.InstanceDataStepRate      = 1;
            ++numDescs;
        }
    }
    hlsl.Append("}; void VSMain(in VSInput _input) {}");

    // null input layout
    if (numDescs == 0)
    {
        m_d3d11InputLayoutCache[hash] = nullptr;
        if (JAM_DEBUG_ENABLED)
        {
            JAM_LOG_DEBUG("Created null input layout. Hash: {:016X}", hash);
        }
        return nullptr;
    }

    // compilte dummy shader
    constexpr ShaderCompilerInfo info     = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::ReleaseDefault };
    const ShaderCompiler         compiler = ShaderCompiler::CompileHLSLSource(hlsl, info);
    JAM_ASSERT(compiler.IsSucceeded(), "Failed to compile shader for input layout creation. Error: {}", compiler.GetErrorMessage());
    const MemoryView bytecode = compiler.GetBytecode();

    // create input layout
    ID3D11InputLayout* pd3d11InputLayout = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateInputLayout(
        descs.data(),
        numDescs,
        bytecode.GetPtr(),
        bytecode.GetByteWidth(),
        &pd3d11InputLayout));

    m_d3d11InputLayoutCache[hash] = pd3d11InputLayout;

    if (JAM_DEBUG_ENABLED)
    {
        JAM_LOG_DEBUG("Created input layout with {} elements. Hash: {:016X}", numDescs, hash);
        for (Int32 i = 0; i < numDescs; ++i)
        {
            const D3D11_INPUT_ELEMENT_DESC& desc = descs[i];
            JAM_LOG_DEBUG("Element {}: Semantic: {}{} Format: {} InputSlot: {} AlignedByteOffset: {} InputSlotClass: {} InstanceDataStepRate: {}", i, desc.SemanticName, desc.SemanticIndex, NameOf(desc.Format), desc.InputSlot, desc.AlignedByteOffset, NameOf(desc.InputSlotClass), desc.InstanceDataStepRate);
        }
    }

    return pd3d11InputLayout;
}

ID3D11RasterizerState* Graphics::GetOrCreateD3D11RasterizerStateFromCurrentBind_()
{
    const Flags<eRenderFlags> flags = FilterRasterizerFlags(m_renderFlagsBinding);

    Murmur3Hasher32 hasher {};
    hasher.Mix(AsMemoryView(flags));
    const UInt32 key = hasher.Finish();
    const auto   it  = m_d3d11RasterizerStateCache.find(key);
    if (it != m_d3d11RasterizerStateCache.end())
    {
        return it->second;
    }

    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode              = flags & eRenderFlags::Wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;

    const Flags<eRenderFlags> cull = FilterCullModeFlags(m_renderFlagsBinding);
    desc.CullMode                  = cull == eRenderFlags::CullMode_Front ? D3D11_CULL_FRONT
                                   : cull == eRenderFlags::CullMode_Back  ? D3D11_CULL_BACK
                                                                          : D3D11_CULL_NONE;

    desc.FrontCounterClockwise = flags.IsEnabled(eRenderFlags::CCWFront);
    desc.DepthBias             = 0;
    desc.DepthBiasClamp        = 0.f;
    desc.SlopeScaledDepthBias  = 0.f;
    desc.DepthClipEnable       = !(flags & eRenderFlags::DepthClamp);
    desc.MultisampleEnable     = flags.IsEnabled(eRenderFlags::MultiSample);
    desc.AntialiasedLineEnable = flags.IsEnabled(eRenderFlags::LineAA);
    desc.ScissorEnable         = flags.IsEnabled(eRenderFlags::Scissor);

    ID3D11RasterizerState* pRasterizerState = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateRasterizerState(&desc, &pRasterizerState));
    m_d3d11RasterizerStateCache[key] = pRasterizerState;
    JAM_LOG_DEBUG("Created rasterizer state: FillMode: {} CullMode: {} FrontCounterClockwise: {} DepthClipEnable: {} MultisampleEnable: {} AntialiasedLineEnable: {} ScissorEnable: {}", NameOf(desc.FillMode), NameOf(desc.CullMode), desc.FrontCounterClockwise, desc.DepthClipEnable, desc.MultisampleEnable, desc.AntialiasedLineEnable, desc.ScissorEnable);
    return pRasterizerState;
}

ID3D11BlendState* Graphics::GetOrCreateD3D11BlendStateFromCurrentBind_()
{
    const Flags<eRenderFlags> flags = FilterBlendFlags(m_renderFlagsBinding);

    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(flags));
    hasher.Mix(m_blendFlagsBindings);
    const UInt64 hash = hasher.Finish();
    const auto   it   = m_d3d11BlendStateCache.find(hash);
    if (it != m_d3d11BlendStateCache.end())
    {
        return it->second;
    }

    D3D11_BLEND_DESC desc       = {};
    desc.AlphaToCoverageEnable  = flags.IsEnabled(eRenderFlags::AlphaToCoverage);
    desc.IndependentBlendEnable = flags.IsEnabled(eRenderFlags::IndependentBlend);
    const Int32 numRts          = desc.IndependentBlendEnable ? kMaxNumRenderTargetViewSlots : 1;
    for (Int32 i = 0; i < numRts; ++i)
    {
        Flags<eBlendFlags>              rtBlend = m_blendFlagsBindings[i];
        D3D11_RENDER_TARGET_BLEND_DESC& rtDesc  = desc.RenderTarget[i];

        rtDesc.BlendEnable    = rtBlend.IsEnabled(eBlendFlags::Enable);
        rtDesc.SrcBlend       = D3D11BlendFrom_(ResolveSrcBlendFlags(rtBlend));
        rtDesc.DestBlend      = D3D11BlendFrom_(ResolveDstBlendFlags(rtBlend));
        rtDesc.BlendOp        = D3D11BlendOpFrom_(ResolveBlendOpFlags(rtBlend));
        rtDesc.SrcBlendAlpha  = D3D11BlendFrom_(ResolveSrcBlendAlphaFlags(rtBlend));
        rtDesc.DestBlendAlpha = D3D11BlendFrom_(ResolveDstBlendAlphaFlags(rtBlend));
        rtDesc.BlendOpAlpha   = D3D11BlendOpFrom_(ResolveBlendOpAlphaFlags(rtBlend));

        rtDesc.RenderTargetWriteMask = 0;
        if (rtBlend & eBlendFlags::WriteMask_R)
        {
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
        }

        if (rtBlend & eBlendFlags::WriteMask_G)
        {
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
        }

        if (rtBlend & eBlendFlags::WriteMask_B)
        {
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
        }

        if (rtBlend & eBlendFlags::WriteMask_A)
        {
            rtDesc.RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
        }
    }

    ID3D11BlendState* pd3d11BlendState = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateBlendState(&desc, &pd3d11BlendState));
    m_d3d11BlendStateCache[hash] = pd3d11BlendState;

    if (JAM_DEBUG_ENABLED)
    {
        JAM_LOG_DEBUG("Created blend state: AlphaToCoverageEnable: {} IndependentBlendEnable: {}", desc.AlphaToCoverageEnable, desc.IndependentBlendEnable);
        for (Int32 i = 0; i < numRts; ++i)
        {
            const D3D11_RENDER_TARGET_BLEND_DESC& rtDesc = desc.RenderTarget[i];

            JAM_LOG_DEBUG("RenderTarget[{}]:\n"
                          "\tBlendEnable: {}\n"
                          "\tSrcBlend: {}\n"
                          "\tDestBlend: {}\n"
                          "\tBlendOp: {}\n"
                          "\tSrcBlendAlpha: {}\n"
                          "\tDestBlendAlpha: {}\n"
                          "\tBlendOpAlpha: {}\n"
                          "\tRenderTargetWriteMask: {}\n",
                          i,
                          rtDesc.BlendEnable,
                          NameOf(rtDesc.SrcBlend),
                          NameOf(rtDesc.DestBlend),
                          NameOf(rtDesc.BlendOp),
                          NameOf(rtDesc.SrcBlendAlpha),
                          NameOf(rtDesc.DestBlendAlpha),
                          NameOf(rtDesc.BlendOpAlpha),
                          rtDesc.RenderTargetWriteMask);
        }
    }

    return pd3d11BlendState;
}

ID3D11DepthStencilState* Graphics::GetOrCreateD3D11DepthStencilStateFromCurrentBind_()
{
    const Flags<eRenderFlags> flags = FilterDepthFlags(m_renderFlagsBinding);

    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(flags));
    hasher.Mix(AsMemoryView(m_fstencilBinding));
    hasher.Mix(AsMemoryView(m_bstencilBinding));
    const UInt64 key = hasher.Finish();
    const auto   it  = m_d3d11DepthStencilStateCache.find(key);
    if (it != m_d3d11DepthStencilStateCache.end())
    {
        return it->second;
    }

    D3D11_DEPTH_STENCIL_DESC  desc      = {};
    const Flags<eRenderFlags> depthFunc = FilterDepthFuncFlags(m_renderFlagsBinding);
    desc.DepthEnable                    = (depthFunc != eRenderFlags::None);
    desc.DepthWriteMask                 = (flags & eRenderFlags::DepthWrite) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc                      = D3D11ComparisonFuncFrom_(ResolveDepthFuncFlags(depthFunc));
    desc.StencilEnable                  = (m_fstencilBinding != eStencilFlags::None || m_bstencilBinding != eStencilFlags::None);
    desc.StencilReadMask                = 0xFF;
    desc.StencilWriteMask               = 0xFF;

    // front
    desc.FrontFace.StencilFailOp      = D3D11StencilOpFrom_(ResolveStencilFailOpFlags(m_fstencilBinding));
    desc.FrontFace.StencilDepthFailOp = D3D11StencilOpFrom_(ResolveDepthFailOpFlags(m_fstencilBinding));
    desc.FrontFace.StencilPassOp      = D3D11StencilOpFrom_(ResolvePassOpFlags(m_fstencilBinding));
    desc.FrontFace.StencilFunc        = D3D11ComparisonFuncFrom_(ResolveCompareFuncFlags(m_fstencilBinding));

    // back
    desc.BackFace.StencilFailOp      = D3D11StencilOpFrom_(ResolveStencilFailOpFlags(m_bstencilBinding));
    desc.BackFace.StencilDepthFailOp = D3D11StencilOpFrom_(ResolveDepthFailOpFlags(m_bstencilBinding));
    desc.BackFace.StencilPassOp      = D3D11StencilOpFrom_(ResolvePassOpFlags(m_bstencilBinding));
    desc.BackFace.StencilFunc        = D3D11ComparisonFuncFrom_(ResolveCompareFuncFlags(m_bstencilBinding));

    ID3D11DepthStencilState* pDepthStencilState;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateDepthStencilState(&desc, &pDepthStencilState));
    m_d3d11DepthStencilStateCache[key] = pDepthStencilState;

    JAM_LOG_DEBUG("Created depth stencil state:\n"
                  "\tDepthEnable: {}\n"
                  "\tDepthWriteMask: {}\n"
                  "\tDepthFunc: {}\n"
                  "\tStencilEnable: {}\n"
                  "\tFrontStencilFailOp: {}\n"
                  "\tFrontStencilDepthFailOp: {}\n"
                  "\tFrontStencilPassOp: {}\n"
                  "\tFrontStencilFunc: {}\n"
                  "\tBackStencilFailOp: {}\n"
                  "\tBackStencilDepthFailOp: {}\n"
                  "\tBackStencilPassOp: {}\n"
                  "\tBackStencilFunc: {}\n",
                  desc.DepthEnable,
                  NameOf(desc.DepthWriteMask),
                  NameOf(desc.DepthFunc),
                  desc.StencilEnable,
                  NameOf(desc.FrontFace.StencilFailOp),
                  NameOf(desc.FrontFace.StencilDepthFailOp),
                  NameOf(desc.FrontFace.StencilPassOp),
                  NameOf(desc.FrontFace.StencilFunc),
                  NameOf(desc.BackFace.StencilFailOp),
                  NameOf(desc.BackFace.StencilDepthFailOp),
                  NameOf(desc.BackFace.StencilPassOp),
                  NameOf(desc.BackFace.StencilFunc));

    return pDepthStencilState;
}

ID3D11SamplerState* Graphics::GetOrCreateD3D11SamplerState_(
    const Flags<eSamplerFlags> _state,
    const RGBA                 _rgba)
{
    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(_state));
    hasher.Mix(AsMemoryView(_rgba));
    const UInt64 key = hasher.Finish();

    const auto it = m_d3d11SamplerStateCache.find(key);
    if (it != m_d3d11SamplerStateCache.end())
    {
        return it->second;
    }

    const Flags<eSamplerFlags> compareFunc = FilterCompareFuncFlags(_state);
    const Flags<eSamplerFlags> filter      = FilterFilterFlags(_state);

    const SamplerInfo& info            = SamplerInfoFrom_(ResolveFilterFlags(_state));
    const bool         bIsComparison   = compareFunc != eSamplerFlags::CompareFunc_None;
    const Int32        anisotopicLevel = filter == eSamplerFlags::Filter_Anisotropic4  ? 4
                                       : filter == eSamplerFlags::Filter_Anisotropic8  ? 8
                                       : filter == eSamplerFlags::Filter_Anisotropic16 ? 16
                                                                                       : 1;

    D3D11_SAMPLER_DESC desc = {};
    desc.Filter             = bIsComparison ? info.comparisonFilter : info.filter;
    desc.AddressU           = D3D11TextureAddressModeFrom_(ResolveAddressUFlags(_state));
    desc.AddressV           = D3D11TextureAddressModeFrom_(ResolveAddressVFlags(_state));
    desc.AddressW           = D3D11TextureAddressModeFrom_(ResolveAddressWFlags(_state));
    desc.MipLODBias         = 0.0f;
    desc.MaxAnisotropy      = anisotopicLevel;
    desc.ComparisonFunc     = bIsComparison ? D3D11ComparisonFuncFrom_(ResolveCompareFuncFlags(compareFunc)) : D3D11_COMPARISON_ALWAYS;
    desc.BorderColor[0]     = static_cast<float>(_rgba.r) / 255.0f;
    desc.BorderColor[1]     = static_cast<float>(_rgba.g) / 255.0f;
    desc.BorderColor[2]     = static_cast<float>(_rgba.b) / 255.0f;
    desc.BorderColor[3]     = static_cast<float>(_rgba.a) / 255.0f;
    desc.MinLOD             = -Limits<float>::kMax;
    desc.MaxLOD             = Limits<float>::kMax;

    ID3D11SamplerState* pSamplerState = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateSamplerState(&desc, &pSamplerState));
    m_d3d11SamplerStateCache[key] = pSamplerState;

    JAM_LOG_DEBUG("Created sampler state:\n"
                  "\tFilter: {}\n"
                  "\tAddressU: {}\n"
                  "\tAddressV: {}\n"
                  "\tAddressW: {}\n"
                  "\tMipLODBias: {}\n"
                  "\tMaxAnisotropy: {}\n"
                  "\tComparisonFunc: {}\n"
                  "\tBorderColor: ({}, {}, {}, {})\n"
                  "\tMinLOD: {}\n"
                  "\tMaxLOD: {}\n",
                  NameOf(desc.Filter),
                  NameOf(desc.AddressU),
                  NameOf(desc.AddressV),
                  NameOf(desc.AddressW),
                  desc.MipLODBias,
                  desc.MaxAnisotropy,
                  NameOf(desc.ComparisonFunc),
                  desc.BorderColor[0],
                  desc.BorderColor[1],
                  desc.BorderColor[2],
                  desc.BorderColor[3],
                  desc.MinLOD,
                  desc.MaxLOD);

    return pSamplerState;
}

void Graphics::SetShaderResource_(
    const UInt32          _handleValueOrNull,
    const eShaderResource _type,
    const eShader         _shader,
    const Int32           _slot,
    const eResourceAccess _access)
{
    const bool bIsValid = _handleValueOrNull != kInvalidHandleValue;

    if (_access == eResourceAccess::ReadWrite)
    {
        JAM_ASSERT(_shader == eShader::ComputeShader || _shader == eShader::PixelShader, "Read-write texture can only be bound to compute shader or pixel shader.");
        if (_shader == eShader::ComputeShader)
        {
            ReadWriteBindings& binds = m_csReadWriteBindings;
            Resource&          bind  = binds.resources[_slot];

            if (bind.type != _type || bind.value != _handleValueOrNull)
            {
                if (bIsValid)
                {
                    ID3D11UnorderedAccessView* pUav = _type == eShaderResource::Texture ? m_textures[{ _handleValueOrNull }].pUav : m_storageBuffers[{ _handleValueOrNull }].pUav;
                    JAM_ASSERT(pUav, "Texture does not have an unordered access view.");
                    binds.binder[_slot] = pUav;
                }
                else
                {
                    binds.binder[_slot] = nullptr;
                }

                bind.type        = _type;
                bind.value       = _handleValueOrNull;
                binds.dirtyBegin = Min(binds.dirtyBegin, _slot);
                binds.dirtyEnd   = Max(binds.dirtyEnd, _slot + 1);
                m_dirtyFlags |= ePipelineDirtyFlags::CSUnorderedAccessView;   // set flags
            }
        }
        else
        {
            ReadWriteBindings& binds = m_psReadWriteBindings;
            Resource&          bind  = binds.resources[_slot];

            if (bind.type != _type || bind.value != _handleValueOrNull)
            {
                if (bIsValid)
                {
                    ID3D11UnorderedAccessView* pUav = (_type == eShaderResource::Texture) ? m_textures[{ _handleValueOrNull }].pUav : m_storageBuffers[{ _handleValueOrNull }].pUav;
                    JAM_ASSERT(pUav, "Texture does not have an unordered access view.");
                    binds.binder[_slot] = pUav;
                    binds.useMask |= (1ull << _slot);
                }
                else
                {
                    binds.binder[_slot] = nullptr;
                    binds.useMask &= ~(1ull << _slot);
                }

                bind.type  = _type;
                bind.value = _handleValueOrNull;
                m_dirtyFlags |= ePipelineDirtyFlags::FrameBuffer;
            }
        }
    }
    else
    {
        constexpr EnumArray<eShader, ePipelineDirtyFlags> kFlags = {
            ePipelineDirtyFlags::VSShaderResourceView,
            ePipelineDirtyFlags::PSShaderResourceView,
            ePipelineDirtyFlags::GSShaderResourceView,
            ePipelineDirtyFlags::HSShaderResourceView,
            ePipelineDirtyFlags::DSShaderResourceView,
            ePipelineDirtyFlags::CSShaderResourceView
        };

        ReadBindings& binds = m_readBindings[_shader];
        Resource&     bind  = binds.resources[_slot];

        if (bind.type != _type || bind.value != _handleValueOrNull)
        {
            if (bIsValid)
            {
                ID3D11ShaderResourceView* pSrv = (_type == eShaderResource::Texture) ? m_textures[{ _handleValueOrNull }].pSrv : m_storageBuffers[{ _handleValueOrNull }].pSrv;
                JAM_ASSERT(pSrv, "Texture does not have a shader resource view.");
                binds.binder[_slot] = pSrv;
            }
            else
            {
                binds.binder[_slot] = nullptr;
            }

            bind.type        = _type;
            bind.value       = _handleValueOrNull;
            binds.dirtyBegin = Min(binds.dirtyBegin, _slot);
            binds.dirtyEnd   = Max(binds.dirtyEnd, _slot + 1);
            m_dirtyFlags |= kFlags[_shader];   // set flags
        }
    }
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