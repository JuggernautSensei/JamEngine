#pragma once
#include <shobjidl_core.h>

namespace Jam
{

void FormatErrnoMessage(ScratchBuffer& _buf, UInt32 _errnum = errno);
void FormatHRESULTMessage(ScratchBuffer& _buf, HRESULT _hr);

// file dialog
enum class eFileDialogMode
{
    OpenFile,
    OpenMultipleFiles,
    SelectFolder,
    SaveFile,
};

struct FileDialogFilter
{
    StringView desc = {};   // e.g. "Image Files"
    StringView ext  = {};   // e.g. "*.png *.jpg *.bmp"
};

class FileDialog
{
public:
    explicit FileDialog(eFileDialogMode _mode = eFileDialogMode::OpenFile);

    bool Open();
    void SetTitle(StringView _title) const;
    void SetDefaultPath(const Path& _path) const;
    void SetFilters(Span<const FileDialogFilter> _filters) const;

    // option in WriteFileAsBinary mode
    void SetDefaultFileName(StringView _fileName) const;
    void SetDefaultExtension(StringView _ext) const;

    [[nodiscard]] const Vector<Path>& GetSelectedPaths() const;
    [[nodiscard]] const Path&         GetFirstSelectedPath() const;

private:
    ComPtr<IFileDialog> m_pfd           = nullptr;
    eFileDialogMode     m_mode          = eFileDialogMode::OpenFile;
    Vector<Path>        m_selectedPaths = {};
};

}   // namespace Jam