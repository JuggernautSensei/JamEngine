#include "pch.h"
#include "File.h"

#include "FileSystem.h"
#include "WindowsUtilities.h"

namespace Jam
{

FileBase::~FileBase()
{
    Close();
}

FileBase::FileBase(
    FileBase&& _other) noexcept
    : m_pFile(_other.m_pFile)
{
    _other.m_pFile = nullptr;
}

FileBase& FileBase::operator=(
    FileBase&& _other) noexcept
{
    if (this != &_other)
    {
        Close();
        m_pFile        = _other.m_pFile;
        _other.m_pFile = nullptr;
    }
    return *this;
}

SizeT FileBase::Seek(
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

    if (::_fseeki64(m_pFile, _offset, static_cast<int>(origin)) != 0)
    {
        JAM_ASSERT(0, "Failed to seek file position.");
    }

    return Tell();
}

SizeT FileBase::Skip(
    const SizeT _numBytes) const
{
    return Seek(_numBytes, eSeekOp::Current);
}

SizeT FileBase::Tell() const
{
    const SizeT pos = ::_ftelli64(m_pFile);
    JAM_ASSERT(pos >= 0, "Failed to tell file position.");
    return pos;
}

SizeT FileBase::Remaining() const
{
    const SizeT cur = Tell();
    Seek(0, eSeekOp::End);
    const SizeT end = Tell();
    Seek(cur, eSeekOp::Set);
    return end - cur;
}

void FileBase::Rewind() const
{
    Seek(0, eSeekOp::Set);
}

SizeT FileBase::CalcTotalSize() const
{
    const SizeT cur = Tell();
    Seek(0, eSeekOp::End);
    const SizeT end = Tell();
    Seek(cur, eSeekOp::Set);
    return end;
}

void FileBase::Close()
{
    if (m_pFile)
    {
        (void)::fclose(m_pFile);
        m_pFile = nullptr;
    }
}

bool FileBase::IsClosed() const
{
    return m_pFile == nullptr;
}

bool FileBase::IsError() const
{
    return m_pFile ? (::ferror(m_pFile) != 0) : true;
}

FILE* FileBase::GetFilePtr() const
{
    return m_pFile;
}

FileBase::FileBase(FILE* _pFile)
    : m_pFile(_pFile)
{
}

Expect<FileWriter> FileWriter::Open(
    const StringView _path,
    const eFileWrite _mode)
{
    // Handle file name collision based on the specified policy.
    if (_mode == eFileWrite::Rename && _mode == eFileWrite::FailIfExists && FS::Exists(_path))
    {
        switch (_mode)
        {
            case eFileWrite::Rename:
                return Open(FS::MakeUniquePath(_path), eFileWrite::Truncate);

            case eFileWrite::FailIfExists:
                return Error::AlreadyExists;

            default:
                JAM_UNREACHABLE;
        }
    }

    // create directories if not exist
    const StringView dir = _path.GetDirectory();
    if (!dir.IsEmpty())
    {
        if (const Error err = FS::MakeDirectory(dir))
        {
            return err;
        }
    }

    const char* mode  = (_mode == eFileWrite::Append) ? "ab" : "wb";
    FILE*       pFile = nullptr;
    if (::fopen_s(&pFile, _path.GetPtr(), mode) != 0)
    {
        return Error::OpenFailed;
    }

    return FileWriter { pFile };
}

Expect<SizeT> FileWriter::Write(
    const MemoryView _data) const
{
    JAM_ASSERT(m_pFile, "File handle is null.");
    const SizeT written = static_cast<SizeT>(::fwrite(_data.GetPtr(), 1, _data.GetByteWidth(), m_pFile));
    if (written < _data.GetByteWidth())
    {
        JAM_LOG_ERROR("Failed to write {} bytes to file, only {} bytes were written.", _data.GetByteWidth(), written);
        return Error::WriteFailed;
    }
    return static_cast<SizeT>(written);
}

void FileWriter::Flush() const
{
    JAM_ASSERT(m_pFile, "File handle is null.");
    (void)::fflush(m_pFile);
}

FileWriter::FileWriter(FILE* _pFile)
    : FileBase(_pFile)
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

Expect<SizeT> FileReader::Read(
    const MutableMemoryView _dst,
    const SizeT             _count) const
{
    JAM_ASSERT(m_pFile, "File handle is null.");
    JAM_ASSERT(_dst.GetPtr(), "Destination buffer pointer is null.");
    JAM_ASSERT(_count <= _dst.GetByteWidth(), "Count exceeds destination buffer size.");

    const SizeT toRead = (_count == 0) ? _dst.GetByteWidth() : _count;
    const SizeT read   = static_cast<SizeT>(::fread(_dst.GetPtr(), 1, toRead, m_pFile));
    if (read < toRead && !IsEof())
    {
        JAM_LOG_ERROR("Failed to read {} bytes from file, only {} bytes were read. {}", toRead, read, MakeWindowsErrorMessage(::ferror(m_pFile), eWindowsErrorCode::Errno));
        return Error::ReadFailed;
    }
    return static_cast<SizeT>(read);
}

bool FileReader::IsEof() const
{
    return m_pFile ? (::feof(m_pFile) != 0) : true;
}

FileReader::FileReader(FILE* _pFile)
    : FileBase(_pFile)
{
}

}   // namespace Jam