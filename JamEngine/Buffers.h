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
    NODISCARD bool          IsValid() const { return m_buffer != nullptr; }
    UInt32                  Reset();

protected:
    Buffer() = default;
    void Initialize_(UINT _bindFlag, eResourceAccess _access, UInt32 _byteWidth, const void* _pInitialData = nullptr);

    ComPtr<ID3D11Buffer> m_buffer    = nullptr;
    UInt32               m_byteWidth = 0;
    eResourceAccess      m_access    = eResourceAccess::Immutable;
};

class VertexBuffer : public Buffer
{
public:
    NODISCARD static VertexBuffer Create(eResourceAccess _access, UInt32 _vertexStride, UInt32 _vertexCount, const void* _pInitialData = nullptr);
    NODISCARD static VertexBuffer Create(eResourceAccess _access, eVertexType _vertexType, UInt32 _vertexCount, const void* _pInitialData = nullptr);

    NODISCARD UInt32 GetStride() const { return m_stride; }
    NODISCARD UInt32 GetVertexCount() const { return m_stride == 0 ? 0 : m_byteWidth / m_stride; }
    void             Bind() const;

private:
    UInt32 m_stride = 0;   // vertex stride in bytes, used for binding
};

class IndexBuffer : public Buffer
{
public:
    NODISCARD static IndexBuffer Create(eResourceAccess _access, UInt32 _indexCount, const Index* _pInitialData = nullptr);
    NODISCARD constexpr UInt32   GetStride() const { return sizeof(UInt32); }
    NODISCARD UInt32             GetIndexCount() const { return m_byteWidth / GetStride(); }
    void                         Bind() const;
};

class ConstantBuffer : public Buffer
{
public:
    static NODISCARD ConstantBuffer Create(UInt32 _byteWidth, const void* _pInitialData = nullptr);
    void                            Bind(eShader _shader, UInt32 _slot) const;
};

}   // namespace jam
