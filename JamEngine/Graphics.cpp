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
#include "Profile.h"

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
                return D3D11_COMPARISON_NEVER;
            case 2:
                return D3D11_COMPARISON_LESS;
            case 3:
                return D3D11_COMPARISON_EQUAL;
            case 4:
                return D3D11_COMPARISON_LESS_EQUAL;
            case 5:
                return D3D11_COMPARISON_GREATER;
            case 6:
                return D3D11_COMPARISON_NOT_EQUAL;
            case 7:
                return D3D11_COMPARISON_GREATER_EQUAL;
            case 8:
                return D3D11_COMPARISON_ALWAYS;
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
        const eStorageBufferDataType _dataType)
    {
        switch (_dataType)
        {
            case eStorageBufferDataType::UInt8x1:
                return { DXGI_FORMAT_R8_UINT, 1 };
            case eStorageBufferDataType::UInt8x2:
                return { DXGI_FORMAT_R8G8_UINT, 2 };
            case eStorageBufferDataType::UInt8x4:
                return { DXGI_FORMAT_R8G8B8A8_UINT, 4 };
            case eStorageBufferDataType::Int8x1:
                return { DXGI_FORMAT_R8_SINT, 1 };
            case eStorageBufferDataType::Int8x2:
                return { DXGI_FORMAT_R8G8_SINT, 2 };
            case eStorageBufferDataType::Int8x4:
                return { DXGI_FORMAT_R8G8B8A8_SINT, 4 };
            case eStorageBufferDataType::UInt16x1:
                return { DXGI_FORMAT_R16_UINT, 2 };
            case eStorageBufferDataType::UInt16x2:
                return { DXGI_FORMAT_R16G16_UINT, 4 };
            case eStorageBufferDataType::UInt16x4:
                return { DXGI_FORMAT_R16G16B16A16_UINT, 8 };
            case eStorageBufferDataType::Int16x1:
                return { DXGI_FORMAT_R16_SINT, 2 };
            case eStorageBufferDataType::Int16x2:
                return { DXGI_FORMAT_R16G16_SINT, 4 };
            case eStorageBufferDataType::Int16x4:
                return { DXGI_FORMAT_R16G16B16A16_SINT, 8 };
            case eStorageBufferDataType::Float16x1:
                return { DXGI_FORMAT_R16_FLOAT, 2 };
            case eStorageBufferDataType::Float16x2:
                return { DXGI_FORMAT_R16G16_FLOAT, 4 };
            case eStorageBufferDataType::Float16x4:
                return { DXGI_FORMAT_R16G16B16A16_FLOAT, 8 };
            case eStorageBufferDataType::UInt32x1:
                return { DXGI_FORMAT_R32_UINT, 4 };
            case eStorageBufferDataType::UInt32x2:
                return { DXGI_FORMAT_R32G32_UINT, 8 };
            case eStorageBufferDataType::UInt32x4:
                return { DXGI_FORMAT_R32G32B32A32_UINT, 16 };
            case eStorageBufferDataType::Int32x1:
                return { DXGI_FORMAT_R32_SINT, 4 };
            case eStorageBufferDataType::Int32x2:
                return { DXGI_FORMAT_R32G32_SINT, 8 };
            case eStorageBufferDataType::Int32x4:
                return { DXGI_FORMAT_R32G32B32A32_SINT, 16 };
            case eStorageBufferDataType::Float32x1:
                return { DXGI_FORMAT_R32_FLOAT, 4 };
            case eStorageBufferDataType::Float32x2:
                return { DXGI_FORMAT_R32G32_FLOAT, 8 };
            case eStorageBufferDataType::Float32x4:
                return { DXGI_FORMAT_R32G32B32A32_FLOAT, 16 };
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

void Graphics::Frame(const bool _bVSync)
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

VertexBufferHandle Graphics::CreateInstanceBuffer(
    const Int32 _numInstances,
    const Int32 _instanceStride)
{
    JAM_ASSERT(_numInstances > 0, "Instance count must be greater than 0");
    JAM_ASSERT(_instanceStride > 0 && _instanceStride % 16 == 0, "Instance stride must be a positive multiple of 16 bytes");

    const HardwareBuffer hwBuffer = CreateHardwareBuffer_(_numInstances * _instanceStride, eBufferType::VertexBuffer, {});
    VertexBuffer         vb       = {};
    vb.pBuffer                    = hwBuffer.pBuffer;
    vb.byteWidth                  = hwBuffer.byteWidth;
    vb.bDynamic                   = hwBuffer.bDynamic;
    vb.stride                     = _instanceStride;
    vb.numVerts                   = _numInstances;
    vb.vlh                        = k_nullHandle;   // instance buffer doesn't use vertex layout

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

ConstantBufferHandle Graphics::CreateConstantBuffer(
    const Int32 _byteWidth)
{
    JAM_ASSERT(_byteWidth > 0 && _byteWidth <= 0xFFFF, "Constant buffer size must be between 1 and 65536 bytes");

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

StorageBufferHandle Graphics::CreateRawBuffer(
    const Int32                            _byteWidth,
    const Flags<eStorageBufferCreateFlags> _flags,
    const MemoryView                       _initData)
{
    JAM_ASSERT(_byteWidth > 0, "Storage buffer size must be greater than 0");
    const StorageBufferHandle sbh = m_storageBuffers.Alloc();
    m_storageBuffers[sbh]         = CreateStorageBuffer_(_byteWidth, 0, eStorageBufferType::Raw, {}, _flags, false, false, _initData);
    SetStorageBufferName(sbh, "");
    return sbh;
}

StorageBufferHandle Graphics::CreateStructuredBuffer(
    const Int32                            _numElements,
    const Int32                            _stride,
    const bool                             _bAppendConsume,
    const bool                             _bCounter,
    const Flags<eStorageBufferCreateFlags> _flags)
{
    JAM_ASSERT(_numElements > 0, "Storage buffer element count must be greater than 0");
    JAM_ASSERT(!(_bAppendConsume && _bCounter), "Storage buffer cannot be both append/consume and counter");
    const StorageBufferHandle sbh = m_storageBuffers.Alloc();
    m_storageBuffers[sbh]         = CreateStorageBuffer_(_numElements * _stride, _stride, eStorageBufferType::Structured, {}, _flags, _bAppendConsume, _bCounter, {});
    SetStorageBufferName(sbh, "");
    return sbh;
}

StorageBufferHandle Graphics::CreateTypedBuffer(
    const Int32                            _numElements,
    const eStorageBufferDataType           _dataType,
    const Flags<eStorageBufferCreateFlags> _flags)
{
    JAM_ASSERT(_numElements > 0, "Storage buffer element count must be greater than 0");

    const BufferDataTypeInfo  dataTypeInfo = BufferDataTypeInfoFrom_(_dataType);
    const StorageBufferHandle sbh          = m_storageBuffers.Alloc();
    m_storageBuffers[sbh]                  = CreateStorageBuffer_(_numElements * dataTypeInfo.stride, dataTypeInfo.stride, eStorageBufferType::Typed, _dataType, _flags, false, false, {});
    SetStorageBufferName(sbh, "");
    return sbh;
}

StorageBufferHandle Graphics::CreateReadbackBuffer(
    const Int32 _byteWidth)
{
    JAM_ASSERT(_byteWidth > 0, "Storage buffer size must be greater than 0");
    const StorageBufferHandle sbh = m_storageBuffers.Alloc();
    m_storageBuffers[sbh]         = CreateStorageBuffer_(_byteWidth, 0, eStorageBufferType::Readback, {}, {}, false, false, {});
    SetStorageBufferName(sbh, "");
    return sbh;
}

VertexLayoutHandle Graphics::CreateVertexLayout(
    const VertexLayout& _vl)
{
    return GetOrCreateVertexLayout_(_vl);
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
            initData.emplace_back(img->pixels, img->slicePitch);
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

TextureHandle Graphics::CreateTexture2D(
    const Int32                      _width,
    const Int32                      _height,
    const eTextureFormat             _format,
    const Int32                      _arraySize,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initDataOrEmpty)
{
    const TextureHandle texh = m_textures.Alloc();
    m_textures[texh]         = CreateHardwareTexture_(_width, _height, 1, _format, _arraySize, eTextureType::Texture2D, _flags, _initDataOrEmpty);
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
    m_textures[texh]         = CreateHardwareTexture_(_width, _height, 1, _format, _cubeCount * 6, eTextureType::TextureCube, _flags, _initDataOrEmpty);
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
    m_textures[texh]         = CreateHardwareTexture_(_width, _height, _depth, _format, 1, eTextureType::Texture3D, _texFlags, _initDataOrEmpty);
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

    Flags<eTextureCreateFlags> rtvFlags = eTextureCreateFlags::RenderTargetAttachment;
    if (_writeOnlyRenderTarget)
    {
        rtvFlags |= eTextureCreateFlags::WriteOnly;
    }

    Flags<eTextureCreateFlags> dsvFlags = eTextureCreateFlags::DepthStencilAttachment;
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
        flags |= eTextureCreateFlags::DepthStencilAttachment;
    }
    else
    {
        flags |= eTextureCreateFlags::RenderTargetAttachment;
    }

    const TextureHandle attachments[] = { CreateTexture2D(_width, _height, _format, 1, flags) };
    return CreateFrameBuffer(attachments, true);
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

        m_shaderCache.erase(shader.hash);   // 캐시에서 제거
        m_shaders.Free(_sh);                // Shader 핸들 해제
    }
}

void Graphics::DestroyProgram(
    const ProgramHandle _ph)
{
    Program& program = m_programs[_ph];

    if (program.vsh)
    {
        DestroyShader(program.vsh);   // VS 참조 해제
    }

    if (program.psh)
    {
        DestroyShader(program.psh);   // PS 참조 해제
    }

    if (program.hsh)
    {
        DestroyShader(program.hsh);   // HS 참조 해제
    }

    if (program.dsh)
    {
        DestroyShader(program.dsh);   // DS 참조 해제
    }

    if (program.gsh)
    {
        DestroyShader(program.gsh);   // GS 참조 해제
    }

    if (program.csh)
    {
        DestroyShader(program.csh);   // CS 참조 해제
    }

    m_programs.Free(_ph);   // Program 핸들 해제
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

void Graphics::SetStorageBufferName(
    const StorageBufferHandle _sbh,
    const StringView          _name)
{
    StorageBuffer& sb = m_storageBuffers[_sbh];
    sb.name           = CreateName_("StorageBuffer", _sbh, _name);
    SetD3DDebugName_(sb.pBuffer, sb.name);

    if (sb.pSrv)
    {
        SetD3DDebugName_(sb.pSrv, CreateName_("StorageBuffer_SRV", _sbh, _name));
    }

    if (sb.pUav)
    {
        SetD3DDebugName_(sb.pUav, CreateName_("StorageBuffer_UAV", _sbh, _name));
    }
}

void Graphics::SetTextureName(
    const TextureHandle _texh,
    const StringView    _name)
{
    constexpr EnumArray<eTextureType, StringView> k_texNames = { "Texture2D", "Texture3D", "TextureCube" };
    constexpr EnumArray<eTextureType, StringView> k_srvNames = { "Texture2D_SRV", "Texture3D_SRV", "TextureCube_SRV" };

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
    constexpr StringView k_names[] = {
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
    constexpr EnumArray<eShader, StringView> k_names = {
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

void Graphics::UploadBuffer(
    const VertexBufferHandle _vbh,
    const MemoryView         _writeData,
    const Int32              _offset,
    const bool               _bDiscard)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];
    UpdateBuffer_(vb, _writeData, _offset, _bDiscard);
}

void Graphics::UploadBuffer(
    const IndexBufferHandle _ibh,
    const MemoryView        _writeData,
    const Int32             _offset,
    const bool              _bDiscard)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];
    UpdateBuffer_(ib, _writeData, _offset, _bDiscard);
}

void Graphics::UploadBuffer(
    const ConstantBufferHandle _cbh,
    const MemoryView           _writeData)
{
    const ConstantBuffer& cb = m_cbuffers[_cbh];
    UpdateBuffer_(cb, _writeData, 0, true);
}
MutableMemoryView Graphics::BeginUploadBuffer(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];
    return BeginUpdateBuffer_(vb);
}

MutableMemoryView Graphics::BeginUploadBuffer(
    const IndexBufferHandle _ibh)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];
    return BeginUpdateBuffer_(ib);
}

void Graphics::EndUploadBuffer(
    const VertexBufferHandle _vbh)
{
    const VertexBuffer& vb = m_vertexBuffers[_vbh];
    EndUpdateBuffer_(vb);
}

void Graphics::EndUploadBuffer(
    const IndexBufferHandle _ibh)
{
    const IndexBuffer& ib = m_indexBuffers[_ibh];
    EndUpdateBuffer_(ib);
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
    UpdateTexture_(
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
    UpdateTexture_(
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
    UpdateTexture_(
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

void Graphics::CopyTexture(
    const TextureHandle _dstTexh,
    const TextureHandle _srcTexh)
{
    const HardwareTextureRef& dstTex = m_textures[_dstTexh];
    const HardwareTextureRef& srcTex = m_textures[_srcTexh];
    CopyTexture_(dstTex, 0, 0, 0, 0, 0, srcTex, 0, 0, 0, 0, 0, 0, 0, 0);
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
    const Int32         _srcDepthOrZero)
{
    const HardwareTextureRef& dstTex = m_textures[_dstTexh];
    const HardwareTextureRef& srcTex = m_textures[_srcTexh];
    CopyTexture_(
        dstTex,
        _dstMipIndex,
        _dstArrayIndex,
        _dstX,
        _dstY,
        _dstZ,
        srcTex,
        _srcMipIndex,
        _srcArrayIndex,
        _srcX,
        _srcY,
        _srcZ,
        _srcWidthOrZero,
        _srcHeightOrZero,
        _srcDepthOrZero);
}

ImageContainer Graphics::Capture(
    const TextureHandle _texh)
{
    const HardwareTextureRef& texture   = m_textures[_texh];
    ImageContainer            container = {};
    JAM_CHECK_HRESULT(DirectX::CaptureTexture(m_pd3d11Device, m_pd3d11DeviceContext, texture.pResource, container));
    return container;
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
    pi.vsh         = program.vsh;
    pi.psh         = program.psh;
    pi.hsh         = program.hsh;
    pi.dsh         = program.dsh;
    pi.gsh         = program.gsh;
    pi.csh         = program.csh;
    return pi;
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

Vec2i Graphics::GetBackBufferResolution() const
{
    return m_backBufferRes;
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

ShaderHandle Graphics::GetShader(
    const ProgramHandle _ph,
    const eShader       _stage)
{
    const Program& program = m_programs[_ph];
    switch (_stage)
    {
        case eShader::VertexShader:
            return program.vsh;
        case eShader::PixelShader:
            return program.psh;
        case eShader::HullShader:
            return program.hsh;
        case eShader::DomainShader:
            return program.dsh;
        case eShader::GeometryShader:
            return program.gsh;
        case eShader::ComputeShader:
            return program.csh;
        default:
            JAM_UNREACHABLE;
    }
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
    const Int32              _stream)
{
    if (_vbhOrInvalid)
    {
        const VertexBuffer& vb = m_vertexBuffers[_vbhOrInvalid];
        SetVertexBuffer(_vbhOrInvalid, _stream, vb.numVerts, 0, vb.vlh);
    }
    else
    {
        SetVertexBuffer(_vbhOrInvalid, _stream, 0, 0, k_nullHandle);
    }
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _vbhOrInvalid,
    const Int32              _stream,
    const Int32              _numVerts,
    const Int32              _vertOffset)
{
    if (_vbhOrInvalid)
    {
        VertexBuffer& vb = m_vertexBuffers[_vbhOrInvalid];
        SetVertexBuffer(_vbhOrInvalid, _stream, _numVerts, _vertOffset, vb.vlh);
    }
    else
    {
        SetVertexBuffer(k_nullHandle, _stream, 0, 0, k_nullHandle);
    }
}

void Graphics::SetVertexBuffer(
    const VertexBufferHandle _vbhOrInvalid,
    const Int32              _stream,
    const Int32              _numVerts,
    const Int32              _vertOffset,
    const VertexLayoutHandle _vlh)
{
    StreamBinds::Stream& stream = m_streamBinds.stream[_stream];

    if (_vbhOrInvalid && (_vbhOrInvalid != stream.vbh || _vertOffset != stream.offset))
    {
        JAM_ASSERT(_vlh, "Vertex layout must not be null when binding vertex buffer.");

        const UInt32 bit = 1u << _stream;
        m_streamBinds.useMask |= bit;

        stream.vbh    = _vbhOrInvalid;
        stream.offset = _vertOffset;
        m_dirtyFlags |= ePipelineDirtyFlags::VertexBuffer;
    }
    else if (!_vbhOrInvalid && stream.vbh)
    {
        JAM_ASSERT(!_vlh, "Vertex layout must be null when unbinding vertex buffer.");

        const UInt32 bit = 1u << _stream;
        m_streamBinds.useMask &= ~bit;

        stream.vbh    = k_nullHandle;
        stream.offset = 0;
        m_dirtyFlags |= ePipelineDirtyFlags::VertexBuffer;
    }

    if (stream.vlh != _vlh)
    {
        stream.vlh = _vlh;
        m_dirtyFlags |= ePipelineDirtyFlags::InputLayout;
    }

    m_numVerts = _numVerts;
}

void Graphics::SetInstanceBuffer(
    const VertexBufferHandle _vbhOrInvalid)
{
    if (_vbhOrInvalid)
    {
        const VertexBuffer& vb = m_vertexBuffers[_vbhOrInvalid];
        SetInstanceBuffer(_vbhOrInvalid, vb.numVerts, 0);
    }
    else
    {
        SetInstanceBuffer(_vbhOrInvalid, 0, 0);
    }
}

void Graphics::SetInstanceBuffer(
    const VertexBufferHandle _vbhOrInvalid,
    const Int32              _numInstances,
    const Int32              _instanceOffset)
{
    if (_vbhOrInvalid != m_instanceBind || _instanceOffset != m_instanceOffsetBind)
    {
        m_instanceBind       = _vbhOrInvalid;
        m_instanceOffsetBind = _instanceOffset;
        m_dirtyFlags |= ePipelineDirtyFlags::InstanceBuffer;
    }

    m_numInstancesBind = _numInstances;
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
        binds.cbhs[_slot]   = _cbh;
        binds.binder[_slot] = _cbh ? m_cbuffers[_cbh].pBuffer : nullptr;
        binds.dirtyStart    = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd      = Max(binds.dirtyEnd, _slot);
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
        binds.texhs[_slot]  = _texh;
        binds.binder[_slot] = _texh ? m_textures[_texh].pSrv : nullptr;
        binds.dirtyStart    = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd      = Max(binds.dirtyEnd, _slot);
        m_dirtyFlags |= k_flags[_shader];   // set flags
    }
}

void Graphics::SetSamplerState(
    Flags<eSamplerFlags> _state,
    const eShader        _shader,
    const Int32          _slot)
{
    constexpr EnumArray<eShader, ePipelineDirtyFlags> k_flags = {
        ePipelineDirtyFlags::VSSamplerState,
        ePipelineDirtyFlags::PSSamplerState,
        ePipelineDirtyFlags::GSSamplerState,
        ePipelineDirtyFlags::HSSamplerState,
        ePipelineDirtyFlags::DSSamplerState,
        ePipelineDirtyFlags::CSSamplerState
    };

    SamplerBinds&          binds   = m_samplerBinds[_shader];
    SamplerBinds::Sampler& sampler = binds.samplers[_slot];

    if (sampler.state != _state)
    {
        sampler.state       = _state;
        binds.binder[_slot] = GetOrCreateD3D11SamplerState_(sampler.state, sampler.rgba);
        binds.dirtyStart    = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd      = Max(binds.dirtyEnd, _slot);
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

    SamplerBinds&          binds   = m_samplerBinds[_shader];
    SamplerBinds::Sampler& sampler = binds.samplers[_slot];

    if (_rgba != sampler.rgba)
    {
        sampler.rgba        = _rgba;
        binds.binder[_slot] = GetOrCreateD3D11SamplerState_(sampler.state, sampler.rgba);
        binds.dirtyStart    = Min(binds.dirtyStart, _slot);
        binds.dirtyEnd      = Max(binds.dirtyEnd, _slot);
        m_dirtyFlags |= k_flags[_shader];   // set flags
    }
}

void Graphics::SetRenderFlags(
    const Flags<eRenderFlags> _flags)
{
    if (m_renderFlagsBind != _flags)
    {
        // depth stencil state
        if (FilterDepthFlags(m_renderFlagsBind) != FilterDepthFlags(_flags))
        {
            m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
        }

        // rasterizer state
        if (FilterRasterizerFlags(m_renderFlagsBind) != FilterRasterizerFlags(_flags))
        {
            m_dirtyFlags |= ePipelineDirtyFlags::RasterizerState;
        }

        // blend state
        if (FilterBlendFlags(m_renderFlagsBind) != FilterBlendFlags(_flags))
        {
            m_dirtyFlags |= ePipelineDirtyFlags::BlendState;
        }

        // primitive
        if (FilterPrimitiveFlags(m_renderFlagsBind) != FilterPrimitiveFlags(_flags))
        {
            m_dirtyFlags |= ePipelineDirtyFlags::Primitive;
        }

        m_renderFlagsBind = _flags;
    }
}

void Graphics::SetBlendFlags(
    const Flags<eBlendFlags> _flags,
    const Int32              _renderTargetIndex)
{
    JAM_ASSERT(_renderTargetIndex >= 0 && _renderTargetIndex < k_maxNumRenderTargetViewSlots, "Render target pos out of range.");
    const Flags<eBlendFlags> flags = m_blendFlagsBinds[_renderTargetIndex];
    if (_flags != flags)
    {
        m_blendFlagsBinds[_renderTargetIndex] = _flags;
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

void Graphics::SetStencilFlags(
    const Flags<eStencilFlags> _fStencil,
    const Flags<eStencilFlags> _bStencil,
    const UInt8                _stencilRef)
{
    if (_fStencil != m_fstencilBind)
    {
        m_fstencilBind = _fStencil;
        m_dirtyFlags |= ePipelineDirtyFlags::DepthStencilState;
    }

    if (_bStencil != m_bstencilBind)
    {
        m_bstencilBind = _bStencil;
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

void Graphics::SetComputeProgram(
    const ProgramHandle _ph)
{
    if (m_computePhBind != _ph)
    {
        m_computePhBind = _ph;
        m_dirtyFlags |= ePipelineDirtyFlags::ComputeProgram;
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
    if (m_vpBind != _vp)
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

void Graphics::Submit()
{
    BindPipeline_();

    if (m_ibhBind)
    {
        if (m_instanceBind)
        {
            const Int32 startIdxLoc  = m_submitParams[eSubmitParameterType::StartIndexLocation];
            const Int32 baseVtxLoc   = m_submitParams[eSubmitParameterType::BaseVertexLocation];
            const Int32 startInstLoc = m_submitParams[eSubmitParameterType::StartInstanceLocation];
            m_pd3d11DeviceContext->DrawIndexedInstanced(m_numIndicesBind, m_numInstancesBind, startIdxLoc, baseVtxLoc, startInstLoc);
        }
        else
        {
            const Int32 startIdxLoc = m_submitParams[eSubmitParameterType::StartIndexLocation];
            const Int32 baseVtxLoc  = m_submitParams[eSubmitParameterType::BaseVertexLocation];
            m_pd3d11DeviceContext->DrawIndexed(m_numIndicesBind, startIdxLoc, baseVtxLoc);
        }
    }
    else
    {
        if (m_instanceBind)
        {
            const Int32 startVertLoc = m_submitParams[eSubmitParameterType::StartVertexLocation];
            const Int32 startInstLoc = m_submitParams[eSubmitParameterType::StartInstanceLocation];
            m_pd3d11DeviceContext->DrawInstanced(m_numVerts, m_numInstancesBind, startVertLoc, startInstLoc);
        }
        else
        {
            const Int32 startVertLoc = m_submitParams[eSubmitParameterType::StartVertexLocation];
            m_pd3d11DeviceContext->Draw(m_numVerts, startVertLoc);
        }
    }
}

void Graphics::Dispatch(
    const Int32 _numThreadGroupsX,
    const Int32 _numThreadGroupsY,
    const Int32 _numThreadGroupsZ)
{
    BindPipeline_();
    m_pd3d11DeviceContext->Dispatch(_numThreadGroupsX, _numThreadGroupsY, _numThreadGroupsZ);
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

    for (ID3D11InputLayout* pInputLayout: m_d3d11InputLayoutCache | std::views::values)
    {
        pInputLayout->Release();
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
    ConstantBufferBinds& binds = m_cbufferBinds[_stage];

    // dirty range
    const Int32 dirtyStart = binds.dirtyStart;
    const Int32 numDirties = binds.dirtyEnd - dirtyStart + 1;
    if (numDirties <= 0)
    {
        return;
    }

    ID3D11Buffer** ppBinder = binds.binder.data() + dirtyStart;

    // bind
    switch (_stage)
    {
        case eShader::VertexShader:
            m_pd3d11DeviceContext->VSSetConstantBuffers(dirtyStart, numDirties, ppBinder);
            break;
        case eShader::PixelShader:
            m_pd3d11DeviceContext->PSSetConstantBuffers(dirtyStart, numDirties, ppBinder);
            break;
        case eShader::GeometryShader:
            m_pd3d11DeviceContext->GSSetConstantBuffers(dirtyStart, numDirties, ppBinder);
            break;
        case eShader::ComputeShader:
            m_pd3d11DeviceContext->CSSetConstantBuffers(dirtyStart, numDirties, ppBinder);
            break;
        case eShader::HullShader:
            m_pd3d11DeviceContext->HSSetConstantBuffers(dirtyStart, numDirties, ppBinder);
            break;
        case eShader::DomainShader:
            m_pd3d11DeviceContext->DSSetConstantBuffers(dirtyStart, numDirties, ppBinder);
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
    TextureBinds& binds = m_textureBinds[_shader];

    const Int32 dirtyStart    = binds.dirtyStart;
    const Int32 numDirtySlots = binds.dirtyEnd - dirtyStart + 1;
    if (numDirtySlots <= 0)
    {
        return;
    }

    ID3D11ShaderResourceView** ppBinder = binds.binder.data() + dirtyStart;

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            m_pd3d11DeviceContext->VSSetShaderResources(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::PixelShader:
            m_pd3d11DeviceContext->PSSetShaderResources(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::GeometryShader:
            m_pd3d11DeviceContext->GSSetShaderResources(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::ComputeShader:
            m_pd3d11DeviceContext->CSSetShaderResources(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::HullShader:
            m_pd3d11DeviceContext->HSSetShaderResources(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::DomainShader:
            m_pd3d11DeviceContext->DSSetShaderResources(dirtyStart, numDirtySlots, ppBinder);
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
    SamplerBinds& binds         = m_samplerBinds[_shader];
    const Int32   dirtyStart    = binds.dirtyStart;
    const Int32   numDirtySlots = binds.dirtyEnd - dirtyStart + 1;
    if (numDirtySlots <= 0)
    {
        return;
    }

    ID3D11SamplerState** ppBinder = binds.binder.data() + dirtyStart;

    // bind
    switch (_shader)
    {
        case eShader::VertexShader:
            m_pd3d11DeviceContext->VSSetSamplers(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::PixelShader:
            m_pd3d11DeviceContext->PSSetSamplers(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::GeometryShader:
            m_pd3d11DeviceContext->GSSetSamplers(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::ComputeShader:
            m_pd3d11DeviceContext->CSSetSamplers(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::HullShader:
            m_pd3d11DeviceContext->HSSetSamplers(dirtyStart, numDirtySlots, ppBinder);
            break;
        case eShader::DomainShader:
            m_pd3d11DeviceContext->DSSetSamplers(dirtyStart, numDirtySlots, ppBinder);
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
            Array<ID3D11Buffer*, k_maxNumVertexStream> binder;
            Array<UINT, k_maxNumVertexStream>          strides;
            Array<UINT, k_maxNumVertexStream>          offsets;
            m_numStreamsBound = 0;

            UInt32 mask = m_streamBinds.useMask;
            for (Int32 idx = 0; mask != 0; mask >>= 1, ++idx, ++m_numStreamsBound)
            {
                const Int32 numZeros = std::countr_zero(mask);
                mask >>= numZeros;
                idx += numZeros;

                StreamBinds::Stream& stream         = m_streamBinds.stream[idx];
                VertexBufferHandle&  vbh            = stream.vbh;
                const VertexBuffer&  vb             = m_vertexBuffers[vbh];
                binder[m_numStreamsBound]           = vb.pBuffer;
                strides[m_numStreamsBound]          = vb.stride;
                offsets[m_numStreamsBound]          = stream.offset * vb.stride;
                m_vlhStreamBound[m_numStreamsBound] = vb.vlh;
            }

            if (m_numStreamsBound > 0)
            {
                m_pd3d11DeviceContext->IASetVertexBuffers(0, m_numStreamsBound, binder.data(), strides.data(), offsets.data());
            }
            else
            {
                ID3D11Buffer* pBuffer = nullptr;
                UInt32        stride  = 0;
                UInt32        offset  = 0;
                m_pd3d11DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &stride, &offset);
            }
        }

        // instance buffer
        if (m_dirtyFlags & ePipelineDirtyFlags::InstanceBuffer)
        {
            if (m_instanceBind)
            {
                const VertexBuffer& vb     = m_vertexBuffers[m_instanceBind];
                const UInt32        offset = m_instanceOffsetBind * vb.stride;
                const UInt32        stride = vb.stride;
                m_pd3d11DeviceContext->IASetVertexBuffers(m_numStreamsBound, 1, &vb.pBuffer, &stride, &offset);
            }
            else
            {
                ID3D11Buffer* pBuffer = nullptr;
                UInt32        stride  = 0;
                UInt32        offset  = 0;
                m_pd3d11DeviceContext->IASetVertexBuffers(m_numStreamsBound, 1, &pBuffer, &stride, &offset);
            }
        }

        // index buffer
        if (m_dirtyFlags & ePipelineDirtyFlags::IndexBuffer)
        {
            if (m_ibhBind)
            {
                const IndexBuffer& ib     = m_indexBuffers[m_ibhBind];
                const DXGI_FORMAT  format = ib.bUseU32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
                const UInt32       offset = ib.bUseU32 ? m_ibOffsetBind * 4 : m_ibOffsetBind * 2;
                m_pd3d11DeviceContext->IASetIndexBuffer(ib.pBuffer, format, offset);
            }
            else
            {
                m_pd3d11DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
            }
        }

        // topology
        if (m_dirtyFlags & ePipelineDirtyFlags::Primitive)
        {
            const UInt32 prim = ResolvePrimitiveFlags(m_renderFlagsBind);
            m_pd3d11DeviceContext->IASetPrimitiveTopology(D3D11PrimitiveTopologyFrom_(prim));
        }

        // input vertexLayout
        if (m_dirtyFlags & ePipelineDirtyFlags::InputLayout)
        {
            if (m_numStreamsBound > 0)
            {
                ID3D11InputLayout* pInputLayout = GetOrCreateD3D11InputLayoutFromCurrentBind_();
                m_pd3d11DeviceContext->IASetInputLayout(pInputLayout);
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
        }

        // compute shader program
        if (m_dirtyFlags & ePipelineDirtyFlags::ComputeProgram)
        {
            if (m_computePhBind)
            {
                const Program&   program = m_programs[m_computePhBind];
                const ShaderRef& shader  = m_shaders[program.csh];
                m_pd3d11DeviceContext->CSSetShader(shader.pComputeShader, nullptr, 0);
            }
            else
            {
                m_pd3d11DeviceContext->CSSetShader(nullptr, nullptr, 0);
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

            ID3D11BlendState* pBs = GetOrCreateD3D11BlendStateFromCurrentBind_();
            m_pd3d11DeviceContext->OMSetBlendState(pBs, blendFactor, 0xFFFFFFFF);
        }

        // depth stencil state
        if (m_dirtyFlags & ePipelineDirtyFlags::DepthStencilState)
        {
            ID3D11DepthStencilState* pBss = GetOrCreateD3D11DepthStencilStateFromCurrentBind_();
            m_pd3d11DeviceContext->OMSetDepthStencilState(pBss, m_stencilRefBind);
        }

        // rasterizer state
        if (m_dirtyFlags & ePipelineDirtyFlags::RasterizerState)
        {
            ID3D11RasterizerState* pRs = GetOrCreateD3D11RasterizerStateFromCurrentBind_();
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
    const eTextureType               _type,
    const Flags<eTextureCreateFlags> _flags,
    const Span<const MemoryView>     _initDataOrEmpty) const
{
    JAM_ASSERT(_width > 0 && _height > 0 && _depth > 0, "Texture dimensions must be greater than zero.");
    JAM_ASSERT(_arraySize > 0, "Texture array size must be greater than zero.");
    JAM_ASSERT(_format != eTextureFormat::Unknown, "Texture format must be specified.");

    const Flags<eTextureCreateFlags> msaaFlags               = FilterMsaaFlags(_flags);
    const bool                       bIsMSAA                 = (msaaFlags != eTextureCreateFlags::None);
    const bool                       bGenMips                = (_flags & eTextureCreateFlags::GenerateMips);
    const bool                       bWriteOnly              = (_flags & eTextureCreateFlags::WriteOnly);
    const bool                       bComputeReadWrite       = (_flags & eTextureCreateFlags::ComputeReadWrite);
    const bool                       bRenderTargetAttachment = (_flags & eTextureCreateFlags::RenderTargetAttachment);
    const bool                       bDepthStencilAttachment = (_flags & eTextureCreateFlags::DepthStencilAttachment);
    const bool                       bHasInitData            = !_initDataOrEmpty.empty();
    const bool                       bMutable                = !bHasInitData || (_flags & eTextureCreateFlags::Mutable);
    const bool                       bIsCubemap              = (_type == eTextureType::TextureCube);
    const bool                       bReadback               = (_flags & eTextureCreateFlags::Readback);
    const bool                       bNeedSrv                = (!bWriteOnly && !bReadback);
    const bool                       bNeedUav                = (bComputeReadWrite && !bReadback);
    const Int32                      numMips                 = bGenMips ? CalcNumMips(_width, _height, _depth) : 1;
    const Int32                      samples                 = (msaaFlags == eTextureCreateFlags::None)    ? 1
                                                             : (msaaFlags == eTextureCreateFlags::MSAAx4)  ? 4
                                                             : (msaaFlags == eTextureCreateFlags::MSAAx8)  ? 8
                                                             : (msaaFlags == eTextureCreateFlags::MSAAx16) ? 16
                                                                                                           : 1;

    JAM_ASSERT(!bIsMSAA || !(bGenMips || bComputeReadWrite || bReadback), "MSAA texture cannot have mipmaps, be a compute read/write target, or be a readback target.");
    JAM_ASSERT(!(bRenderTargetAttachment && bDepthStencilAttachment), "Texture cannot be both render target and depth/stencil target.");
    if (JAM_DEBUG_ENABLED && bReadback)
    {
        JAM_ASSERT(!bComputeReadWrite, "Readback texture cannot be a compute read/write target.");
        JAM_ASSERT(!bIsMSAA, "Readback texture cannot be MSAA.");
        JAM_ASSERT(!bGenMips, "Readback texture cannot have mipmaps.");
        JAM_ASSERT(!bWriteOnly, "Readback texture cannot be write-only.");
        JAM_ASSERT(!bComputeReadWrite, "Readback texture cannot be a compute read/write target.");
        JAM_ASSERT(!bRenderTargetAttachment, "Readback texture cannot be a render target.");
        JAM_ASSERT(!bDepthStencilAttachment, "Readback texture cannot be a depth/stencil target.");
        JAM_ASSERT(!bMutable, "Readback texture cannot be mutable.");
        JAM_ASSERT(!bNeedSrv, "Readback texture does not need a shader resource view.");
        JAM_ASSERT(!bNeedUav, "Readback texture cannot be a UAV target.");
    }

    // format
    const TextureFormatInfo formatInfo       = TextureFormatInfoFrom(_format);
    const bool              bWriteOnlyDsvTex = bWriteOnly && bDepthStencilAttachment && !bRenderTargetAttachment && !bComputeReadWrite;
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
    texture.bRtvAttachable     = bRenderTargetAttachment;
    texture.bDsvAttachable     = bDepthStencilAttachment;
    texture.bReadback          = bReadback;

    if (_type == eTextureType::Texture2D || _type == eTextureType::TextureCube)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width                = NumericCast<UINT>(texture.width);
        desc.Height               = NumericCast<UINT>(texture.height);
        desc.MipLevels            = NumericCast<UINT>(texture.numMips);
        desc.ArraySize            = NumericCast<UINT>(_arraySize);
        desc.Format               = dxgiFormat;

        desc.SampleDesc.Count   = NumericCast<UINT>(samples);
        desc.SampleDesc.Quality = bIsMSAA ? QueryMaxMsaaQualityLevel_(desc.Format, samples) : 0;

        if (bReadback)
        {
            desc.Usage          = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.BindFlags      = 0;
        }
        else
        {
            desc.Usage          = bMutable ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
            desc.CPUAccessFlags = 0;

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
            if (bComputeReadWrite)
            {
                desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                desc.Usage = D3D11_USAGE_DEFAULT;
            }
        }

        // misc
        desc.MiscFlags = 0;
        if (bIsCubemap)
        {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        }
        if (bGenMips)
        {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        if (bHasInitData)
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
                    const UINT       rowPitch     = NumericCast<UINT>(width * BytePerPixelOf(_format));
                    JAM_ASSERT(mem.GetByteWidth() == rowPitch * height, "Texture data size mismatch");
                    initData.emplace_back(mem.GetPtr(), rowPitch, 0);
                }
            }

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture2D(&desc, initData.data(), &texture.pTexture2D));
        }
        else
        {
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture2D(&desc, nullptr, &texture.pTexture2D));
        }
    }
    else
    {
        JAM_ASSERT(_arraySize == 1, "3D texture array size must be 1");
        JAM_ASSERT(!bIsCubemap, "3D texture cannot be a cubemap");
        JAM_ASSERT(!bDepthStencilAttachment, "3D texture cannot be a Depth/Stencil view.");

        D3D11_TEXTURE3D_DESC desc = {};
        desc.Width                = NumericCast<UINT>(texture.width);
        desc.Height               = NumericCast<UINT>(texture.height);
        desc.Depth                = NumericCast<UINT>(texture.depth);
        desc.MipLevels            = NumericCast<UINT>(texture.numMips);
        desc.Format               = dxgiFormat;

        if (bReadback)
        {
            desc.Usage          = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.BindFlags      = 0;
        }
        else
        {
            desc.Usage          = bMutable ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
            desc.CPUAccessFlags = 0;

            // bind flags 설정
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
            if (bComputeReadWrite)
            {
                desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                desc.Usage = D3D11_USAGE_DEFAULT;
            }
        }

        // 기타 설정
        desc.MiscFlags = 0;
        if (bGenMips)
        {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        if (bHasInitData)
        {
            JAM_ASSERT(_initDataOrEmpty.size() == numMips, "Texture data count mismatch");

            Vector<D3D11_SUBRESOURCE_DATA> initData = {};
            initData.reserve(numMips);
            for (Int32 mipIdx = 0; mipIdx < numMips; mipIdx++)
            {
                const auto [width, height, depth] = CalcTextureResolution(_width, _height, _depth, mipIdx);
                const MemoryView mem              = _initDataOrEmpty[mipIdx];
                const UINT       rowPitch         = NumericCast<UINT>(width * BytePerPixelOf(_format));
                const UINT       slicePitch       = NumericCast<UINT>(rowPitch * height);
                JAM_ASSERT(mem.GetByteWidth() == slicePitch * depth, "Texture data size mismatch");
                initData.emplace_back(mem.GetPtr(), rowPitch, slicePitch);
            }

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture3D(&desc, initData.data(), &texture.pTexture3D));
        }
        else
        {
            JAM_CHECK_HRESULT(m_pd3d11Device->CreateTexture3D(&desc, nullptr, &texture.pTexture3D));
        }
    }

    // SRV 생성
    if (bNeedSrv)
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
                    srvDesc.TextureCubeArray.MipLevels        = NumericCast<UINT>(numMips);
                    srvDesc.TextureCubeArray.First2DArrayFace = 0;
                    srvDesc.TextureCubeArray.NumCubes         = NumericCast<UINT>(_arraySize / 6);
                }
                else
                {
                    srvDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURECUBE;
                    srvDesc.TextureCube.MostDetailedMip = 0;
                    srvDesc.TextureCube.MipLevels       = NumericCast<UINT>(numMips);
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
                        srvDesc.Texture2DMSArray.ArraySize       = NumericCast<UINT>(_arraySize);
                    }
                    else
                    {
                        srvDesc.ViewDimension                  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                        srvDesc.Texture2DArray.MostDetailedMip = 0;
                        srvDesc.Texture2DArray.MipLevels       = NumericCast<UINT>(numMips);
                        srvDesc.Texture2DArray.FirstArraySlice = 0;
                        srvDesc.Texture2DArray.ArraySize       = NumericCast<UINT>(_arraySize);
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
                        srvDesc.Texture2D.MipLevels       = NumericCast<UINT>(numMips);
                    }
                }
            }
        }
        else
        {
            srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MostDetailedMip = 0;
            srvDesc.Texture3D.MipLevels       = NumericCast<UINT>(numMips);
        }

        JAM_CHECK_HRESULT(m_pd3d11Device->CreateShaderResourceView(texture.pResource, &srvDesc, &texture.pSrv));
    }

    if (bNeedUav)
    {
        JAM_CHECK_HRESULT(m_pd3d11Device->CreateUnorderedAccessView(texture.pResource, nullptr, &texture.pUav));
    }

    return texture;
}

HardwareBuffer Graphics::CreateHardwareBuffer_(
    const Int32       _byteWidth,
    const eBufferType _type,
    const MemoryView  _initDataOrEmpty) const
{
    JAM_ASSERT(_byteWidth > 0, "Buffer size must be greater than zero.");

    const bool bHasInitData = !_initDataOrEmpty.IsEmpty();
    const bool bDynamic     = !bHasInitData || _type == eBufferType::ConstantBuffer;

    Int32 byteWidth = _byteWidth;
    if (_type == eBufferType::ConstantBuffer)
    {
        byteWidth = AlignUp(byteWidth, 16);
    }

    D3D11_BUFFER_DESC desc   = {};
    desc.ByteWidth           = NumericCast<UINT>(byteWidth);
    desc.StructureByteStride = 0;
    desc.MiscFlags           = 0;
    desc.BindFlags           = _type == eBufferType::VertexBuffer   ? D3D11_BIND_VERTEX_BUFFER
                             : _type == eBufferType::IndexBuffer    ? D3D11_BIND_INDEX_BUFFER
                             : _type == eBufferType::ConstantBuffer ? D3D11_BIND_CONSTANT_BUFFER
                                                                    : 0;
    desc.Usage               = bDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
    desc.CPUAccessFlags      = bDynamic ? D3D11_CPU_ACCESS_WRITE : 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = bHasInitData ? _initDataOrEmpty.GetPtr() : nullptr;

    HardwareBuffer buffer = {};
    buffer.byteWidth      = byteWidth;
    buffer.bDynamic       = bDynamic;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, bHasInitData ? &initData : nullptr, &buffer.pBuffer));
    return buffer;
}

StorageBuffer Graphics::CreateStorageBuffer_(
    const Int32                            _byteWidth,
    const Int32                            _stride,
    const eStorageBufferType               _type,
    const eStorageBufferDataType           _dataType,
    const Flags<eStorageBufferCreateFlags> _flags,
    const bool                             _bAppendConsumer,
    const bool                             _bCounter,
    const MemoryView                       _initDataOrEmpty) const
{
    JAM_ASSERT(_byteWidth > 0, "Buffer size must be greater than zero.");
    JAM_ASSERT(!(_bAppendConsumer && _bCounter), "Buffer cannot be both append/consume and counter.");

    const bool bHasInitData  = !_initDataOrEmpty.IsEmpty();
    const bool bNeedSrv      = (_flags & eStorageBufferCreateFlags::Readable);
    const bool bNeedUav      = (_flags & eStorageBufferCreateFlags::ComputeReadWrite);
    const bool bDrawIndirect = (_flags & eStorageBufferCreateFlags::IndirectArgs);
    const bool bReadback     = (_type == eStorageBufferType::Readback);
    const bool bDynamic      = !bReadback && !bHasInitData && !bNeedUav;

    if (JAM_DEBUG_ENABLED && bReadback)
    {
        JAM_ASSERT(!(_flags & eStorageBufferCreateFlags::ComputeReadWrite), "Readback buffer cannot have UAV.");
        JAM_ASSERT(!(_flags & eStorageBufferCreateFlags::Readable), "Readback buffer cannot have SRV.");
        JAM_ASSERT(!(_flags & eStorageBufferCreateFlags::IndirectArgs), "Readback buffer cannot be indirect args.");
    }

    if (JAM_DEBUG_ENABLED && _type != eStorageBufferType::Structured)
    {
        JAM_ASSERT(_bAppendConsumer == false, "Append/Consume flag is only valid for Structured buffer.");
        JAM_ASSERT(_bCounter == false, "Counter flag is only valid for Structured buffer.");
    }

    Int32       byteWidth   = _byteWidth;
    Int32       stride      = 0;
    Int32       numElements = 0;
    DXGI_FORMAT format      = DXGI_FORMAT_UNKNOWN;

    switch (_type)
    {
        case eStorageBufferType::Raw:
        {
            byteWidth   = AlignUp(byteWidth, 4);
            stride      = 0;
            format      = DXGI_FORMAT_R32_TYPELESS;
            numElements = byteWidth / 4;
            break;
        }
        case eStorageBufferType::Structured:
        {
            stride      = _stride;
            format      = DXGI_FORMAT_UNKNOWN;
            numElements = byteWidth / stride;
            break;
        }
        case eStorageBufferType::Typed:
        {
            const BufferDataTypeInfo info = BufferDataTypeInfoFrom_(_dataType);
            stride                        = info.stride;
            format                        = info.format;
            numElements                   = byteWidth / stride;
            break;
        }
        case eStorageBufferType::Readback:
        {
            break;
        }
        default: JAM_UNREACHABLE;
    }

    D3D11_BUFFER_DESC desc   = {};
    desc.ByteWidth           = NumericCast<UINT>(byteWidth);
    desc.StructureByteStride = NumericCast<UINT>(stride);
    desc.BindFlags           = 0;
    desc.MiscFlags           = 0;

    if (bReadback)
    {
        desc.Usage          = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    }
    else
    {
        if (bNeedSrv)
        {
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }

        if (bNeedUav)
        {
            desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        if (bDrawIndirect)
        {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        }

        if (_type == eStorageBufferType::Raw)
        {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        }

        if (_type == eStorageBufferType::Structured)
        {
            desc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        }

        if (bNeedUav)
        {
            desc.Usage          = D3D11_USAGE_DEFAULT;
            desc.CPUAccessFlags = 0;
        }
        else if (bDynamic)
        {
            desc.Usage          = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        }
        else
        {
            desc.Usage          = D3D11_USAGE_IMMUTABLE;
            desc.CPUAccessFlags = 0;
        }
    }

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = bHasInitData ? _initDataOrEmpty.GetPtr() : nullptr;

    StorageBuffer buffer  = {};
    buffer.type           = _type;
    buffer.byteWidth      = byteWidth;
    buffer.bDynamic       = bDynamic;
    buffer.stride         = stride;
    buffer.numElems       = numElements;
    buffer.bReadback      = bReadback;
    buffer.bIndirectArgs  = bDrawIndirect;
    buffer.bAppendConsume = _bAppendConsumer;
    buffer.bCounter       = _bCounter;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateBuffer(&desc, bHasInitData ? &initData : nullptr, &buffer.pBuffer));

    if (!bReadback)
    {
        // SRV
        if (bNeedSrv)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format                          = format;

            if (_type == eStorageBufferType::Raw)
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
        if (bNeedUav)
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format                           = format;
            uavDesc.ViewDimension                    = D3D11_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement              = 0;
            uavDesc.Buffer.NumElements               = numElements;
            uavDesc.Buffer.Flags                     = 0;

            if (_type == eStorageBufferType::Raw)
            {
                uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
            }

            if (_type == eStorageBufferType::Structured && _bAppendConsumer)
            {
                uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
            }

            if (_type == eStorageBufferType::Structured && _bCounter)
            {
                uavDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_COUNTER;
            }

            JAM_CHECK_HRESULT(m_pd3d11Device->CreateUnorderedAccessView(buffer.pBuffer, &uavDesc, &buffer.pUav));
        }
    }

    return buffer;
}

FrameBuffer Graphics::CreateFrameBuffer_(
    const Span<const TextureHandle> _attachments,
    const bool                      _bDestoryTexture)
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

MutableMemoryView Graphics::BeginUpdateBuffer_(
    const HardwareBuffer& _buffer) const
{
    JAM_ASSERT(_buffer.bDynamic, "Only dynamic buffers can be updated with BeginUpdateBuffer/EndUpdateBuffer.");
    D3D11_MAPPED_SUBRESOURCE mapped;
    JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(_buffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
    return { mapped.pData, _buffer.byteWidth };
}

void Graphics::EndUpdateBuffer_(
    const HardwareBuffer& _buffer) const
{
    JAM_ASSERT(_buffer.bDynamic, "Only dynamic buffers can be updated with BeginUpdateBuffer/EndUpdateBuffer.");
    m_pd3d11DeviceContext->Unmap(_buffer.pBuffer, 0);
}

void Graphics::UpdateBuffer_(
    const HardwareBuffer& _buffer,
    const MemoryView      _uploadData,
    const Int32           _offset,
    const bool            _bDiscard)
{
    const Byte* pUploadData     = _uploadData.GetPtr();
    Int32       uploadDataWidth = NumericCast<Int32>(_uploadData.GetByteWidth());
    uploadDataWidth             = Min<Int32>(uploadDataWidth, _buffer.byteWidth - _offset);

    if (_buffer.bDynamic)
    {
        if (_bDiscard)
        {
            D3D11_MAPPED_SUBRESOURCE mapped;
            JAM_CHECK_HRESULT(m_pd3d11DeviceContext->Map(_buffer.pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
            std::memcpy(static_cast<Byte*>(mapped.pData) + _offset, pUploadData, uploadDataWidth);
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

            D3D11_BOX box;
            box.left   = 0;
            box.right  = uploadDataWidth;
            box.top    = 0;
            box.bottom = 1;
            box.front  = 0;
            box.back   = 1;

            m_pd3d11DeviceContext->CopySubresourceRegion(_buffer.pBuffer, 0, _offset, 0, 0, pUploadBuffer, 0, &box);
        }
    }
    else
    {
        JAM_ASSERT(_bDiscard == false, "Discard flag cannot be used for non-dynamic buffers.");

        // 해당 코드는 ConstantBuffer에서 오류가 발생함
        // ConstantBuffer는 UpdateSubresource로 복사가 안됨.
        // https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-updatesubresource

        D3D11_BOX box;
        box.left   = _offset;
        box.right  = _offset + uploadDataWidth;
        box.top    = 0;
        box.bottom = 1;
        box.front  = 0;
        box.back   = 1;

        m_pd3d11DeviceContext->UpdateSubresource(_buffer.pBuffer, 0, &box, pUploadData, 0, 0);
    }
}

MemoryView Graphics::BeginReadBuffer_(const HardwareBuffer& _buffer) const
{
}

void Graphics::EndReadBuffer_(const HardwareBuffer& _buffer) const
{
}

Int32 Graphics::ReadBuffer_(const HardwareBuffer& _buffer, MemoryView _dst) const
{
}

void Graphics::CopyBuffer_(const HardwareBuffer& _dstBuffer, Int32 _dstOffset, const HardwareBuffer& _srcBuffer, Int32 _srcOffset, Int32 _byteWidth) const
{
}

MemoryView Graphics::BeginReadTexture_(const HardwareTextureRef& _texture, Int32 _mipIndex, Int32 _arrayIndex) const
{
}

void Graphics::EndReadTexture_(const HardwareTextureRef& _texture, Int32 _mipIndex, Int32 _arrayIndex) const
{
}

Int32 Graphics::ReadTexture_(const HardwareTextureRef& _texture, Int32 _mipIndex, Int32 _arrayIndex, MemoryView _dst, Int32 _dstRowPitch, Int32 _dstSlicePitch) const
{
}

void Graphics::UpdateTexture_(
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

void Graphics::CopyTexture_(
    const HardwareTextureRef& _dstTexture,
    const Int32               _dstMipIndex,
    const Int32               _dstArrayIndex,
    const Int32               _dstX,
    const Int32               _dstY,
    const Int32               _dstZ,
    const HardwareTextureRef& _srcTexture,
    const Int32               _srcMipIndex,
    const Int32               _srcArrayIndex,
    const Int32               _srcX,
    const Int32               _srcY,
    const Int32               _srcZ,
    const Int32               _srcWidthOrZero,
    const Int32               _srcHeightOrZero,
    const Int32               _srcDepthOrZero) const
{
    const auto [srcWidth, srcHeight, srcDepth] = CalcTextureResolution(_srcTexture.width, _srcTexture.height, _srcTexture.depth, _srcMipIndex);
    const auto [dstWidth, dstHeight, dstDepth] = CalcTextureResolution(_dstTexture.width, _dstTexture.height, _dstTexture.depth, _dstMipIndex);

    Int32 blitWidth  = _srcWidthOrZero > 0 ? _srcWidthOrZero : srcWidth - _srcX;
    Int32 blitHeight = _srcHeightOrZero > 0 ? _srcHeightOrZero : srcHeight - _srcY;
    Int32 blitDepth  = _srcDepthOrZero > 0 ? _srcDepthOrZero : srcDepth - _srcZ;
    blitWidth        = Min(blitWidth, srcWidth - _srcX, dstWidth - _dstX);
    blitHeight       = Min(blitHeight, srcHeight - _srcY, dstHeight - _dstY);
    blitDepth        = Min(blitDepth, srcDepth - _srcZ, dstDepth - _dstZ);

    D3D11_BOX box = {};
    box.left      = _srcX;
    box.top       = _srcY;
    box.front     = _srcZ;
    box.right     = _srcX + blitWidth;
    box.bottom    = _srcY + blitHeight;
    box.back      = _srcZ + blitDepth;

    const UINT srcIndex = CalcTextureIndex(_srcMipIndex, _srcArrayIndex, _srcTexture.numMips);
    const UINT dstIndex = CalcTextureIndex(_dstMipIndex, _dstArrayIndex, _dstTexture.numMips);

    m_pd3d11DeviceContext->CopySubresourceRegion(
        _dstTexture.pResource,
        dstIndex,
        _dstX,
        _dstY,
        _dstZ,
        _srcTexture.pResource,
        srcIndex,
        &box);
}

ID3D11InputLayout* Graphics::GetOrCreateD3D11InputLayoutFromCurrentBind_()
{
    // input layout 을 바꿔야하는 경우
    // 1. 버텍스 레이아웃이 하나라도 변경되었을 경우 -> 버텍스 레이아웃 핸들값 섞기
    // 2. 인스턴스 버퍼 바인딩이 변경되었을 경우 -> 인스턴스 버퍼 핸들값 섞기
    // 3. 스트림의 구성이 바뀌었을 경우 -> 스트림 개수 섞기
    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(m_numStreamsBound));

    Int32 numInstanceData = 0;
    if (m_instanceBind)
    {
        const VertexBuffer& vb = m_vertexBuffers[m_instanceBind];
        numInstanceData        = vb.stride / 16;
        hasher.Mix(AsMemoryView(numInstanceData));
    }
    for (Int32 i = 0; i < m_numStreamsBound; ++i)
    {
        const VertexLayoutHandle& vlh = m_vlhStreamBound[i];
        hasher.Mix(AsMemoryView(vlh));
    }
    const UInt64 hash = hasher.Finish();
    const auto   it   = m_d3d11InputLayoutCache.find(hash);
    if (it != m_d3d11InputLayoutCache.end())
    {
        return it->second;
    }

    // TODO: delete this
    JAM_FUNCTION_PROFILE();

    Vector<D3D11_INPUT_ELEMENT_DESC> descs = {};
    descs.reserve(k_maxNumVertexAttributes);

    String hlsl = "struct VSInput {";
    for (Int32 i = 0; i < m_numStreamsBound; ++i)
    {
        const VertexLayoutHandle& vlh = m_vlhStreamBound[i];
        const VertexLayoutRef&    vl  = m_vertexLayouts[vlh];
        Int32                     j   = 0;
        for (const VertexLayoutElement& vle: vl.vl.GetElems())
        {
            const HLSLDataTypeInfo&   dataType = HLSLDataTypeInfoFrom_(vle.format);
            const HLSLSemanticInfo&   semantic = HLSLSemanticInfoFrom_(vle.attrib);
            D3D11_INPUT_ELEMENT_DESC& desc     = descs.emplace_back();
            desc.SemanticName                  = semantic.name.GetPtr();
            desc.SemanticIndex                 = semantic.index;
            desc.Format                        = dataType.format;
            desc.InputSlot                     = i;
            desc.AlignedByteOffset             = D3D11_APPEND_ALIGNED_ELEMENT;
            desc.InputSlotClass                = D3D11_INPUT_PER_VERTEX_DATA;
            desc.InstanceDataStepRate          = 0;

            hlsl.AppendFormat("{} Element{} : {}{};", dataType.name, ++j, semantic.name, semantic.index);
        }
    }

    if (m_instanceBind)
    {
        for (Int32 i = 0; i < numInstanceData; ++i)
        {
            D3D11_INPUT_ELEMENT_DESC& desc = descs.emplace_back();
            desc.SemanticName              = "INSTANCE";
            desc.SemanticIndex             = i;
            desc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
            desc.InputSlot                 = NumericCast<UInt32>(m_numStreamsBound);
            desc.AlignedByteOffset         = D3D11_APPEND_ALIGNED_ELEMENT;
            desc.InputSlotClass            = D3D11_INPUT_PER_INSTANCE_DATA;
            desc.InstanceDataStepRate      = 1;

            hlsl.AppendFormat("float4 InstanceData{} : INSTANCE{};", i, i);
        }
    }
    hlsl.Append("}; void VSMain(in VSInput input) {}");

    ShaderCompilerInfo info     = { eShader::VertexShader, "VSMain", {}, eShaderCompileFlags::ReleaseDefault };
    ShaderCompiler     compiler = ShaderCompiler::CompileHLSLSource(hlsl, info);
    JAM_ASSERT(compiler.IsSucceeded(), "Failed to compile shader for input layout creation. Error: {}", compiler.GetErrorMessage());

    const MemoryView   mem               = compiler.GetBytecode();
    ID3D11InputLayout* pd3d11InputLayout = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateInputLayout(
        descs.data(),
        NumericCast<UINT>(descs.size()),
        mem.GetPtr(),
        mem.GetByteWidth(),
        &pd3d11InputLayout));

    m_d3d11InputLayoutCache[hash] = pd3d11InputLayout;

    if (JAM_DEBUG_ENABLED)
    {
        JAM_LOG_DEBUG("Created input layout: NumElements: {} InstanceData: {} Hash: {:016X}", descs.size(), m_instanceBind ? numInstanceData : 0, hash);
        for (const D3D11_INPUT_ELEMENT_DESC& desc: descs)
        {
            JAM_LOG_DEBUG("  SemanticName: {}{} Format: {} InputSlot: {} AlignedByteOffset: {} InputSlotClass: {} InstanceDataStepRate: {}", desc.SemanticName, desc.SemanticIndex, desc.Format, desc.InputSlot, desc.AlignedByteOffset, desc.InputSlotClass, desc.InstanceDataStepRate);
        }
    }

    return pd3d11InputLayout;
}

ID3D11RasterizerState* Graphics::GetOrCreateD3D11RasterizerStateFromCurrentBind_()
{
    const Flags<eRenderFlags> flags = FilterRasterizerFlags(m_renderFlagsBind);

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

    const Flags<eRenderFlags> cull = FilterCullModeFlags(m_renderFlagsBind);
    desc.CullMode                  = cull == eRenderFlags::CullMode_Front ? D3D11_CULL_FRONT
                                   : cull == eRenderFlags::CullMode_Back  ? D3D11_CULL_BACK
                                                                          : D3D11_CULL_NONE;

    desc.FrontCounterClockwise = flags & eRenderFlags::CCWFront;
    desc.DepthBias             = 0;
    desc.DepthBiasClamp        = 0.f;
    desc.SlopeScaledDepthBias  = 0.f;
    desc.DepthClipEnable       = !(flags & eRenderFlags::DepthClamp);
    desc.MultisampleEnable     = flags & eRenderFlags::MultiSample;
    desc.AntialiasedLineEnable = flags & eRenderFlags::LineAA;
    desc.ScissorEnable         = flags & eRenderFlags::Scissor;

    ID3D11RasterizerState* pRasterizerState = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateRasterizerState(&desc, &pRasterizerState));
    m_d3d11RasterizerStateCache[key] = pRasterizerState;
    JAM_LOG_DEBUG("Created rasterizer state: FillMode: {} CullMode: {} FrontCounterClockwise: {} DepthClipEnable: {} MultisampleEnable: {} AntialiasedLineEnable: {} ScissorEnable: {}", desc.FillMode, desc.CullMode, desc.FrontCounterClockwise, desc.DepthClipEnable, desc.MultisampleEnable, desc.AntialiasedLineEnable, desc.ScissorEnable);
    return pRasterizerState;
}

ID3D11BlendState* Graphics::GetOrCreateD3D11BlendStateFromCurrentBind_()
{
    const Flags<eRenderFlags> flags = FilterBlendFlags(m_renderFlagsBind);

    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(flags));
    hasher.Mix(m_blendFlagsBinds);
    const UInt64 hash = hasher.Finish();
    const auto   it   = m_d3d11BlendStateCache.find(hash);
    if (it != m_d3d11BlendStateCache.end())
    {
        return it->second;
    }

    D3D11_BLEND_DESC desc       = {};
    desc.AlphaToCoverageEnable  = flags & eRenderFlags::AlphaToCoverage;
    desc.IndependentBlendEnable = flags & eRenderFlags::IndependentBlend;
    const Int32 numRts          = desc.IndependentBlendEnable ? k_maxNumRenderTargetViewSlots : 1;
    for (Int32 i = 0; i < numRts; ++i)
    {
        Flags<eBlendFlags>              rtBlend = m_blendFlagsBinds[i];
        D3D11_RENDER_TARGET_BLEND_DESC& rtDesc  = desc.RenderTarget[i];

        rtDesc.BlendEnable    = rtBlend & eBlendFlags::Enable;
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
        JAM_LOG_DEBUG("Created blend state: AlphaToCoverageEnable: {} IndependentBlendEnable: {}");
        for (Int32 i = 0; i < numRts; ++i)
        {
            const D3D11_RENDER_TARGET_BLEND_DESC& rtDesc = desc.RenderTarget[i];
            JAM_LOG_DEBUG("RenderTarget[{}]: BlendEnable: {} SrcBlend: {} DestBlend: {} BlendOp: {} SrcBlendAlpha: {} DestBlendAlpha: {} BlendOpAlpha: {} RenderTargetWriteMask: {}", i, rtDesc.BlendEnable, rtDesc.SrcBlend, rtDesc.DestBlend, rtDesc.BlendOp, rtDesc.SrcBlendAlpha, rtDesc.DestBlendAlpha, rtDesc.BlendOpAlpha, rtDesc.RenderTargetWriteMask);
        }
    }

    return pd3d11BlendState;
}

ID3D11DepthStencilState* Graphics::GetOrCreateD3D11DepthStencilStateFromCurrentBind_()
{
    const Flags<eRenderFlags> flags = FilterDepthFlags(m_renderFlagsBind);

    Murmur3Hasher64 hasher {};
    hasher.Mix(AsMemoryView(flags));
    hasher.Mix(AsMemoryView(m_fstencilBind));
    hasher.Mix(AsMemoryView(m_bstencilBind));
    const UInt64 key = hasher.Finish();
    const auto   it  = m_d3d11DepthStencilStateCache.find(key);
    if (it != m_d3d11DepthStencilStateCache.end())
    {
        return it->second;
    }

    D3D11_DEPTH_STENCIL_DESC  desc      = {};
    const Flags<eRenderFlags> depthFunc = FilterDepthFuncFlags(m_renderFlagsBind);
    desc.DepthEnable                    = (depthFunc != eRenderFlags::None);
    desc.DepthWriteMask                 = (flags & eRenderFlags::DepthWrite) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc                      = D3D11ComparisonFuncFrom_(ResolveDepthFuncFlags(depthFunc));
    desc.StencilEnable                  = (m_fstencilBind != eStencilFlags::None || m_bstencilBind != eStencilFlags::None);
    desc.StencilReadMask                = 0xFF;
    desc.StencilWriteMask               = 0xFF;

    // front
    desc.FrontFace.StencilFailOp      = D3D11StencilOpFrom_(ResolveStencilFailOpFlags(m_fstencilBind));
    desc.FrontFace.StencilDepthFailOp = D3D11StencilOpFrom_(ResolveDepthFailOpFlags(m_fstencilBind));
    desc.FrontFace.StencilPassOp      = D3D11StencilOpFrom_(ResolvePassOpFlags(m_fstencilBind));
    desc.FrontFace.StencilFunc        = D3D11ComparisonFuncFrom_(ResolveCompareFuncFlags(m_fstencilBind));

    // back
    desc.BackFace.StencilFailOp      = D3D11StencilOpFrom_(ResolveStencilFailOpFlags(m_bstencilBind));
    desc.BackFace.StencilDepthFailOp = D3D11StencilOpFrom_(ResolveDepthFailOpFlags(m_bstencilBind));
    desc.BackFace.StencilPassOp      = D3D11StencilOpFrom_(ResolvePassOpFlags(m_bstencilBind));
    desc.BackFace.StencilFunc        = D3D11ComparisonFuncFrom_(ResolveCompareFuncFlags(m_bstencilBind));

    ID3D11DepthStencilState* pDepthStencilState;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateDepthStencilState(&desc, &pDepthStencilState));
    m_d3d11DepthStencilStateCache[key] = pDepthStencilState;

    JAM_LOG_DEBUG("Created depth stencil state: DepthEnable: {} DepthWriteMask: {} DepthFunc: {} StencilEnable: {} FrontStencilFailOp: {} FrontStencilDepthFailOp: {} FrontStencilPassOp: {} FrontStencilFunc: {} BackStencilFailOp: {} BackStencilDepthFailOp: {} BackStencilPassOp: {} BackStencilFunc: {}", desc.DepthEnable, desc.DepthWriteMask, desc.DepthFunc, desc.StencilEnable, desc.FrontFace.StencilFailOp, desc.FrontFace.StencilDepthFailOp, desc.FrontFace.StencilPassOp, desc.FrontFace.StencilFunc, desc.BackFace.StencilFailOp, desc.BackFace.StencilDepthFailOp, desc.BackFace.StencilPassOp, desc.BackFace.StencilFunc);
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
    desc.BorderColor[0]     = NumericCast<float>(_rgba.r) / 255.0f;
    desc.BorderColor[1]     = NumericCast<float>(_rgba.g) / 255.0f;
    desc.BorderColor[2]     = NumericCast<float>(_rgba.b) / 255.0f;
    desc.BorderColor[3]     = NumericCast<float>(_rgba.a) / 255.0f;
    desc.MinLOD             = -Limits<float>::k_max;
    desc.MaxLOD             = Limits<float>::k_max;

    ID3D11SamplerState* pSamplerState = nullptr;
    JAM_CHECK_HRESULT(m_pd3d11Device->CreateSamplerState(&desc, &pSamplerState));
    m_d3d11SamplerStateCache[key] = pSamplerState;

    JAM_LOG_DEBUG("Created sampler state: Filter: {} AddressU: {} AddressV: {} AddressW: {} CompareFunc: {} BorderColor: ({}, {}, {}, {})", filter, ResolveAddressUFlags(_state), ResolveAddressVFlags(_state), ResolveAddressWFlags(_state), compareFunc, _rgba.r, _rgba.g, _rgba.b, _rgba.a);
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

    JAM_LOG_DEBUG("Created upload buffer of size {} bytes", _byteWidth);
    return m_pUploadBufferCache;
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