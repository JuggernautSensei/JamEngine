#pragma once

namespace jam
{

// file explorer
bool OpenFileExplorer(const fs::path& _path);
bool OpenFileWithDefaultApplication(const fs::path& _path);

// file dialog
struct FileDialogFilter
{
    StringView desc;   // e.g. "Image Files"
    StringView ext;    // e.g. "*.png *.jpg *.bmp"
};

[[nodiscard]] Optional<fs::path> OpenFileDialog(
    StringView                  _title              = "open file",
    StringView                  _defaultPathOrEmpty = {},
    std::span<const FileDialogFilter> _filtersOrEmpty     = {});

[[nodiscard]] Optional<Vector<fs::path>> OpenMultiFileDialog(
    StringView                  _title              = "open files",
    StringView                  _defaultPathOrEmpty = {},
    std::span<const FileDialogFilter> _filtersOrEmpty     = {});

[[nodiscard]] Optional<fs::path> SaveFileDialog(
    StringView                  _title              = "save file",
    StringView                  _defaultPathOrEmpty = {},
    std::span<const FileDialogFilter> _filtersOrEmpty     = {});

[[nodiscard]] Optional<fs::path> OpenFolderDialog(
    StringView _title              = "open folder",
    StringView _defaultPathOrEmpty = {});

}   // namespace jam