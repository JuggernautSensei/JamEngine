#include "pch.h"

#include "Buffers.h"
#include "Renderer.h"

namespace jam
{

void Buffer::CopyFrom(const Buffer& _other)
{
    ID3D11Buffer* pBuffer      = m_buffer.Get();
    ID3D11Buffer* pOtherBuffer = _other.m_buffer.Get();

    JAM_ASSERT(pOtherBuffer, "Source buffer is null");
    JAM_ASSERT(pBuffer, "Destination buffer is null");
    JAM_ASSERT(m_byteWidth == _other.m_byteWidth, "Buffer sizes must match for copy");
    JAM_ASSERT(m_access != eResourceAccess::Immutable, "Cannot copy to an immutable buffer");

    ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();
    ctx->CopyResource(pBuffer, pOtherBuffer);
    m_usingByte = _other.m_usingByte;
}

void Buffer::Upload(const UInt32 _dataByteWidth, const void* _pData, const UInt32 _offset)
{
    JAM_ASSERT(_pData, "Data pointer is null");
    JAM_ASSERT(_offset + _dataByteWidth <= m_byteWidth, "Data byte width exceeds buffer size");
    JAM_ASSERT(m_access == eResourceAccess::CPUWriteable, "Buffer must be dynamic");

    ID3D11DeviceContext* ctx     = Renderer::GetDeviceContext();
    ID3D11Buffer*        pBuffer = m_buffer.Get();

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(ctx->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        UInt8* dest = static_cast<UInt8*>(mapped.pData);
        memcpy(dest + _offset, _pData, _dataByteWidth);
        m_usingByte = _offset + _dataByteWidth;
        ctx->Unmap(pBuffer, 0);
    }
    else
    {
        JAM_ERROR("Failed to map buffer for upload.");
    }
}

std::optional<std::vector<UInt8>> Buffer::Download() const
{
    ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();

    Buffer stagingBuffer;
    stagingBuffer.Initialize_(0, m_byteWidth, eResourceAccess::CPUReadable, nullptr);

    ID3D11Buffer* pStagingBuffer = stagingBuffer.m_buffer.Get();
    ID3D11Buffer* pBuffer        = m_buffer.Get();
    JAM_ASSERT(pBuffer, "Buffer is null");

    ctx->CopyResource(pStagingBuffer, pBuffer);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(ctx->Map(pStagingBuffer, 0, D3D11_MAP_READ, 0, &mapped)))
    {
        std::vector<UInt8> data(m_byteWidth);
        ::memcpy(data.data(), mapped.pData, m_byteWidth);
        ctx->Unmap(pStagingBuffer, 0);
        return data;
    }
    else
    {
        JAM_ERROR("Failed to map buffer for download. Usage: STAGING, Size: {}", m_byteWidth);
        return std::nullopt;
    }
}

void Buffer::Initialize_(const UINT _flags, const UInt32 _byteWidth, eResourceAccess _access, const BufferInitializeData* _initializeData_orNull)
{
    constexpr D3D11_USAGE k_usageTable[] = {
        D3D11_USAGE_IMMUTABLE,   // Immutable
        D3D11_USAGE_DEFAULT,     // GPUWriteable
        D3D11_USAGE_STAGING,     // CPUReadable
        D3D11_USAGE_DYNAMIC,     // CPUWriteable
    };

    constexpr UINT k_accessFlagsTable[] = {
        0,                        // Immutable
        0,                        // GPUWriteable
        D3D11_CPU_ACCESS_READ,    // CPUReadable
        D3D11_CPU_ACCESS_WRITE,   // CPUWriteable
    };

    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = _byteWidth;
    desc.Usage               = k_usageTable[static_cast<int>(_access)];
    desc.BindFlags           = _flags;
    desc.CPUAccessFlags      = k_accessFlagsTable[static_cast<int>(_access)];
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;

    const void*    pInitializeData     = _initializeData_orNull ? _initializeData_orNull->pInitialData : nullptr;
    const unsigned pInitializeDataSize = _initializeData_orNull ? _initializeData_orNull->byteWidth : 0;
    m_byteWidth                        = _byteWidth;
    m_usingByte                        = pInitializeDataSize;
    Renderer::CreateBuffer(desc, pInitializeData, m_buffer.GetAddressOf());
}

VertexBuffer VertexBuffer::Create(const UInt32 _vertexStride, const UInt32 _vertexCount, const eResourceAccess _access, const BufferInitializeData* _vertexData_orNull)
{
    JAM_ASSERT(_access != eResourceAccess::CPUReadable, "Vertex buffer cannot be CPU readable");

    VertexBuffer buffer;
    buffer.Initialize_(D3D11_BIND_VERTEX_BUFFER, _vertexStride * _vertexCount, _access, _vertexData_orNull);
    buffer.m_stride = _vertexStride;
    return buffer;
}

void VertexBuffer::Bind() const
{
    Renderer::SetVertexBuffer(m_buffer.Get(), m_stride);
}

IndexBuffer IndexBuffer::Create(const bool _bUseExtendedIndex, const UInt32 _indexCount, const eResourceAccess _access, const BufferInitializeData* _indexData_orNull)
{
    JAM_ASSERT(_access != eResourceAccess::CPUReadable, "Vertex buffer cannot be CPU readable");
    IndexBuffer  buffer;
    const UInt32 indexStride = (_bUseExtendedIndex ? sizeof(UInt32) : sizeof(UInt16));
    buffer.Initialize_(D3D11_BIND_INDEX_BUFFER, indexStride * _indexCount, _access, _indexData_orNull);
    buffer.m_bUseExtendedIndex = _bUseExtendedIndex;
    return buffer;
}

void IndexBuffer::Bind() const
{
    Renderer::SetIndexBuffer(m_buffer.Get(), m_bUseExtendedIndex);
}

ConstantBuffer ConstantBuffer::Create(const UInt32 _byteWidth, const BufferInitializeData* _intialData_orNull)
{
    ConstantBuffer buffer;
    buffer.Initialize_(D3D11_BIND_CONSTANT_BUFFER, _byteWidth, eResourceAccess::CPUWriteable, _intialData_orNull);
    return buffer;
}

void ConstantBuffer::Bind(const eShader _shader, const UInt32 _slot) const
{
    Renderer::SetConstantBuffers(_shader, _slot, 1, m_buffer.GetAddressOf());
}

}   // namespace jam