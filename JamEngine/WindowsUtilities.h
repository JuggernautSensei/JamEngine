#pragma once

namespace jam
{

// com error message
JAM_NODISCARD std::string CreateComErrorMessage(HRESULT _hr);

// file explorer
eErrorCode OpenFileExplorer(const fs::path& _path);

JAM_NODISCARD std::optional<fs::path> OpenFileDialog(
    const char*            _titleOrNull             = NULL,
    const char*            _defaultPathOrNull       = NULL,
    std::span<const char* const> _filtersOrEmpty = {},
    const char*            _filterDescriptionOrNull = NULL);

JAM_NODISCARD std::optional<std::vector<fs::path>> OpenMultipleFileDialog(
    const char*            _titleOrNull             = NULL,
    const char*            _defaultPathOrNull       = NULL,
    std::span<const char* const> _filtersOrEmpty = {},
    const char*            _filterDescriptionOrNull = NULL);

JAM_NODISCARD std::optional<fs::path> SaveFileDialog(
    const char*            _titleOrNull             = NULL,
    const char*            _defaultPathOrNull       = NULL,
    std::span<const char* const> _filtersOrEmpty = {},
    const char*            _filterDescriptionOrNull = NULL);


}   // namespace jam