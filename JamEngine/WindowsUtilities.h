#pragma once

namespace jam
{

// error handling
NODISCARD std::string GetSystemErrorMessage(DWORD _errorCode);
NODISCARD std::string GetSystemLastErrorMessage();

// file explorer
void OpenInExplorer(fs::path _path);

// file dialog
struct FileDialogFilter
{
    std::string_view name;   // m_textFilter name
    std::string_view ext;    // file extension (.txt, .png, etc.)
};

NODISCARD std::optional<fs::path> OpenFileDialog(std::span<const FileDialogFilter> _filtersOrEmpty = {}, std::string_view _defaltPathOrEmpty = "");
NODISCARD std::optional<std::vector<fs::path>> OpenFileDialogMulti(std::span<const FileDialogFilter> _filtersOrEmpty = {}, std::string_view _defaltPathOrEmpty = "");
NODISCARD std::optional<fs::path> SaveFileDialog(std::span<const FileDialogFilter> _filtersOrEmpty = {}, std::string_view _defaltPathOrEmpty = "");

}   // namespace jam