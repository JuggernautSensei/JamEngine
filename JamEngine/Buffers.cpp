#include "pch.h"

#include "Buffers.h"
#include "Renderer.h"

namespace jam
{

void Buffer::CopyFrom(const Buffer& _other) const
{
    ID3D11Buffer* pBuffer      = m_buffer.Get();
    ID3D11Buffer* pOtherBuffer = _other.m_buffer.Get();

    JAM_ASSERT(pOtherBuffer, "Source buffer is null");
    JAM_ASSERT(pBuffer, "Destination buffer is null");
    JAM_ASSERT(m_byteWidth == _other.m_byteWidth, "Buffer sizes must match for copy");
    JAM_ASSERT(m_access != eResourceAccess::Immutable, "Cannot copy to an immutable buffer");

    ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();
    ctx->CopyResource(pBuffer, pOtherBuffer);
}

void Buffer::Upload(const UInt32 _dataByteWidth, const void* _pData, const UInt32 _offset) const
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

    // create staging buffer
    Buffer stagingBuffer;
    stagingBuffer.Initialize_(m_byteWidth, 0, eResourceAccess::CPUReadable);

    // copy data to staging buffer
    JAM_ASSERT(m_buffer.Get(), "Buffer is null");
    stagingBuffer.CopyFrom(*this);

    // map the staging buffer to read data
    ID3D11Buffer*            pStagingBuffer = stagingBuffer.m_buffer.Get();
    D3D11_MAPPED_SUBRESOURCE mapped;
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

UInt32 Buffer::Reset()
{
    UInt32 refCount = m_buffer.Reset();
    *this           = Buffer();   // Reset the buffer to a new state
    return refCount;
}

void Buffer::Initialize_(const UInt32 _byteWidth, const UINT _bindFlag, eResourceAccess _access, const std::optional<BufferInitializeData>& _initializeData)
{
    constexpr UINT k_accessFlagsTable[] = {
        0,                        // GPUWriteable
        0,                        // Immutable
        D3D11_CPU_ACCESS_WRITE,   // CPUWriteable
        D3D11_CPU_ACCESS_READ,    // CPUReadable
    };

    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = _byteWidth;
    desc.Usage               = static_cast<D3D11_USAGE>(_access);
    desc.BindFlags           = _bindFlag;
    desc.CPUAccessFlags      = k_accessFlagsTable[static_cast<int>(_access)];
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;

    Renderer::CreateBuffer(desc, _initializeData, m_buffer.GetAddressOf());
    m_byteWidth = _byteWidth;
    m_access    = _access;
}

void VertexBuffer::Initialize(const UInt32 _vertexStride, const UInt32 _vertexCount, const eResourceAccess _access, const std::optional<BufferInitializeData>& _initializeData)
{
    JAM_ASSERT(_access != eResourceAccess::CPUReadable, "Vertex buffer cannot be CPU readable");
    Initialize_(_vertexStride * _vertexCount, D3D11_BIND_VERTEX_BUFFER, _access, _initializeData);
    m_stride = _vertexStride;
}

void VertexBuffer::Bind() const
{
    Renderer::BindVertexBuffer(m_buffer.Get(), m_stride);
}

void IndexBuffer::Initialize(const UInt32 _indexCount, const eResourceAccess _access, const std::optional<IndexBufferInitializeData>& _initializeData)
{
    if (_initializeData)
    {
        BufferInitializeData initData;
        initData.pData = _initializeData->pData;
        Initialize_(sizeof(Index) * _indexCount, D3D11_BIND_INDEX_BUFFER, _access, initData);
    }
    else
    {
        Initialize_(sizeof(Index) * _indexCount, D3D11_BIND_INDEX_BUFFER, _access, std::nullopt);
    }   
}

void IndexBuffer::Bind() const
{
    Renderer::BindIndexBuffer(m_buffer.Get());
}

void ConstantBuffer::Initialize(const UInt32 _byteWidth, const std::optional<BufferInitializeData>& _initializeData)
{
    Initialize_(_byteWidth, D3D11_BIND_CONSTANT_BUFFER, eResourceAccess::CPUWriteable, _initializeData);
}

void ConstantBuffer::Bind(const eShader _shader, const UInt32 _slot) const
{
    ID3D11Buffer* buffers[] = { m_buffer.Get() };
    Renderer::BindConstantBuffers(_shader, _slot, buffers);
}

}   // namespace jam