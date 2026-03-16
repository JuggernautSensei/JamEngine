#pragma once

namespace Jam
{

enum class eSeekOp
{
    None,
    Set,
    Current,
    End
};

class MemoryIO
{
public:
    SizeT               Seek(SizeT _offset, eSeekOp _op) const;
    SizeT               Skip(SizeT _numBytes) const;
    [[nodiscard]] SizeT Tell() const;
    [[nodiscard]] SizeT Remaining() const;
    void                Rewind() const;
    [[nodiscard]] bool  IsEof() const;

protected:
    explicit MemoryIO(SizeT _byteWidth);

    mutable SizeT m_offset    = 0;
    SizeT         m_byteWidth = 0;
};

class MemoryWriter : public MemoryIO
{
public:
    explicit MemoryWriter(MemoryBlock _mem);
    SizeT Write(CMemoryBlock _src) const;

private:
    MemoryBlock m_mem = {};
};

class MemoryReader : public MemoryIO
{
public:
    explicit MemoryReader(CMemoryBlock _mem);
    SizeT Read(MemoryBlock _dst) const;

private:
    CMemoryBlock m_mem = {};
};

}   // namespace Jam