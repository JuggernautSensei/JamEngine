#include "pch.h"
#include "File.h"

#include "MemoryReadWriter.h"

namespace Jam
{

Error ResolveSaveFileFlags(
    const Path&                 _path,
    const Flags<eSaveFileFlags> _flags)
{
    if (_flags & eSaveFileFlags::FailIfExists)
    {
        if (fs::exists(_path))
        {
            JAM_LOG.Error("File '{}' already exists.", _path.native());
            return Error::AlreadyExists;
        }
    }

    if (_flags & eSaveFileFlags::CreateDirs)
    {
        const Path parent = _path.parent_path();
        if (!parent.empty() && !fs::exists(parent))
        {
            std::error_code ec = {};
            fs::create_directories(parent, ec);
            if (ec)
            {
                JAM_LOG.Error("Failed to create directories for path '{}'. {}", parent.native(), ec.message());
                return Error::DirectoryCreationFailed;
            }
        }
    }

    return Error::None;
}

FileRWBase::~FileRWBase()
{
    Close();
}

FileRWBase::FileRWBase(
    FileRWBase&& _other) noexcept
    : m_pFile(_other.m_pFile)
{
    _other.m_pFile = nullptr;
}

FileRWBase& FileRWBase::operator=(
    FileRWBase&& _other) noexcept
{
    if (this != &_other)
    {
        Close();
        m_pFile        = _other.m_pFile;
        _other.m_pFile = nullptr;
    }
    return *this;
}

void FileRWBase::Seek(
    const Int32   _offset,
    const eSeekOp _op) const
{
    Int32 origin = 0;
    switch (_op)
    {
        case eSeekOp::Set:
            origin = SEEK_SET;
            break;
        case eSeekOp::Current:
            origin = SEEK_CUR;
            break;
        case eSeekOp::End:
            origin = SEEK_END;
            break;
        default:
            JAM_UNREACHABLE;
    }

    if (::fseek(m_pFile, _offset, origin) != 0)
    {
        JAM_UNREACHABLE;
    }
}

void FileRWBase::Skip(
    const Int32 _numBytes) const
{
    Seek(_numBytes, eSeekOp::Current);
}

Int32 FileRWBase::Tell() const
{
    const long pos = ::ftell(m_pFile);
    JAM_ASSERT(pos >= 0, "Failed to tell file position.");
    return NumericCast<Int32>(pos);
}

Int32 FileRWBase::Remaining() const
{
    const Int32 cur = Tell();
    Seek(0, eSeekOp::End);
    const Int32 end = Tell();
    Seek(cur, eSeekOp::Set);
    return end - cur;
}

void FileRWBase::Rewind() const
{
    Seek(0, eSeekOp::Set);
}

Int32 FileRWBase::CalcTotalSize() const
{
    const Int32 cur = Tell();
    Seek(0, eSeekOp::End);
    const Int32 end = Tell();
    Seek(cur, eSeekOp::Set);
    return end;
}

void FileRWBase::Close()
{
    if (m_pFile)
    {
        (void)::fclose(m_pFile);
        m_pFile = nullptr;
    }
}

bool FileRWBase::IsClosed() const
{
    return m_pFile == nullptr;
}

bool FileRWBase::IsError() const
{
    return m_pFile ? (::ferror(m_pFile) != 0) : true;
}

FILE* FileRWBase::GetFilePtr() const
{
    return m_pFile;
}

FileRWBase::FileRWBase(FILE* _pFile)
    : m_pFile(_pFile)
{
}

Expect<FileWriter> FileWriter::Open(
    const Path&                 _path,
    const Flags<eSaveFileFlags> _flags)
{
    if (const Error err = ResolveSaveFileFlags(_path, _flags))
    {
        return err;
    }

    const char* mode  = (_flags & eSaveFileFlags::Append) ? "ab" : "wb";
    FILE*       pFile = nullptr;
    if (::fopen_s(&pFile, _path.c_str(), mode) != 0)
    {
        return Error::OpenFailed;
    }

    return FileWriter { pFile };
}

Error FileWriter::WriteFileFromMemory(
    const CMemoryBlock          _data,
    const Path&                 _path,
    const Flags<eSaveFileFlags> _flags)
{
    Expect<FileWriter> writer = Open(_path, _flags);
    if (!writer)
    {
        return writer.GetError();
    }

    return writer->Write(_data);
}

Error FileWriter::WriteFileFromString(
    const StringView            _str,
    const Path&                 _path,
    const Flags<eSaveFileFlags> _flags)
{
    return WriteFileFromMemory(CMemoryBlock { _str.GetPtr(), _str.GetLength() }, _path, _flags);
}

Error FileWriter::Write(
    const CMemoryBlock _data) const
{
    JAM_ASSERT(m_pFile, "File handle is null.");
    const size_t written = ::fwrite(_data.GetPtr(), 1, _data.GetByteWidth(), m_pFile);
    if (CmpLess(written, _data.GetByteWidth()))
    {
        JAM_LOG.Error("Failed to write {} bytes to file, only {} bytes were written.", _data.GetByteWidth(), written);
        return Error::WriteFailed;
    }
    return Error::None;
}

Error FileWriter::Write(
    const StringView _str) const
{
    return Write(CMemoryBlock { _str.GetPtr(), _str.GetLength() });
}

void FileWriter::Flush() const
{
    JAM_ASSERT(m_pFile, "File handle is null.");
    (void)::fflush(m_pFile);
}

FileWriter::FileWriter(FILE* _pFile)
    : FileRWBase(_pFile)
{
}

Expect<FileReader> FileReader::Open(
    const Path& _path)
{
    FILE* pFile = nullptr;
    if (::fopen_s(&pFile, _path.c_str(), "rb") != 0)
    {
        JAM_LOG.Error("Failed to open file '{}'.", _path.native());
        return Error::OpenFailed;
    }

    return FileReader { pFile };
}

Expect<Vector<Byte>> FileReader::ReadFileAsMemory(
    const Path& _path)
{
    Expect<FileReader> reader = Open(_path);
    if (!reader)
    {
        return reader.GetError();
    }
    const Int32  totalSize = reader->CalcTotalSize();
    Vector<Byte> data(totalSize);
    if (const Error err = reader->Read(data))
    {
        return err;
    }
    return data;
}

Expect<String> FileReader::ReadFileAsString(
    const Path& _path)
{
    Expect<FileReader> reader = Open(_path);
    if (!reader)
    {
        return reader.GetError();
    }
    const Int32 totalSize = reader->CalcTotalSize();
    String      str { totalSize, '\0' };
    if (const Error err = reader->Read(MemoryBlock { str.GetPtr(), totalSize }))
    {
        return err;
    }
    return str;
}

Error FileReader::Read(
    const MemoryBlock _dst) const
{
    JAM_ASSERT(m_pFile, "File handle is null.");
    const size_t read = ::fread(_dst.GetPtr(), 1, _dst.GetByteWidth(), m_pFile);
    if (CmpLess(read, _dst.GetByteWidth()))
    {
        if (IsEof())
        {
            JAM_LOG.Error("Reached end of file while reading {} bytes, only {} bytes were read.", _dst.GetByteWidth(), read);
        }
        else
        {
            JAM_LOG.Error("Failed to read {} bytes from file, only {} bytes were read.", _dst.GetByteWidth(), read);
        }
        return Error::ReadFailed;
    }

    return Error::None;
}

bool FileReader::IsEof() const
{
    return m_pFile ? (::feof(m_pFile) != 0) : true;
}

FileReader::FileReader(FILE* _pFile)
    : FileRWBase(_pFile)
{
}

}   // namespace Jam