#pragma once
#include "String.h"
#include "Error.h"
#include "MemoryView.h"

namespace Jam
{

enum class eSeekOp
{
    Set,
    Current,
    End
};

class FileBase
{
public:
    ~FileBase();
    FileBase(const FileBase&)            = delete;
    FileBase& operator=(const FileBase&) = delete;
    FileBase(FileBase&& _other) noexcept;
    FileBase& operator=(FileBase&& _other) noexcept;

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
    explicit FileBase(FILE* _pFile);

    FILE* m_pFile = nullptr;
};

enum class eFileWrite
{
    Truncate,       // 기존 내용을 모두 지우고 새로 쓰기
    Append,         // 이어쓰기
    Rename,         // 파일이 존재하면 이름을 바꿔서 새로 쓰기
    FailIfExists,   // 파일이 존재하면 실패
};

class FileWriter : public FileBase
{
public:
    [[nodiscard]] static Expect<FileWriter> Open(StringView _path, eFileWrite _mode = eFileWrite::Truncate);

    Expect<SizeT> Write(MemoryView _data) const;
    void          Flush() const;

private:
    explicit FileWriter(FILE* _pFile);
};

class FileReader : public FileBase
{
public:
    [[nodiscard]] static Expect<FileReader> Open(StringView _path);

    Expect<SizeT>      Read(MutableMemoryView _dst, SizeT _count = 0) const;   // zero is treated as "read as much as possible"
    [[nodiscard]] bool IsEof() const;

private:
    explicit FileReader(FILE* _pFile);
};

}   // namespace Jam
