#pragma once
#include <shobjidl_core.h>

namespace jam
{

// error message
[[nodiscard]] StringView GetWindowsLastErrorMessage();

// file explorer
bool OpenFileExplorer(const fs::path& _path);
bool OpenFileWithDefaultApplication(const fs::path& _path);

enum class eFileDialogMode
{
    OpenFile,
    OpenMultipleFiles,
    SelectFolder,
    SaveFile,
};

struct FileDialogFilter
{
    StringView desc;   // e.g. "Image Files"
    StringView ext;    // e.g. "*.png *.jpg *.bmp"
};

class FileDialog
{
public:
    explicit FileDialog(eFileDialogMode _mode = eFileDialogMode::OpenFile);

    bool Open();
    void SetTitle(StringView _title) const;
    void SetDefaultPath(const fs::path& _path) const;
    void SetFilters(std::span<const FileDialogFilter> _filters) const;

    [[nodiscard]] const Vector<fs::path>& GetSelectedPaths() const;
    [[nodiscard]] const fs::path&         GetFirstSelectedPath() const;

private:
    ComPtr<IFileDialog> m_pfd           = nullptr;
    eFileDialogMode     m_mode          = eFileDialogMode::OpenFile;
    Vector<fs::path>    m_selectedPaths = {};
};

}   // namespace jam