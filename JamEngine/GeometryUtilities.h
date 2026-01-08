#pragma once

namespace jam
{

class VertexWriter
{
    struct Elem   // tiny helper struct
    {
        Int32 offset    = 0;
        Int32 byteWidth = 0;
        bool  bValid    = false;
    };

public:
    VertexWriter() = default;

    explicit VertexWriter(MemView _vertData, const VertexLayout& _vl)
        : m_pBeginPos(_vertData.GetPtr())
        , m_pCurPos(_vertData.GetPtr())
        , m_pEndPos(m_pCurPos + _vertData.GetByteWidth())
        , m_stride(_vl.GetStride())
    {
        for (const VertexLayoutElement& vle: _vl.GetElements())
        {
            Elem elem                   = { vle.offset, vle.byteWidth, true };
            m_vles[IntFrom(vle.attrib)] = elem;
        }
    }

    void Next()
    {
        JAM_ASSERT(m_pCurPos + m_stride <= m_pEndPos, "VertexWriter: Out of bounds.");
        m_pCurPos += m_stride;
    }

    void MoveTo(const Int32 _idx)
    {
        JAM_ASSERT(m_pBeginPos + (_idx + 1) * m_stride <= m_pEndPos, "VertexWriter: Out of bounds.");
        m_pCurPos = m_pBeginPos + _idx * m_stride;
    }

    void Write(const eVertexAttribute _attrib, const CMemView _data)
    {
        JAM_ASSERT(m_pCurPos + m_stride <= m_pEndPos, "VertexWriter: Out of bounds.");

        const Elem& elem = m_vles[IntFrom(_attrib)];
        JAM_ASSERT(elem.bValid, "VertexWriter: Vertex attribute not defined in layout.");

        const Int32 byteWidth = Min(elem.byteWidth, _data.GetByteWidth());
        std::memcpy(m_pCurPos + elem.offset, _data.GetPtr(), byteWidth);
    }

    void WriteAt(const Int32 _idx, const eVertexAttribute _attrib, const CMemView _data)
    {
        JAM_ASSERT(m_pBeginPos + (_idx + 1) * m_stride <= m_pEndPos, "VertexWriter: Out of bounds.");

        const Elem& elem = m_vles[IntFrom(_attrib)];
        JAM_ASSERT(elem.bValid, "VertexWriter: Vertex attribute not defined in layout.");

        const Int32 byteWidth = Min(elem.byteWidth, _data.GetByteWidth());
        std::memcpy(m_pBeginPos + _idx * m_stride + elem.offset, _data.GetPtr(), byteWidth);
    }

    [[nodiscard]] bool IsEOF() const noexcept
    {
        return m_pCurPos == m_pEndPos;
    }

private:
    Byte* m_pBeginPos = nullptr;
    Byte* m_pCurPos   = nullptr;
    Byte* m_pEndPos   = nullptr;

    Array<Elem, NumEnums<eVertexAttribute>()> m_vles   = {};
    Int32                                    m_stride = 0;
};

class VertexReader
{
    struct Elem   // tiny helper struct
    {
        Int32 offset    = 0;
        Int32 byteWidth = 0;
        bool  bValid    = false;
    };

public:
    VertexReader() = default;

    explicit VertexReader(const CMemView _data, const VertexLayout& _vl)
        : m_pBeginPos(_data.GetPtr())
        , m_pCurPos(_data.GetPtr())
        , m_pEndPos(m_pCurPos + _data.GetByteWidth())
        , m_stride(_vl.GetStride())
    {
        JAM_ASSERT((_data.GetByteWidth() % m_stride) == 0, "VertexReader: Data size mismatch.");

        for (const VertexLayoutElement& vle: _vl.GetElements())
        {
            Elem elem                   = { vle.offset, vle.byteWidth, true };
            m_vles[IntFrom(vle.attrib)] = elem;
        }
    }

    void Next()
    {
        JAM_ASSERT(m_pCurPos + m_stride <= m_pEndPos, "VertexReader: Out of bounds.");
        m_pCurPos += m_stride;
    }

    void MoveTo(const Int32 _idx)
    {
        JAM_ASSERT(m_pBeginPos + (_idx + 1) * m_stride <= m_pEndPos, "VertexReader: Out of bounds.");
        m_pCurPos = m_pBeginPos + _idx * m_stride;
    }

    void Read(const eVertexAttribute _attrib, const MemView _outData) const
    {
        JAM_ASSERT(m_pCurPos + m_stride <= m_pEndPos, "VertexReader: Out of bounds.");

        const Elem& elem = m_vles[IntFrom(_attrib)];
        JAM_ASSERT(elem.bValid, "VertexReader: Vertex attribute not defined in layout.");

        const Int32 byteWidth = Min(elem.byteWidth, _outData.GetByteWidth());
        std::memcpy(_outData.GetPtr(), m_pCurPos + elem.offset, byteWidth);
    }

    void ReadAt(const Int32 _idx, const eVertexAttribute _attrib, const MemView _outData) const
    {
        JAM_ASSERT(m_pBeginPos + (_idx + 1) * m_stride <= m_pEndPos, "VertexReader: Out of bounds.");

        const Elem& elem = m_vles[IntFrom(_attrib)];
        JAM_ASSERT(elem.bValid, "VertexReader: Vertex attribute not defined in layout.");

        const Int32 byteWidth = Min(elem.byteWidth, _outData.GetByteWidth());
        std::memcpy(_outData.GetPtr(), m_pBeginPos + _idx * m_stride + elem.offset, byteWidth);
    }

    [[nodiscard]] bool IsEOF() const noexcept
    {
        return m_pCurPos == m_pEndPos;
    }

private:
    const Byte* m_pBeginPos = nullptr;
    const Byte* m_pCurPos   = nullptr;
    const Byte* m_pEndPos   = nullptr;

    Array<Elem, NumEnums<eVertexAttribute>()> m_vles   = {};
    Int32                                    m_stride = 0;
};

class IndexWriter
{
public:
    IndexWriter() = default;

    explicit IndexWriter(MemView _indexData, const bool _bUseU32)
        : m_pBeginPos(_indexData.GetPtr())
        , m_pCurPos(_indexData.GetPtr())
        , m_pEndPos(m_pCurPos + _indexData.GetByteWidth())
        , m_stride(_bUseU32 ? sizeof(UInt32) : sizeof(UInt16))
    {
    }

    void Next()
    {
        JAM_ASSERT(m_pCurPos + m_stride <= m_pEndPos, "IndexWriter: Out of bounds.");
        m_pCurPos += m_stride;
    }

    void MoveTo(const Int32 _idx)
    {
        JAM_ASSERT(m_pBeginPos + (_idx + 1) * m_stride <= m_pEndPos, "IndexWriter: Out of bounds.");
        m_pCurPos = m_pBeginPos + _idx * m_stride;
    }

    void Write(const UInt32 _indexValue)
    {
        JAM_ASSERT(m_pCurPos + m_stride <= m_pEndPos, "IndexWriter: Out of bounds.");
        if (m_stride == sizeof(UInt32))
        {
            *reinterpret_cast<UInt32*>(m_pCurPos) = _indexValue;
        }
        else
        {
            *reinterpret_cast<UInt16*>(m_pCurPos) = NarrowCast<UInt16>(_indexValue);
        }
    }

    void WriteAt(const Int32 _idx, const UInt32 _indexValue)
    {
        JAM_ASSERT(m_pBeginPos + (_idx + 1) * m_stride <= m_pEndPos, "IndexWriter: Out of bounds.");
        Byte* pTarget = m_pBeginPos + _idx * m_stride;
        if (m_stride == sizeof(UInt32))
        {
            *reinterpret_cast<UInt32*>(pTarget) = _indexValue;
        }
        else
        {
            *reinterpret_cast<UInt16*>(pTarget) = NarrowCast<UInt16>(_indexValue);
        }
    }

    [[nodiscard]] bool IsEOF() const noexcept
    {
        return m_pCurPos == m_pEndPos;
    }

private:
    Byte* m_pBeginPos = nullptr;
    Byte* m_pCurPos   = nullptr;
    Byte* m_pEndPos   = nullptr;
    Int32 m_stride    = 0;
};

class IndexReader
{
public:
    IndexReader() = default;

    explicit IndexReader(const CMemView _data, const bool _bUseU32)
        : m_pBeginPos(_data.GetPtr())
        , m_pCurPos(_data.GetPtr())
        , m_pEndPos(m_pCurPos + _data.GetByteWidth())
        , m_stride(_bUseU32 ? sizeof(UInt32) : sizeof(UInt16))
    {
        JAM_ASSERT((_data.GetByteWidth() % m_stride) == 0, "IndexReader: Data size mismatch.");
    }

    void Next()
    {
        JAM_ASSERT(m_pCurPos + m_stride <= m_pEndPos, "IndexReader: Out of bounds.");
        m_pCurPos += m_stride;
    }

    void MoveTo(const Int32 _idx)
    {
        JAM_ASSERT(m_pBeginPos + (_idx + 1) * m_stride <= m_pEndPos, "IndexReader: Out of bounds.");
        m_pCurPos = m_pBeginPos + _idx * m_stride;
    }

    [[nodiscard]] UInt32 Read() const
    {
        JAM_ASSERT(m_pCurPos + m_stride <= m_pEndPos, "IndexReader: Out of bounds.");
        if (m_stride == sizeof(UInt32))
        {
            return *reinterpret_cast<const UInt32*>(m_pCurPos);
        }
        else
        {
            return *reinterpret_cast<const UInt16*>(m_pCurPos);
        }
    }

    [[nodiscard]] UInt32 ReadAt(const Int32 _idx) const
    {
        JAM_ASSERT(m_pBeginPos + (_idx + 1) * m_stride <= m_pEndPos, "IndexReader: Out of bounds.");
        const Byte* pTarget = m_pBeginPos + _idx * m_stride;
        if (m_stride == sizeof(UInt32))
        {
            return *reinterpret_cast<const UInt32*>(pTarget);
        }
        else
        {
            return *reinterpret_cast<const UInt16*>(pTarget);
        }
    }

    [[nodiscard]] bool IsEOF() const noexcept
    {
        return m_pCurPos == m_pEndPos;
    }

private:
    const Byte* m_pBeginPos = nullptr;
    const Byte* m_pCurPos   = nullptr;
    const Byte* m_pEndPos   = nullptr;
    Int32       m_stride    = 0;
};

[[nodiscard]] Int32 CalcNumIndices(CMemView _idxData, bool _bUseU32) noexcept;
[[nodiscard]] Int32 CalcNumVertices(CMemView _vertData, const VertexLayout& _vl) noexcept;
[[nodiscard]] Int32 CalcIndicesByteWidth(Int32 _numIdxes, bool _bUseU32) noexcept;
[[nodiscard]] bool  IsGreaterThanUInt16(Int32 _value) noexcept;

}   // namespace jam