#pragma once
#include "VertexLayout.h"

namespace Jam
{

class VertexWriter
{
public:
    explicit VertexWriter(
        const MutableMemoryView _data,
        const VertexLayout&     _vl)
        : m_mem(_data)
        , m_stride(_vl.GetStride())
    {
        for (const VertexElement& ve: _vl.GetElements())
        {
            m_offsets[ve.attrib]     = ve.offset;
            m_byteWidths[ve.attrib]  = ve.byteWidth;
            m_validations[ve.attrib] = true;
        }
    }

    template<typename T>
    void WriteAt(
        const SizeT            _index,
        const eVertexAttribute _attrib,
        const T                _value) const
    {
        JAM_ASSERT(m_validations[_attrib], "Vertex attribute is not valid in the given vertex layout.");
        const SizeT offset = _index * m_stride + m_offsets[_attrib];
        JAM_ASSERT(sizeof(T) <= m_byteWidths[_attrib], "Value size does not match vertex attribute size.");
        JAM_ASSERT(m_mem.GetByteWidth() >= offset + m_byteWidths[_attrib], "Vertex attribute write out of bounds in vertex buffer.");
        std::memcpy(m_mem.GetPtr() + offset, &_value, sizeof(T));
    }

private:
    // seek must be placed vertex start
    MutableMemoryView                  m_mem         = {};
    Int32                              m_stride      = 0;
    EnumArray<eVertexAttribute, Int32> m_offsets     = {};
    EnumArray<eVertexAttribute, Int32> m_byteWidths  = {};
    EnumArray<eVertexAttribute, bool>  m_validations = {};
};

class VertexReader
{
public:
    VertexReader(
        const MemoryView    _data,
        const VertexLayout& _vl)
        : m_mem(_data)
        , m_stride(_vl.GetStride())
    {
        for (const VertexElement& ve: _vl.GetElements())
        {
            m_offsets[ve.attrib]     = ve.offset;
            m_byteWidths[ve.attrib]  = ve.byteWidth;
            m_validations[ve.attrib] = true;
        }
    }

    template<typename T>
    [[nodiscard]] T ReadAt(
        const SizeT            _index,
        const eVertexAttribute _attrib) const
    {
        JAM_ASSERT(m_validations[_attrib], "Vertex attribute is not valid in the given vertex layout.");
        const SizeT offset = _index * m_stride + m_offsets[_attrib];
        JAM_ASSERT(sizeof(T) <= m_byteWidths[_attrib], "Value size does not match vertex attribute size.");
        JAM_ASSERT(m_mem.GetByteWidth() >= offset + m_byteWidths[_attrib], "Vertex attribute read out of bounds in vertex buffer.");
        return *reinterpret_cast<const T*>(m_mem.GetPtr() + offset);
    }

private:
    MemoryView                         m_mem;
    Int32                              m_stride      = 0;
    EnumArray<eVertexAttribute, Int32> m_offsets     = {};
    EnumArray<eVertexAttribute, Int32> m_byteWidths  = {};
    EnumArray<eVertexAttribute, bool>  m_validations = {};
};

class IndexWriter
{
public:
    IndexWriter(
        const MutableMemoryView _index,
        const bool              _bUseU32)
        : m_mem(_index)
        , m_bU32(_bUseU32)
    {
    }

    template<typename T>
        requires(std::is_integral_v<T>)
    void WriteAt(
        const SizeT _index,
        const T     _value) const
    {
        if (m_bU32)
        {
            JAM_ASSERT(m_mem.GetByteWidth() >= static_cast<SizeT>((_index + 1) * sizeof(UInt32)), "Index out of bounds in index buffer.");
            const UInt32 index = static_cast<UInt32>(_value);
            std::memcpy(m_mem.GetPtr() + _index * sizeof(UInt32), &index, sizeof(UInt32));
        }
        else
        {
            JAM_ASSERT(m_mem.GetByteWidth() >= static_cast<SizeT>((_index + 1) * sizeof(UInt16)), "Index out of bounds in index buffer.");
            const UInt16 index = static_cast<UInt16>(_value);
            std::memcpy(m_mem.GetPtr() + _index * sizeof(UInt16), &index, sizeof(UInt16));
        }
    }

private:
    MutableMemoryView m_mem  = {};
    bool              m_bU32 = false;
};

class IndexReader
{
public:
    IndexReader(
        const MemoryView _data,
        const bool       _bUseU32)
        : m_mem(_data)
        , m_bU32(_bUseU32)
    {
    }

    template<typename T>
        requires(std::is_integral_v<T>)
    T ReadAt(
        const SizeT _index) const
    {
        if (m_bU32)
        {
            JAM_ASSERT(m_mem.GetByteWidth() >= static_cast<SizeT>((_index + 1) * sizeof(UInt32)), "Index out of bounds in index buffer.");
            return *reinterpret_cast<const UInt32*>(m_mem.GetPtr() + _index * sizeof(UInt32));
        }
        else
        {
            JAM_ASSERT(m_mem.GetByteWidth() >= static_cast<SizeT>((_index + 1) * sizeof(UInt16)), "Index out of bounds in index buffer.");
            return *reinterpret_cast<const UInt16*>(m_mem.GetPtr() + _index * sizeof(UInt16));
        }
    }

private:
    MemoryView m_mem  = {};
    bool       m_bU32 = false;
};

}   // namespace Jam