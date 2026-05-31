#include "pch.h"
#include "FileLock.h"

#include "StaticString.h"
#include "WindowsUtilities.h"

namespace Jam
{

Expect<Scoped<FileLock>> FileLock::Create(
    const StringView    _path,
    const eFileLockMode _mode)
{
    Scoped<FileLock> pLock = new FileLock;
    if (Error err = pLock->Init_(_path, _mode))
    {
        return err;   // failed to acquire the file lock
    }
    return pLock;
}

FileLock::~FileLock()
{
    Close_();
}

FileLock::FileLock() = default;

void FileLock::Close_()
{
    if (m_fileHandle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_fileHandle);
        m_fileHandle = INVALID_HANDLE_VALUE;
    }
}

Error FileLock::Init_(
    const StringView    _path,
    const eFileLockMode _mode)
{
    const WString path = WString::Encoded(_path);

    DWORD accessMode         = 0;
    DWORD shareMode          = 0;
    DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

    switch (_mode)
    {
        case eFileLockMode::Read:
            accessMode = GENERIC_READ;
            shareMode  = FILE_SHARE_READ;
            break;

        case eFileLockMode::Write:
            accessMode = GENERIC_WRITE;
            shareMode  = 0;   // exclusive access
            break;

        case eFileLockMode::ReadWrite:
            accessMode = GENERIC_READ | GENERIC_WRITE;
            shareMode  = 0;   // exclusive access
            break;

        default:
            JAM_UNREACHABLE;
    }

    if (_path.IsDirectory())
    {
        flagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;   // required to open a handle to a directory
    }

    m_fileHandle = ::CreateFileW(
        path.GetPtr(),
        accessMode,
        shareMode,
        nullptr,
        OPEN_EXISTING,
        flagsAndAttributes,
        nullptr);

    if (m_fileHandle == INVALID_HANDLE_VALUE)
    {
        const Error err = GetLastError() == ERROR_SHARING_VIOLATION ? Error::FileInUse : Error::OpenFailed;
        JAM_LOG_ERROR("Failed to acquire file lock for '{}': {}", _path, CreateWindowsLastErrorMessage());
        return err;
    }

    return Error::None;
}

}   // namespace Jam