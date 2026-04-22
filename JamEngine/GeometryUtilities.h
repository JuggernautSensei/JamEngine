#pragma once
#include "VertexLayout.h"

namespace Jam
{

namespace Details
{
    struct TinyElem   // tiny helper struct
    {
        Int32 offset    = 0;
        Int32 byteWidth = 0;
        bool  bValid    = false;
    };

    using TinyLayout = EnumArray<eVertexAttribute, TinyElem>;

}   // namespace Details

class VertexWriter
{

public:
    explicit VertexWriter(
        const MutableMemoryView   _vertData,
        const VertexLayout& _vl)
        : m_mem(_vertData)
        , m_stride(_vl.GetStride())
    {
        for (const VertexLayoutElement& vle: _vl.GetElems())
        {
            Details::TinyElem elem = { vle.offset, vle.byteWidth, true };
            m_vles[vle.attrib]     = elem;
        }
    }

    template<typename T>
    void WriteAt(
        const SizeT            _vertIdx,
        const eVertexAttribute _attrib,
        const T                _value) const
    {
        const Details::TinyElem& elem = m_vles[_attrib];
        JAM_ASSERT(elem.bValid, "Vertex attribute is not valid in the given vertex layout.");
        const SizeT offset = _vertIdx * m_stride + elem.offset;
        JAM_ASSERT(sizeof(T) <= elem.byteWidth, "Value size does not match vertex attribute size.");
        JAM_ASSERT(m_mem.GetByteWidth() >= offset + elem.byteWidth, "Vertex attribute write out of bounds in vertex buffer.");
        std::memcpy(m_mem.GetPtr() + offset, &_value, sizeof(T));
    }

private:
    // seek must be placed vertex start
    MutableMemoryView         m_mem    = {};
    Details::TinyLayout m_vles   = {};
    Int32               m_stride = 0;
};

class VertexReader
{
public:
    VertexReader(
        const MemoryView  _vertData,
        const VertexLayout& _vl)
        : m_mem(_vertData)
        , m_stride(_vl.GetStride())
    {
        for (const VertexLayoutElement& vle: _vl.GetElems())
        {
            Details::TinyElem elem = { vle.offset, vle.byteWidth, true };
            m_vles[vle.attrib]     = elem;
        }
    }

    template<typename T>
    T ReadAt(
        const SizeT            _vertIdx,
        const eVertexAttribute _attrib) const
    {
        const Details::TinyElem& elem = m_vles[_attrib];
        JAM_ASSERT(elem.bValid, "Vertex attribute is not valid in the given vertex layout.");
        const SizeT offset = _vertIdx * m_stride + elem.offset;
        JAM_ASSERT(sizeof(T) <= elem.byteWidth, "Value size does not match vertex attribute size.");
        JAM_ASSERT(m_mem.GetByteWidth() >= offset + elem.byteWidth, "Vertex attribute read out of bounds in vertex buffer.");
        T value = {};
        std::memcpy(&value, m_mem.GetPtr() + offset, sizeof(T));
        return value;
    }

private:
    MemoryView        m_mem;
    Details::TinyLayout m_vles   = {};
    Int32               m_stride = 0;
};

class IndexWriter
{
public:
    IndexWriter(
        const MutableMemoryView _idxData,
        const bool        _bUseU32)
        : m_mem(_idxData)
        , m_bU32(_bUseU32)
    {
    }

    template<typename T>
        requires(std::is_integral_v<T>)
    void WriteAt(
        const SizeT _idxIdx,
        const T     _value) const
    {
        if (m_bU32)
        {
            JAM_ASSERT(std::cmp_greater_equal(m_mem.GetByteWidth(), (_idxIdx + 1) * sizeof(UInt32)), "Index out of bounds in index buffer.");
            const UInt32 idx = static_cast<UInt32>(_value);
            std::memcpy(m_mem.GetPtr() + _idxIdx * sizeof(UInt32), &idx, sizeof(UInt32));
        }
        else
        {
            JAM_ASSERT(std::cmp_greater_equal(m_mem.GetByteWidth(), (_idxIdx + 1) * sizeof(UInt16)), "Index out of bounds in index buffer.");
            const UInt16 idx = static_cast<UInt16>(_value);
            std::memcpy(m_mem.GetPtr() + _idxIdx * sizeof(UInt16), &idx, sizeof(UInt16));
        }
    }

private:
    MutableMemoryView m_mem  = {};
    bool        m_bU32 = false;
};

class IndexReader
{
public:
    IndexReader(
        const MemoryView _idxData,
        const bool         _bUseU32)
        : m_mem(_idxData)
        , m_bU32(_bUseU32)
    {
    }

    template<typename T>
        requires(std::is_integral_v<T>)
    T ReadAt(
        const SizeT _idxIdx) const
    {
        if (m_bU32)
        {
            JAM_ASSERT(std::cmp_greater_equal(m_mem.GetByteWidth(), (_idxIdx + 1) * sizeof(UInt32)), "Index out of bounds in index buffer.");
            UInt32 idx = 0;
            std::memcpy(&idx, m_mem.GetPtr() + _idxIdx * sizeof(UInt32), sizeof(UInt32));
            return static_cast<T>(idx);
        }
        else
        {
            JAM_ASSERT(std::cmp_greater_equal(m_mem.GetByteWidth(), (_idxIdx + 1) * sizeof(UInt16)), "Index out of bounds in index buffer.");
            UInt16 idx = 0;
            std::memcpy(&idx, m_mem.GetPtr() + _idxIdx * sizeof(UInt16), sizeof(UInt16));
            return static_cast<T>(idx);
        }
    }

private:
    MemoryView m_mem  = {};
    bool         m_bU32 = false;
};

[[nodiscard]] Int32 CalcNumIndices(MemoryView _idxData, bool _bUseU32) noexcept;
[[nodiscard]] Int32 CalcNumVertices(MemoryView _vertData, const VertexLayout& _vl) noexcept;
[[nodiscard]] Int32 CalcIndicesByteWidth(Int32 _numIdxes, bool _bUseU32) noexcept;

}   // namespace Jam