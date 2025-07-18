#pragma once
#include "RendererCommons.h"

namespace jam
{

struct BufferInitializeData
{
    UInt32      byteWidth    = 0;
    const void* pInitialData = nullptr;
};

class Buffer
{
public:
    // utility functions
    void      CopyFrom(const Buffer& _other);
    void      Upload(UInt32 _dataByteWidth, const void* _pData, UInt32 _offset = 0);
    NODISCARD std::optional<std::vector<UInt8>> Download() const;

    // getters
    NODISCARD UInt32 GetByteWidth() const { return m_byteWidth; }
    NODISCARD UInt32 GetUsingByte() const { return m_usingByte; }
    NODISCARD bool   IsDynamic() const { return m_access == eResourceAccess::CPUWriteable; }   // dynamic buffer

    // d3d11 buffer
    NODISCARD ID3D11Buffer*        Get() const { return m_buffer.Get(); }
    NODISCARD ID3D11Buffer* const* GetAddressOf() const { return m_buffer.GetAddressOf(); }

protected:
    void Initialize_(UINT _flags, UInt32 _byteWidth, eResourceAccess _access, const BufferInitializeData* _initializeData_orNull = nullptr);
    Buffer() = default;

    // instance
    ComPtr<ID3D11Buffer> m_buffer;

    // buffer information
    UInt32          m_byteWidth = 0;   // buffer capacity
    UInt32          m_usingByte = 0;   // currently used byte size, can be less than m_byteWidth
    eResourceAccess m_access    = eResourceAccess::Immutable;
};

class VertexBuffer : public Buffer
{
public:
    static NODISCARD VertexBuffer Create(UInt32 _vertexStride, UInt32 _vertexCount, eResourceAccess _access, const BufferInitializeData* _vertexData_orNull = nullptr);
    NODISCARD UInt32              GetStride() const { return m_stride; }
    NODISCARD UInt32              GetVertexCount() const { return m_usingByte / m_stride; }
    void                          Bind() const;

private:
    UInt32 m_stride = 0;   // vertex stride in bytes, used for binding
};

class IndexBuffer : public Buffer
{
public:
    static NODISCARD IndexBuffer Create(bool _bUseExtendedIndex, UInt32 _indexCount, eResourceAccess _access, const BufferInitializeData* _indexData_orNull = nullptr);
    NODISCARD UInt32             GetStride() const { return m_bUseExtendedIndex ? sizeof(UInt32) : sizeof(UInt16); }
    NODISCARD UInt32             GetIndexCount() const { return m_usingByte / GetStride(); }
    void                         Bind() const;

private:
    bool m_bUseExtendedIndex = false;   // true if using 32-bit indices, false for 16-bit
};

class ConstantBuffer : public Buffer
{
public:
    static NODISCARD ConstantBuffer Create(UInt32 _byteWidth, const BufferInitializeData* _intialData_orNull = nullptr);
    void                            Bind(eShader _shader, UInt32 _slot) const;
};

}   // namespace jam
