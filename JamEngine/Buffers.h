#pragma once
#include "RendererCommons.h"
#include "Vertex.h"

namespace jam
{

class Buffer
{
public:
    // utility
    void      CopyFrom(const Buffer& _other) const;
    void      Upload(UInt32 _dataByteWidth, const void* _pData, UInt32 _offset = 0) const;
    NODISCARD std::optional<std::vector<UInt8>> Download() const;

    // getters
    NODISCARD ID3D11Buffer* const* GetAddressOf() const { return m_buffer.GetAddressOf(); }
    NODISCARD UInt32               GetByteWidth() const { return m_byteWidth; }

    // d3d11 accessors
    NODISCARD ID3D11Buffer* Get() const { return m_buffer.Get(); }
    UInt32                  Reset();
    NODISCARD bool          IsValid() const { return m_buffer != nullptr; }

protected:
    Buffer() = default;
    void Initialize_(UInt32 _byteWidth, UINT _bindFlag, eResourceAccess _access, const std::optional<BufferInitializeData>& _initializeData = std::nullopt);

    ComPtr<ID3D11Buffer> m_buffer    = nullptr;
    UInt32               m_byteWidth = 0;
    eResourceAccess      m_access    = eResourceAccess::Immutable;
};

class VertexBuffer : public Buffer
{
public:
    void Initialize(UInt32 _vertexStride, UInt32 _vertexCount, eResourceAccess _access, const std::optional<BufferInitializeData>& _initializeData = std::nullopt);
    void Bind() const;

    NODISCARD UInt32 GetStride() const { return m_stride; }
    NODISCARD UInt32 GetVertexCount() const { return m_stride == 0 ? 0 : m_byteWidth / m_stride; }

private:
    UInt32 m_stride = 0;   // vertex stride in bytes, used for binding
};

class IndexBuffer : public Buffer
{
public:
    void Initialize(UInt32 _indexCount, eResourceAccess _access, const std::optional<IndexBufferInitializeData>& _initializeData = std::nullopt);
    void Bind() const;

    NODISCARD constexpr UInt32 GetStride() const { return sizeof(UInt32); }
    NODISCARD UInt32           GetIndexCount() const { return m_byteWidth / GetStride(); }
};

class ConstantBuffer : public Buffer
{
public:
    void Initialize(UInt32 _byteWidth, const std::optional<BufferInitializeData>& _initializeData = std::nullopt);
    void Bind(eShader _shader, UInt32 _slot) const;
};

}   // namespace jam
