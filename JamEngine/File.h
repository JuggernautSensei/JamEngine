#pragma once
#include "MemoryBlock.h"

namespace Jam
{

enum class eSeekOp;

enum class eFileWriterCollisionPolicy
{
    Overwrite,
    Rename,
    Fail,
    Append
};

class FileRWBase
{
public:
    ~FileRWBase();
    FileRWBase(const FileRWBase&)            = delete;
    FileRWBase& operator=(const FileRWBase&) = delete;
    FileRWBase(FileRWBase&& _other) noexcept;
    FileRWBase& operator=(FileRWBase&& _other) noexcept;

    SizeT               Seek(SizeT _offset, eSeekOp _op) const;
    SizeT               Skip(SizeT _numBytes) const;
    [[nodiscard]] SizeT Tell() const;
    [[nodiscard]] SizeT Remaining() const;
    void                Rewind() const;

    [[nodiscard]] SizeT CalcTotalSize() const;
    void                Close();
    [[nodiscard]] bool  IsClosed() const;
    [[nodiscard]] bool  IsError() const;
    [[nodiscard]] FILE* GetFilePtr() const;

protected:
    explicit FileRWBase(FILE* _pFile);

    FILE* m_pFile = nullptr;
};

class FileWriter : public FileRWBase
{
public:
    [[nodiscard]] static Expect<FileWriter> Open(StringView _path, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite);
    static Error                            WriteFileFromMemory(CMemoryBlock _data, StringView _path, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite);
    static Error                            WriteFileFromString(StringView _str, StringView _path, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite);

    Expect<SizeT> Write(CMemoryBlock _data) const;
    Expect<SizeT> Write(StringView _str) const;
    void          Flush() const;

private:
    explicit FileWriter(FILE* _pFile);
};

class FileReader : public FileRWBase
{
public:
    [[nodiscard]] static Expect<FileReader> Open(StringView _path);
    static Expect<Vector<Byte>>             ReadFileAsMemory(StringView _path);
    static Expect<String>                   ReadFileAsString(StringView _path);

    Expect<SizeT>      Read(MemoryBlock _dst) const;
    [[nodiscard]] bool IsEof() const;

private:
    explicit FileReader(FILE* _pFile);
};

}   // namespace Jam
