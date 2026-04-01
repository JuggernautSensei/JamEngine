#pragma once
#include "LRUCache.h"
#include "WindowsUtilities.h"

namespace Jam
{

enum class eFileEvent
{
    Added,
    Deleted,
    Renamed,
    Modified
};

struct FileWatcherEvent
{
    StringView     oldPath = {};
    StringView     path    = {};
    eFileEvent type    = {};
};

struct FileWatcherInfo
{
    StringView                              path                = {};
    bool                                    bRecursive          = false;
    Int64                                   pollingDelayMs      = 500;
    Delegate<void(const FileWatcherEvent&)> FileWatcherListener = nullptr;
};

class FileWatcher
{
public:
    [[nodiscard]] static Expect<Scoped<FileWatcher>> Create(const FileWatcherInfo& _info);

    ~FileWatcher();

    FileWatcher(const FileWatcher&)                = delete;
    FileWatcher& operator=(const FileWatcher&)     = delete;
    FileWatcher(FileWatcher&&) noexcept            = delete;   // impl if needed
    FileWatcher& operator=(FileWatcher&&) noexcept = delete;   // impl if needed

private:
    FileWatcher() = default;
    void  Close_();
    Error Init_(const FileWatcherInfo& _info);
    void  Run_();

    std::thread m_watcherThread = {};

    bool                                    m_bRecursive          = false;
    Int64                                   m_pollingDelayMs      = 100;
    Delegate<void(const FileWatcherEvent&)> m_fileWatcherListener = nullptr;
    bool                                    m_bRunning            = false;
    HANDLE                                  m_dirHandle           = INVALID_HANDLE_VALUE;
    String                                  m_watchPath           = {};

    static constexpr UInt64 m_fileModifiedEventDebounceMs = 50;   // for debouncing file modified events
};

}   // namespace Jam