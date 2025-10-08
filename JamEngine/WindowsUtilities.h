#pragma once

namespace jam
{

JAM_NODISCARD std::string CreateComErrorMessage(HRESULT _hr);

// file explorer
void OpenInFileExplorer(const fs::path& _path);

// file dialog
struct FileDialogFilter
{
    std::string_view name;   // m_textFilter displayName
    std::string_view ext;    // file extension (.txt, .png, etc.)
};

JAM_NODISCARD std::optional<fs::path> OpenFileDialog(
    std::span<const FileDialogFilter> _filtersOrEmpty = {},
    std::string_view _defaltPathOrEmpty = "");

JAM_NODISCARD std::optional<std::vector<fs::path>> OpenFileDialogMulti(
    std::span<const FileDialogFilter> _filtersOrEmpty = {},
    std::string_view _defaltPathOrEmpty = "");

JAM_NODISCARD std::optional<fs::path> SaveFileDialog(
    std::span<const FileDialogFilter> _filtersOrEmpty    = {},
    std::string_view                  _defaltPathOrEmpty = "");

}   // namespace jam