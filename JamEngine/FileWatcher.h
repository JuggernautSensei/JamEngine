// ReSharper disable CppClangTidyClangDiagnosticUnusedPrivateField
#pragma once
#include "Error.h"
#include "String.h"
#include "Delegate.h"

namespace Jam
{

enum class eFileEvent
{
    Added,
    Deleted,
    Renamed,
    Modified
};

// full path is dir / path. dir / oldPath
struct FileWatcherEvent
{
    eFileEvent type    = {};
    String     path    = {};
    String     oldPath = {};
};

struct FileWatcherInfo
{
    StringView                              path                = {};
    bool                                    bRecursive          = false;
    std::chrono::milliseconds               pollingDelay        = 500ms;
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
    std::chrono::milliseconds               m_pollingDelay        = 100ms;
    Delegate<void(const FileWatcherEvent&)> m_fileWatcherListener = nullptr;
    bool                                    m_bRunning            = false;
    HANDLE                                  m_dirHandle           = INVALID_HANDLE_VALUE;
    String                                  m_watchPath           = {};

    static constexpr std::chrono::milliseconds m_fileModifiedEventDebounce = 50ms;   // for debouncing file modified events
};

}   // namespace Jam