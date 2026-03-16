#include "pch.h"
#include "File.h"

#include "FileSystem.h"
#include "MemoryIO.h"
#include "WindowsUtilities.h"

namespace Jam
{

FileIO::~FileIO()
{
    Close();
}

FileIO::FileIO(
    FileIO&& _other) noexcept
    : m_pFile(_other.m_pFile)
{
    _other.m_pFile = nullptr;
}

FileIO& FileIO::operator=(
    FileIO&& _other) noexcept
{
    if (this != &_other)
    {
        Close();
        m_pFile        = _other.m_pFile;
        _other.m_pFile = nullptr;
    }
    return *this;
}

SizeT FileIO::Seek(
    const SizeT   _offset,
    const eSeekOp _op) const
{
    SizeT origin = 0;
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

    if (::_fseeki64(m_pFile, _offset, NumericCast<int>(origin)) != 0)
    {
        JAM_ASSERT(0, "Failed to seek file position.");
    }

    return Tell();
}

SizeT FileIO::Skip(
    const SizeT _numBytes) const
{
    return Seek(_numBytes, eSeekOp::Current);
}

SizeT FileIO::Tell() const
{
    const SizeT pos = ::_ftelli64(m_pFile);
    JAM_ASSERT(pos >= 0, "Failed to tell file position.");
    return pos;
}

SizeT FileIO::Remaining() const
{
    const SizeT cur = Tell();
    Seek(0, eSeekOp::End);
    const SizeT end = Tell();
    Seek(cur, eSeekOp::Set);
    return end - cur;
}

void FileIO::Rewind() const
{
    Seek(0, eSeekOp::Set);
}

SizeT FileIO::CalcTotalSize() const
{
    const SizeT cur = Tell();
    Seek(0, eSeekOp::End);
    const SizeT end = Tell();
    Seek(cur, eSeekOp::Set);
    return end;
}

void FileIO::Close()
{
    if (m_pFile)
    {
        (void)::fclose(m_pFile);
        m_pFile = nullptr;
    }
}

bool FileIO::IsClosed() const
{
    return m_pFile == nullptr;
}

bool FileIO::IsError() const
{
    return m_pFile ? (::ferror(m_pFile) != 0) : true;
}

FILE* FileIO::GetFilePtr() const
{
    return m_pFile;
}

FileIO::FileIO(FILE* _pFile)
    : m_pFile(_pFile)
{
}

Expect<FileWriter> FileWriter::Open(
    const StringView                 _path,
    const eFileWriterCollisionPolicy _policy)
{
    // Handle file name collision based on the specified policy.
    if (_policy != eFileWriterCollisionPolicy::Overwrite && _policy != eFileWriterCollisionPolicy::Append && FileSystem::Exists(_path))
    {
        switch (_policy)
        {
            case eFileWriterCollisionPolicy::Rename:
                return Open(FileSystem::GenerateUniquePath(_path), eFileWriterCollisionPolicy::Overwrite);

            case eFileWriterCollisionPolicy::Fail:
                return Error::AlreadyExists;

            case eFileWriterCollisionPolicy::Append:
            case eFileWriterCollisionPolicy::Overwrite:
            default:
                JAM_UNREACHABLE;
        }
    }

    // create directories if not exist
    const StringView dir = _path.GetDirectory();
    if (!dir.IsEmpty())
    {
        if (const Error err = FileSystem::MakeDirectory(dir))
        {
            return err;
        }
    }

    const char* mode  = (_policy == eFileWriterCollisionPolicy::Append) ? "ab" : "wb";
    FILE*       pFile = nullptr;
    if (::fopen_s(&pFile, _path.GetPtr(), mode) != 0)
    {
        return Error::OpenFailed;
    }

    return FileWriter { pFile };
}

Error FileWriter::WriteFileAsBinary(
    const CMemoryBlock               _data,
    const StringView                 _path,
    const eFileWriterCollisionPolicy _policy)
{
    Expect<FileWriter> writer = Open(_path, _policy);
    if (!writer)
    {
        return writer.GetError();
    }

    const Expect<SizeT> ret = writer->WriteBinary(_data);
    if (!ret)
    {
        return ret.GetError();
    }

    return Error::None;
}

Error FileWriter::WriteFileAsString(
    const StringView                 _str,
    const StringView                 _path,
    const eFileWriterCollisionPolicy _policy)
{
    return WriteFileAsBinary(CMemoryBlock { _str.GetPtr(), _str.GetLength() }, _path, _policy);
}

Expect<SizeT> FileWriter::WriteBinary(
    const CMemoryBlock _data) const
{
    JAM_ASSERT(m_pFile, "File handle is null.");
    const size_t written = ::fwrite(_data.GetPtr(), 1, _data.GetByteWidth(), m_pFile);
    if (CmpLess(written, _data.GetByteWidth()))
    {
        JAM_LOG_ERROR("Failed to write {} bytes to file, only {} bytes were written.", _data.GetByteWidth(), written);
        return Error::WriteFailed;
    }
    return NumericCast<SizeT>(written);
}

Expect<SizeT> FileWriter::WriteString(
    const StringView _str) const
{
    return WriteBinary(CMemoryBlock { _str.GetPtr(), _str.GetLength() });   // exclude null terminator
}

void FileWriter::Flush() const
{
    JAM_ASSERT(m_pFile, "File handle is null.");
    (void)::fflush(m_pFile);
}

FileWriter::FileWriter(FILE* _pFile)
    : FileIO(_pFile)
{
}

Expect<FileReader> FileReader::Open(
    StringView _path)
{
    FILE* pFile = nullptr;
    if (::fopen_s(&pFile, _path.GetPtr(), "rb") != 0)
    {
        JAM_LOG_ERROR("Failed to open file '{}'.", _path);
        return Error::OpenFailed;
    }

    return FileReader { pFile };
}

Expect<Vector<Byte>> FileReader::ReadFileAsBinary(
    const StringView _path)
{
    Expect<FileReader> reader = Open(_path);
    if (!reader)
    {
        return reader.GetError();
    }

    const SizeT         totalSize = reader->CalcTotalSize();
    Vector<Byte>        data(totalSize);
    const Expect<SizeT> ret = reader->ReadBinary(data);
    if (!ret)
    {
        return ret.GetError();
    }

    return data;
}

Expect<String> FileReader::ReadFileAsString(
    const StringView _path)
{
    Expect<FileReader> reader = Open(_path);
    if (!reader)
    {
        return reader.GetError();
    }

    const SizeT         totalSize = reader->CalcTotalSize();
    String              str { totalSize, '\0' };
    const Expect<SizeT> ret = reader->ReadBinary({ str.GetPtr(), totalSize });
    if (!ret)
    {
        return ret.GetError();
    }

    return str;
}

Expect<SizeT> FileReader::ReadBinary(
    const MemoryBlock _dst,
    const SizeT       _count) const
{
    JAM_ASSERT(m_pFile, "File handle is null.");
    JAM_ASSERT(_dst.GetPtr(), "Destination buffer pointer is null.");
    JAM_ASSERT(_count <= _dst.GetByteWidth(), "Count exceeds destination buffer size.");
    const SizeT toRead = (_count == 0) ? _dst.GetByteWidth() : _count;
    const SizeT read   = NumericCast<SizeT>(::fread(_dst.GetPtr(), 1, toRead, m_pFile));
    if (CmpLess(read, toRead) && !IsEof())
    {
        JAM_LOG_ERROR("Failed to read {} bytes from file, only {} bytes were read. {}", toRead, read, CreateWindowsErrorMessage(::ferror(m_pFile), eWindowsErrorCodeType::Errno));
        return Error::ReadFailed;
    }
    return NumericCast<SizeT>(read);
}

Expect<String> FileReader::ReadString(
    const SizeT _count) const
{
    const SizeT         toRead = (_count == 0) ? Remaining() : _count;
    String              str { toRead, '\0' };
    const Expect<SizeT> ret = ReadBinary({ str.GetPtr(), toRead });
    if (!ret)
    {
        return ret.GetError();
    }
    return str;
}

bool FileReader::IsEof() const
{
    return m_pFile ? (::feof(m_pFile) != 0) : true;
}

FileReader::FileReader(FILE* _pFile)
    : FileIO(_pFile)
{
}

}   // namespace Jam