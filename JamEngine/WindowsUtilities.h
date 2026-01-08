#pragma once
#include <shobjidl_core.h>

namespace jam
{

// error message
[[nodiscard]] std::string CreateErrorMessageFromWin32LastError();
[[nodiscard]] std::string CreateErrorMessageFromHRESULT(HRESULT _hr);

// file explorer
bool OpenFileExplorer(const fs::path& _path);
bool OpenFileAsDefaultApplication(const fs::path& _path);

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
    void SetFilters(Span<const FileDialogFilter> _filters) const;

    // option in SaveFile mode
    void SetDefaultFileName(StringView _fileName) const;
    void SetDefaultExtension(StringView _ext) const;

    [[nodiscard]] const Vector<fs::path>& GetSelectedPaths() const;
    [[nodiscard]] const fs::path&         GetFirstSelectedPath() const;

private:
    ComPtr<IFileDialog> m_pfd           = nullptr;
    eFileDialogMode     m_mode          = eFileDialogMode::OpenFile;
    Vector<fs::path>    m_selectedPaths = {};
};

}   // namespace jam