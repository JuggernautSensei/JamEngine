#include "pch.h"

#include "WindowsUtilities.h"

#include <comdef.h>
#include <portable-file-dialogs.h>

namespace jam
{

std::string CreateComErrorMessage(const HRESULT _hr)
{
    _com_error err(_hr);
#ifdef JAM_TEXT_ENCODING_UNICODE
    std::wstring_view wmsg = err.ErrorMessage();
    return ToUTF8(wmsg);
#else
    return err.ErrorMessage();
#endif
}

eErrorCode OpenFileExplorer(const fs::path& _path)
{
    fs::path     absPath = fs::absolute(_path);
    std::wstring wPath   = absPath.wstring();

    HINSTANCE result;
    if (fs::is_directory(absPath))
    {
        result = ShellExecuteW(nullptr, L"open", L"explorer.exe", wPath.c_str(), nullptr, SW_SHOW);
    }
    else if (fs::exists(absPath))
    {
        std::wstring arg = std::format(L"/select,\"{}\"", wPath);
        result           = ShellExecuteW(nullptr, L"open", L"explorer.exe", arg.c_str(), nullptr, SW_SHOW);
    }
    else
    {
        SetErrorMessage("Invalid path (does not exist): {}", absPath.string());
        return eErrorCode::InvalidArgument;
    }

    const INT_PTR ret = reinterpret_cast<INT_PTR>(result);
    if (ret <= 32)
    {
        SetErrorMessage("Failed to open file explorer (code {}): {}", ret, absPath.string());
        return eErrorCode::RuntimeError;
    }

    return eErrorCode::Ok;
}

std::optional<fs::path> OpenFileDialog(
    const std::string_view                  _title,
    const std::string_view                  _defaultPathOrEmpty,
    const std::span<const FileDialogFilter> _filtersOrEmpty)
{
    std::vector<std::string> filters;
    filters.reserve(_filtersOrEmpty.size() * 2);
    for (const FileDialogFilter& filter: _filtersOrEmpty)
    {
        filters.emplace_back(filter.description);
        filters.emplace_back(filter.extension);
    }

    pfd::open_file of { std::string(_title), std::string(_defaultPathOrEmpty), filters };

    std::vector<std::string> result = of.result();
    if (result.empty())
    {
        return std::nullopt;
    }
    return fs::path(result[0]);
}

std::vector<fs::path> OpenMultiFileDialog(
    const std::string_view                  _title,
    const std::string_view                  _defaultPathOrEmpty,
    const std::span<const FileDialogFilter> _filtersOrEmpty)
{
    std::vector<std::string> filters;
    filters.reserve(_filtersOrEmpty.size() * 2);
    for (const FileDialogFilter& filter: _filtersOrEmpty)
    {
        filters.emplace_back(filter.description);
        filters.emplace_back(filter.extension);
    }

    pfd::open_file of { std::string(_title), std::string(_defaultPathOrEmpty), filters, pfd::opt::multiselect };

    std::vector<std::string> result = of.result();
    std::vector<fs::path>    paths;
    paths.reserve(result.size());
    for (const std::string& str: result)
    {
        paths.push_back(fs::path(str));
    }
    return paths;
}

std::optional<fs::path> SaveFileDialog(
    const std::string_view                  _title,
    const std::string_view                  _defaultPathOrEmpty,
    const std::span<const FileDialogFilter> _filtersOrEmpty)
{
    std::vector<std::string> filters;
    filters.reserve(_filtersOrEmpty.size() * 2);
    for (const FileDialogFilter& filter: _filtersOrEmpty)
    {
        filters.emplace_back(filter.description);
        filters.emplace_back(filter.extension);
    }

    pfd::save_file sf { std::string(_title), std::string(_defaultPathOrEmpty), filters };

    std::string result = sf.result();
    if (result.empty())
    {
        return std::nullopt;
    }
    return fs::path(result);
}

std::optional<fs::path> OpenFolderDialog(
    const std::string_view _title,
    const std::string_view _defaultPathOrEmpty)
{
    pfd::select_folder sf { std::string(_title), std::string(_defaultPathOrEmpty) };

    std::string result = sf.result();
    if (result.empty())
    {
        return std::nullopt;
    }
    return fs::path(result);
}

}   // namespace jam