#pragma once
#include <shobjidl_core.h>

namespace Jam
{

struct FileDialogFilter
{
    StringView desc = {};   // e.g. "Image Files"
    StringView ext  = {};   // e.g. "*.png *.jpg *.bmp"
};

struct OpenFileDialogInfo
{
    StringView                   title              = "open file";
    StringView                   defaultPathOrEmpty = {};
    Span<const FileDialogFilter> filtersOrEmpty     = {};
    bool                         bAllowMultiSelect  = false;
};

struct OpenDirectoryDialogInfo
{
    StringView title              = "open directory";
    StringView defaultPathOrEmpty = {};
    bool       bAllowMultiSelect  = false;
};

struct SaveFileDialogInfo
{
    StringView                   title              = "save file";
    StringView                   defaultPathOrEmpty = {};
    Span<const FileDialogFilter> filtersOrEmpty     = {};
};

class FileDialog
{
public:
    explicit FileDialog(const OpenFileDialogInfo& _info);
    explicit FileDialog(const SaveFileDialogInfo& _info);
    explicit FileDialog(const OpenDirectoryDialogInfo& _info);

    bool Open();

    [[nodiscard]] const Vector<String>& GetSelectedPaths() const;
    [[nodiscard]] StringView            GetFirstSelectedPath() const;

private:
    FileDialog() = default;

    void SetTitle_(StringView _title) const;
    void SetDefaultPath_(StringView _dir) const;
    void SetFilters_(Span<const FileDialogFilter> _filters) const;

    ComPtr<IFileDialog> m_pfd            = nullptr;
    Vector<String>      m_selectedPaths  = {};
    bool                m_bIsMultiSelect = false;
};

}   // namespace Jam