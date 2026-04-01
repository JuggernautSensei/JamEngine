#pragma once
#include "MemoryView.h"

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

class FileIO
{
public:
    ~FileIO();
    FileIO(const FileIO&)            = delete;
    FileIO& operator=(const FileIO&) = delete;
    FileIO(FileIO&& _other) noexcept;
    FileIO& operator=(FileIO&& _other) noexcept;

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
    explicit FileIO(FILE* _pFile);

    FILE* m_pFile = nullptr;
};

class FileWriter : public FileIO
{
public:
    [[nodiscard]] static Expect<FileWriter> Open(StringView _path, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite);
    static Error                            WriteFileAsBinary(MemoryView _data, StringView _path, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite);
    static Error                            WriteFileAsString(StringView _str, StringView _path, eFileWriterCollisionPolicy _policy = eFileWriterCollisionPolicy::Overwrite);

    Expect<SizeT> WriteBinary(MemoryView _data) const;
    Expect<SizeT> WriteString(StringView _str) const;   // if you want to write text. use this. WriteBinary write raw bytes, so it can be included null terminators.
    void          Flush() const;

private:
    explicit FileWriter(FILE* _pFile);
};

class FileReader : public FileIO
{
public:
    [[nodiscard]] static Expect<FileReader> Open(StringView _path);
    static Expect<Vector<Byte>>             ReadFileAsBinary(StringView _path);
    static Expect<String>                   ReadFileAsString(StringView _path);

    Expect<SizeT>      ReadBinary(MutableMemoryView _dst, SizeT _count = 0) const;   // zero is treated as "read as much as possible"
    Expect<String>     ReadString(SizeT _count = 0) const;                     // zero is treated as "read as much as possible"
    [[nodiscard]] bool IsEof() const;

private:
    explicit FileReader(FILE* _pFile);
};

}   // namespace Jam
