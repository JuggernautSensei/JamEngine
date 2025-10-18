#pragma once

namespace jam
{

// com error message
JAM_NODISCARD std::string CreateComErrorMessage(HRESULT _hr);

// file explorer
eErrorCode OpenFileExplorer(const fs::path& _path);

// file dialog
struct FileDialogFilter
{
    std::string_view description;   // e.g. "Image Files"
    std::string_view extension;     // e.g. "*.png *.jpg *.bmp"
};

JAM_NODISCARD std::optional<fs::path> OpenFileDialog(
    std::string_view                  _title              = "Open File",
    std::string_view                  _defaultPathOrEmpty = {},
    std::span<const FileDialogFilter> _filtersOrEmpty     = {});

JAM_NODISCARD std::vector<fs::path> OpenMultiFileDialog(
    std::string_view                  _title              = "Open Files",
    std::string_view                  _defaultPathOrEmpty = {},
    std::span<const FileDialogFilter> _filtersOrEmpty     = {});

JAM_NODISCARD std::optional<fs::path> SaveFileDialog(
    std::string_view                  _title              = "Save File",
    std::string_view                  _defaultPathOrEmpty = {},
    std::span<const FileDialogFilter> _filtersOrEmpty     = {});

JAM_NODISCARD std::optional<fs::path> OpenFolderDialog(
    std::string_view _title              = "Open Folder",
    std::string_view _defaultPathOrEmpty = {});

}   // namespace jam