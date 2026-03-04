#include "pch.h"
#include "FileWatcher.h"
#include "StackString.h"

namespace Jam
{

Expect<Scoped<FileWatcher>> FileWatcher::Create(
    const FileWatcherInfo& _info)
{
    Scoped<FileWatcher> pWatcher = new FileWatcher;
    if (Error err = pWatcher->Init_(_info))
    {
        return err;
    }
    return pWatcher;
}

FileWatcher::~FileWatcher()
{
    Close_();
}

void FileWatcher::Close_()
{
    m_bRunning = false;

    if (m_dirHandle != INVALID_HANDLE_VALUE)
    {
        ::CancelIoEx(m_dirHandle, nullptr);
        ::CloseHandle(m_dirHandle);
        m_dirHandle = INVALID_HANDLE_VALUE;
    }

    if (m_watcherThread.joinable())
    {
        m_watcherThread.join();
    }
}

Error FileWatcher::Init_(
    const FileWatcherInfo& _info)
{
    JAM_ASSERT(_info.FileWatcherListener, "FileWatcherListener cannot be null.");

    WStackString<k_maxPathLength> path;
    path.Encode(_info.path);

    m_dirHandle = ::CreateFileW(
        path.GetPtr(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (m_dirHandle == INVALID_HANDLE_VALUE)
    {
        JAM_LOG_ERROR("Failed to create file watcher for '{}': {}", _info.path, CreateWindowsLastErrorMessage());
        return ErrorFromPlatformErrorCode(::GetLastError());
    }

    m_bRecursive          = _info.bRecursive;
    m_pollingDelayMs      = _info.pollingDelayMs;
    m_fileWatcherListener = _info.FileWatcherListener;
    m_bRunning            = true;
    m_watcherThread       = std::thread { &FileWatcher::Run_, this };
    return Error::None;
}

void FileWatcher::Run_()
{
    BYTE  buffer[4096];
    DWORD bytesReturned = 0;

    while (m_bRunning)
    {
        const BOOL bSuccess = ::ReadDirectoryChangesW(
            m_dirHandle,
            buffer,
            sizeof(buffer),
            m_bRecursive ? TRUE : FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
            &bytesReturned,
            nullptr,
            nullptr);

        if (bSuccess == TRUE && m_bRunning)
        {
            FILE_NOTIFY_INFORMATION* pInfo  = nullptr;
            DWORD                    offset = 0;

            StackString<k_maxPathLength> path;
            StackString<k_maxPathLength> oldPath;

            do
            {
                pInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer + offset);

                FileWatcherEvent event     = {};
                bool             bDispatch = true;

                switch (pInfo->Action)
                {
                    case FILE_ACTION_ADDED:
                    {
                        path.Encode(pInfo->FileName, NumericCast<SizeT>(pInfo->FileNameLength / sizeof(WCHAR)));
                        event.type    = eFileEventType::Added;
                        event.newPath = path;
                    }
                    break;

                    case FILE_ACTION_REMOVED:
                    {
                        path.Encode(pInfo->FileName, NumericCast<SizeT>(pInfo->FileNameLength / sizeof(WCHAR)));
                        event.type    = eFileEventType::Deleted;
                        event.oldPath = path;
                    }
                    break;

                    case FILE_ACTION_MODIFIED:
                    {
                        path.Encode(pInfo->FileName, NumericCast<SizeT>(pInfo->FileNameLength / sizeof(WCHAR)));
                        event.type    = eFileEventType::Modified;
                        event.newPath = path;
                    }
                    break;

                    case FILE_ACTION_RENAMED_OLD_NAME:
                    {
                        oldPath.Encode(pInfo->FileName, NumericCast<SizeT>(pInfo->FileNameLength / sizeof(WCHAR)));
                        bDispatch = false;   // dispatch in FILE_ACTION_RENAMED_NEW_NAME case
                    }
                    break;

                    case FILE_ACTION_RENAMED_NEW_NAME:
                    {
                        path.Encode(pInfo->FileName, NumericCast<SizeT>(pInfo->FileNameLength / sizeof(WCHAR)));
                        event.type    = eFileEventType::Renamed;
                        event.oldPath = oldPath;
                        event.newPath = path;
                    }
                    break;

                    default:
                        JAM_UNREACHABLE;
                }

                if (bDispatch)
                {
                    m_fileWatcherListener(event);
                }

                offset += pInfo->NextEntryOffset;
            } while (pInfo->NextEntryOffset != 0);
        }

        if (m_pollingDelayMs > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds { m_pollingDelayMs });
        }
    }
}

}   // namespace Jam