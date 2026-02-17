#include "pch.h"
#include "MemoryReadWriter.h"

namespace Jam
{

SizeT MemoryReadWriterBase::Seek(
    const SizeT   _offset,
    const eSeekOp _op) const
{
    switch (_op)
    {
        case eSeekOp::Set:
            m_offset = _offset;
            break;
        case eSeekOp::Current:
            m_offset += _offset;
            break;
        case eSeekOp::End:
            JAM_ASSERT(_offset <= 0, "Offset from end must be non-positive.");
            m_offset = m_byteWidth + _offset;
            break;
        default:
            JAM_UNREACHABLE;
    }

    JAM_ASSERT(m_offset >= 0 && m_offset <= m_byteWidth, "Seek resulted in invalid offset.");
    return m_offset;
}

SizeT MemoryReadWriterBase::Skip(
    const SizeT _numBytes) const
{
    return Seek(_numBytes, eSeekOp::Current);
}

SizeT MemoryReadWriterBase::Tell() const
{
    return m_offset;
}

SizeT MemoryReadWriterBase::Remaining() const
{
    return m_byteWidth - m_offset;
}

void MemoryReadWriterBase::Rewind() const
{
    m_offset = 0;
}

bool MemoryReadWriterBase::IsEof() const
{
    return m_offset >= m_byteWidth;
}

MemoryReadWriterBase::MemoryReadWriterBase(
    const SizeT _byteWidth)
    : m_byteWidth(_byteWidth)
{
}

MemoryWriter::MemoryWriter(
    const MemoryBlock _mem)
    : MemoryReadWriterBase(_mem.GetByteWidth())
    , m_mem(_mem)
{
}

SizeT MemoryWriter::Write(
    const CMemoryBlock _src) const
{
    JAM_ASSERT(m_byteWidth >= m_offset + _src.GetByteWidth(), "Not enough space in memory block to write data.");
    const SizeT byteWidth = _src.GetByteWidth();
    std::memcpy(m_mem.GetPtr() + m_offset, _src.GetPtr(), byteWidth);
    return Seek(byteWidth, eSeekOp::Current);
}

MemoryReader::MemoryReader(
    const CMemoryBlock _mem)
    : MemoryReadWriterBase(_mem.GetByteWidth())
    , m_mem(_mem)
{
}

SizeT MemoryReader::Read(
    const MemoryBlock _dst) const
{
    JAM_ASSERT(m_byteWidth >= m_offset + _dst.GetByteWidth(), "Not enough space in memory block to read data.");
    const SizeT byteWidth = _dst.GetByteWidth();
    std::memcpy(_dst.GetPtr(), m_mem.GetPtr() + m_offset, byteWidth);
    return Seek(byteWidth, eSeekOp::Current);
}

}   // namespace Jam