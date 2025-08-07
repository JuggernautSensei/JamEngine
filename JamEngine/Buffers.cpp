#include "pch.h"

#include "Buffers.h"
#include "Renderer.h"

namespace jam
{

void Buffer::CopyFrom(const Buffer& _other) const
{
    JAM_ASSERT(_other.Get(), "Source buffer is null");
    JAM_ASSERT(Get(), "Destination buffer is null");
    JAM_ASSERT(m_byteWidth == _other.m_byteWidth, "Buffer sizes must match for copy");
    JAM_ASSERT(m_access != eResourceAccess::Immutable, "Cannot copy to an immutable buffer");

    ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();
    ctx->CopyResource(Get(), _other.Get());
}

void Buffer::Upload(const UInt32 _dataByteWidth, const void* _pData, const UInt32 _offset) const
{
    JAM_ASSERT(_pData, "Data pointer is null");
    JAM_ASSERT(_offset + _dataByteWidth <= m_byteWidth, "Data byte width exceeds buffer size");
    JAM_ASSERT(m_access == eResourceAccess::CPUWriteable, "Buffer must be dynamic");

    ID3D11DeviceContext*     ctx = Renderer::GetDeviceContext();
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        UInt8* dest = static_cast<UInt8*>(mapped.pData);
        memcpy(dest + _offset, _pData, _dataByteWidth);
        ctx->Unmap(Get(), 0);
    }
    else
    {
        JAM_ERROR("Failed to map buffer for upload.");
    }
}

UInt32 Buffer::Reset()
{
    UInt32 refCount = m_buffer.Reset();
    *this           = Buffer();   // Reset the buffer to a new state
    return refCount;
}

void Buffer::Initialize_(const UInt32 _byteWidth, const UInt32 _bindFlag, eResourceAccess _access, const std::optional<BufferInitializeData>& _initDataOrNull)
{
    constexpr UINT k_accessFlagsTable[] = {
        0,                        // GPUWriteable
        0,                        // Immutable
        D3D11_CPU_ACCESS_WRITE,   // CPUWriteable
        D3D11_CPU_ACCESS_READ,    // CPUReadable
    };

    // reset resource
    Reset();

    // create buffer description
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth           = _byteWidth;
    desc.Usage               = static_cast<D3D11_USAGE>(_access);
    desc.BindFlags           = _bindFlag;
    desc.CPUAccessFlags      = k_accessFlagsTable[static_cast<int>(_access)];
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;

    // create buffer
    Renderer::CreateBuffer(desc, _initDataOrNull, m_buffer.GetAddressOf());
    m_byteWidth = _byteWidth;
    m_access    = _access;
}

void VertexBuffer::Initialize(const UInt32 _vertexStride, const UInt32 _vertexCount, const eResourceAccess _access, const std::optional<BufferInitializeData>& _initDataOrNull)
{
    JAM_ASSERT(_access != eResourceAccess::CPUReadable, "Vertex buffer cannot be CPU readable");
    Initialize_(_vertexStride * _vertexCount, D3D11_BIND_VERTEX_BUFFER, _access, _initDataOrNull);
    m_stride = _vertexStride;
}

void VertexBuffer::Bind() const
{
    Renderer::BindVertexBuffer(m_buffer.Get(), m_stride);
}

void IndexBuffer::Initialize(const UInt32 _indexCount, const eResourceAccess _access, const std::optional<IndexBufferInitializeData>& _initDataOrNull)
{
    if (_initDataOrNull)
    {
        BufferInitializeData initData;
        initData.pData = _initDataOrNull->pData;
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

void ConstantBuffer::Initialize(const UInt32 _byteWidth, const std::optional<BufferInitializeData>& _initDataOrNull)
{
    Initialize_(_byteWidth, D3D11_BIND_CONSTANT_BUFFER, eResourceAccess::CPUWriteable, _initDataOrNull);
}

void ConstantBuffer::Bind(const eShader _shader, const UInt32 _slot) const
{
    ID3D11Buffer* buffers[] = { m_buffer.Get() };
    Renderer::BindConstantBuffers(_shader, _slot, buffers);
}

void StagingBuffer::Initialize(const UInt32 _byteWidth)
{
    Initialize_(_byteWidth, 0, eResourceAccess::CPUReadable);
}

std::optional<std::vector<UInt8>> StagingBuffer::ReadData() const
{
    JAM_ASSERT(m_access == eResourceAccess::CPUReadable, "Staging buffer must be CPU readable");

    ID3D11DeviceContext*     ctx = Renderer::GetDeviceContext();
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(m_buffer.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
    {
        std::vector<UInt8> data(mapped.RowPitch);
        memcpy(data.data(), mapped.pData, mapped.RowPitch);
        ctx->Unmap(m_buffer.Get(), 0);
        return data;
    }
    else
    {
        JAM_ERROR("Failed to map staging buffer for reading.");
        return std::nullopt;
    }
}

}   // namespace jam