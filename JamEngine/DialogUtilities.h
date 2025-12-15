#pragma once

namespace jam
{

// file explorer
bool OpenFileExplorer(const fs::path& _path);
bool OpenFileWithDefaultApplication(const fs::path& _path);

// file dialog
struct FileDialogFilter
{
    std::string_view desc;   // e.g. "Image Files"
    std::string_view ext;    // e.g. "*.png *.jpg *.bmp"
};

[[nodiscard]] std::optional<fs::path> OpenFileDialog(
    std::string_view                  _title              = "open file",
    std::string_view                  _defaultPathOrEmpty = {},
    std::span<const FileDialogFilter> _filtersOrEmpty     = {});

[[nodiscard]] std::optional<Vector<fs::path>> OpenMultiFileDialog(
    std::string_view                  _title              = "open files",
    std::string_view                  _defaultPathOrEmpty = {},
    std::span<const FileDialogFilter> _filtersOrEmpty     = {});

[[nodiscard]] std::optional<fs::path> SaveFileDialog(
    std::string_view                  _title              = "save file",
    std::string_view                  _defaultPathOrEmpty = {},
    std::span<const FileDialogFilter> _filtersOrEmpty     = {});

[[nodiscard]] std::optional<fs::path> OpenFolderDialog(
    std::string_view _title              = "open folder",
    std::string_view _defaultPathOrEmpty = {});

}   // namespace jam