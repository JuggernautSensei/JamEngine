#pragma once

namespace Jam
{

enum class eFileLockMode
{
    Read,       // exclusive lock for reading. other processes can read but cannot write to the file.
    Write,      // exclusive lock for writing. other processes cannot read or write to the file.
    ReadWrite   // exclusive lock for reading and writing. other processes cannot read or write to the file.
};

class FileLock
{
public:
    [[nodiscard]] static Expect<Scoped<FileLock>> Create(StringView _path, eFileLockMode _mode);

    ~FileLock();
    FileLock(const FileLock&)                = delete;
    FileLock& operator=(const FileLock&)     = delete;
    FileLock(FileLock&&) noexcept            = delete;   // impl if needed
    FileLock& operator=(FileLock&&) noexcept = delete;   // impl if needed

private:
    FileLock();
    void  Close_();
    Error Init_(StringView _path, eFileLockMode _mode);

    HANDLE m_fileHandle = INVALID_HANDLE_VALUE;
};

}   // namespace Jam