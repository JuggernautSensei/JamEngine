#include "pch.h"
#include "MemoryIO.h"

namespace Jam
{

SizeT MemoryIO::Seek(
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

SizeT MemoryIO::Skip(
    const SizeT _numBytes) const
{
    return Seek(_numBytes, eSeekOp::Current);
}

SizeT MemoryIO::Tell() const
{
    return m_offset;
}

SizeT MemoryIO::Remaining() const
{
    return m_byteWidth - m_offset;
}

void MemoryIO::Rewind() const
{
    m_offset = 0;
}

bool MemoryIO::IsEof() const
{
    return m_offset >= m_byteWidth;
}

MemoryIO::MemoryIO(
    const SizeT _byteWidth)
    : m_byteWidth(_byteWidth)
{
}

MemoryWriter::MemoryWriter(
    const MutableMemoryView _mem)
    : MemoryIO(_mem.GetByteWidth())
    , m_mem(_mem)
{
}

SizeT MemoryWriter::Write(
    const MemoryView _src) const
{
    JAM_ASSERT(m_byteWidth >= m_offset + _src.GetByteWidth(), "Not enough space in memory block to write data.");
    const SizeT byteWidth = _src.GetByteWidth();
    std::memcpy(m_mem.GetPtr() + m_offset, _src.GetPtr(), byteWidth);
    return Seek(byteWidth, eSeekOp::Current);
}

MemoryReader::MemoryReader(
    const MemoryView _mem)
    : MemoryIO(_mem.GetByteWidth())
    , m_mem(_mem)
{
}

SizeT MemoryReader::Read(
    const MutableMemoryView _dst) const
{
    JAM_ASSERT(m_byteWidth >= m_offset + _dst.GetByteWidth(), "Not enough space in memory block to read data.");
    const SizeT byteWidth = _dst.GetByteWidth();
    std::memcpy(_dst.GetPtr(), m_mem.GetPtr() + m_offset, byteWidth);
    return Seek(byteWidth, eSeekOp::Current);
}

}   // namespace Jam