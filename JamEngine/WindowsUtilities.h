#pragma once

namespace jam
{

// error handling
NODISCARD std::string GetSystemErrorMessage(DWORD _errorCode);
NODISCARD std::string GetSystemLastErrorMessage();

// file explorer
void OpenInFileExplorer(const fs::path& _path);

// file dialog
struct FileDialogFilter
{
    std::string_view name;   // m_textFilter displayName
    std::string_view ext;    // file extension (.txt, .png, etc.)
};

NODISCARD Result<fs::path> OpenFileDialog(std::span<const FileDialogFilter> _filtersOrEmpty = {}, std::string_view _defaltPathOrEmpty = "");
NODISCARD Result<std::vector<fs::path>> OpenFileDialogMulti(std::span<const FileDialogFilter> _filtersOrEmpty = {}, std::string_view _defaltPathOrEmpty = "");
NODISCARD Result<fs::path> SaveFileDialog(std::span<const FileDialogFilter> _filtersOrEmpty = {}, std::string_view _defaltPathOrEmpty = "");

}   // namespace jam