#pragma once
#include "MemoryBlock.h"

namespace Jam
{
enum class eSeekOp;

enum class eSaveFileFlags : UInt32
{
    None         = 0,
    FailIfExists = 1 << 0,
    CreateDirs   = 1 << 1,
    Append       = 1 << 2,
};

[[nodiscard]] Error ResolveSaveFileFlags(const Path& _path, Flags<eSaveFileFlags> _flags);

class FileRWBase
{
public:
    ~FileRWBase();
    FileRWBase(const FileRWBase&)            = delete;
    FileRWBase& operator=(const FileRWBase&) = delete;
    FileRWBase(FileRWBase&& _other) noexcept;
    FileRWBase& operator=(FileRWBase&& _other) noexcept;

    void                Seek(Int32 _offset, eSeekOp _op) const;
    void                Skip(Int32 _numBytes) const;
    [[nodiscard]] Int32 Tell() const;
    [[nodiscard]] Int32 Remaining() const;
    void                Rewind() const;

    [[nodiscard]] Int32 CalcTotalSize() const;
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
    [[nodiscard]] static Expect<FileWriter> Open(const Path& _path, Flags<eSaveFileFlags> _flags = {});
    static Error                            WriteFileFromMemory(CMemoryBlock _data, const Path& _path, Flags<eSaveFileFlags> _flags = {});
    static Error                            WriteFileFromString(StringView _str, const Path& _path, Flags<eSaveFileFlags> _flags = {});

    Error Write(CMemoryBlock _data) const;
    Error Write(StringView _str) const;
    void  Flush() const;

private:
    explicit FileWriter(FILE* _pFile);
};

class FileReader : public FileRWBase
{
public:
    [[nodiscard]] static Expect<FileReader> Open(const Path& _path);
    static Expect<Vector<Byte>>             ReadFileAsMemory(const Path& _path);
    static Expect<String>                   ReadFileAsString(const Path& _path);

    Error              Read(MemoryBlock _dst) const;
    [[nodiscard]] bool IsEof() const;

private:
    explicit FileReader(FILE* _pFile);
};

}   // namespace Jam
