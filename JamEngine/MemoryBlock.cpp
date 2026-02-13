#include "pch.h"
#include "MemoryBlock.h"

namespace Jam
{

MemoryBlock::MemoryBlock(
    Byte* const _pData,
    const SizeT _byteWidth)
    : m_pData(_pData)
    , m_size(_byteWidth)
{
}

MemoryBlock::MemoryBlock(
    void*       _pData,
    const SizeT _byteWidth)
    : m_pData(static_cast<Byte*>(_pData))
    , m_size(_byteWidth)
{
}

Byte* MemoryBlock::GetPtr() const
{
    return m_pData;
}

SizeT MemoryBlock::GetByteWidth() const
{
    return m_size;
}

MemoryBlock MemoryBlock::SubBlock(const SizeT _offset) const
{
    return SubBlock(_offset, m_size - _offset);
}

MemoryBlock MemoryBlock::SubBlock(const SizeT _offset, const SizeT _byteWidth) const
{
    JAM_ASSERT(_offset >= 0 && _byteWidth >= 0, "Offset and byte width must be non-negative.");
    JAM_ASSERT(_offset + _byteWidth <= m_size, "SubBlock exceeds memory block size.");
    return MemoryBlock { m_pData + _offset, _byteWidth };
}

bool MemoryBlock::IsEmpty() const
{
    return m_pData == nullptr || m_size == 0;
}

CMemoryBlock::CMemoryBlock(
    const Byte* const _pData,
    const SizeT       _byteWidth)
    : m_pData(_pData)
    , m_size(_byteWidth)
{
}

CMemoryBlock::CMemoryBlock(
    const void* _pData,
    const SizeT _byteWidth)
    : m_pData(static_cast<const Byte*>(_pData))
    , m_size(_byteWidth)
{
}

CMemoryBlock::CMemoryBlock(
    const MemoryBlock _mem)
    : m_pData(_mem.GetPtr())
    , m_size(_mem.GetByteWidth())
{
}

const Byte* CMemoryBlock::GetPtr() const
{
    return m_pData;
}

SizeT CMemoryBlock::GetByteWidth() const
{
    return m_size;
}

CMemoryBlock CMemoryBlock::SubBlock(const SizeT _offset) const
{
    return SubBlock(_offset, m_size - _offset);
}

CMemoryBlock CMemoryBlock::SubBlock(const SizeT _offset, const SizeT _byteWidth) const
{
    JAM_ASSERT(_offset >= 0 && _byteWidth >= 0, "Offset and byte width must be non-negative.");
    JAM_ASSERT(_offset + _byteWidth <= m_size, "SubBlock exceeds memory block size.");
    return CMemoryBlock { m_pData + _offset, _byteWidth };
}

bool CMemoryBlock::IsEmpty() const
{
    return m_pData == nullptr || m_size == 0;
}

}   // namespace Jam