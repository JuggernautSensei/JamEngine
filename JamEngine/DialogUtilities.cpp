#include "pch.h"

#include "DialogUtilities.h"

#include <comdef.h>
#include <portable-file-dialogs.h>

namespace jam
{

bool OpenFileExplorer(const fs::path& _path)
{
    const fs::path     absPath = fs::absolute(_path);
    std::wstring wPath   = absPath.wstring();

    HINSTANCE result;
    if (fs::is_directory(absPath))
    {
        result = ShellExecute(nullptr, L"open", L"explorer.exe", wPath.c_str(), nullptr, SW_SHOW);
    }
    else if (fs::exists(absPath))
    {
        const std::wstring arg = std::format(L"/select,\"{}\"", wPath);
        result           = ShellExecute(nullptr, L"open", L"explorer.exe", arg.c_str(), nullptr, SW_SHOW);
    }
    else
    {
        GlobalBuffer::SetLastError("Invalid path (does not exist): {}", absPath.string());
        return false;
    }

    const INT_PTR ret = reinterpret_cast<INT_PTR>(result);
    if (ret <= 32)
    {
        GlobalBuffer::SetLastError("Failed to open file explorer (code {}): {}", ret, absPath.string());
        return false;
    }

    return true;
}

bool OpenFileWithDefaultApplication(const fs::path& _path)
{
    JAM_ASSERT(!fs::is_directory(_path), "Cannot open a directory with default application");

    const fs::path     absPath = fs::absolute(_path);
    const std::wstring wPath   = absPath.wstring();

    HINSTANCE result = ShellExecute(nullptr, L"open", wPath.c_str(), nullptr, nullptr, SW_SHOW);

    const INT_PTR ret = reinterpret_cast<INT_PTR>(result);
    if (ret <= 32)
    {
        GlobalBuffer::SetLastError("Failed to open file with default application (code {}): {}", ret, absPath.string());
        return false;
    }

    return true;
}

Optional<fs::path> OpenFileDialog(
    const StringView                  _title,
    const StringView                  _defaultPathOrEmpty,
    const std::span<const FileDialogFilter> _filtersOrEmpty)
{
    Vector<std::string> filters;
    filters.reserve(_filtersOrEmpty.size() * 2);
    for (const FileDialogFilter& filter: _filtersOrEmpty)
    {
        filters.emplace_back(filter.desc);
        filters.emplace_back(filter.ext);
    }

    pfd::open_file of { std::string(_title), std::string(_defaultPathOrEmpty), filters };

    Vector<std::string> result = of.result();
    if (result.empty())
    {
        return k_nullopt;
    }
    return result[0];
}

Optional<Vector<fs::path>> OpenMultiFileDialog(
    const StringView                  _title,
    const StringView                  _defaultPathOrEmpty,
    const std::span<const FileDialogFilter> _filtersOrEmpty)
{
    Vector<std::string> filters;
    filters.reserve(_filtersOrEmpty.size() * 2);
    for (const FileDialogFilter& filter: _filtersOrEmpty)
    {
        filters.emplace_back(filter.desc);
        filters.emplace_back(filter.ext);
    }

    pfd::open_file of { std::string(_title), std::string(_defaultPathOrEmpty), filters, pfd::opt::multiselect };

    Vector<std::string> result = of.result();
    Vector<fs::path>    paths;
    paths.reserve(result.size());
    for (const std::string& str: result)
    {
        paths.push_back(fs::path(str));
    }
    return paths;
}

Optional<fs::path> SaveFileDialog(
    const StringView                  _title,
    const StringView                  _defaultPathOrEmpty,
    const std::span<const FileDialogFilter> _filtersOrEmpty)
{
    Vector<std::string> filters;
    filters.reserve(_filtersOrEmpty.size() * 2);
    for (const FileDialogFilter& filter: _filtersOrEmpty)
    {
        filters.emplace_back(filter.desc);
        filters.emplace_back(filter.ext);
    }

    pfd::save_file sf { std::string(_title), std::string(_defaultPathOrEmpty), filters };

    const std::string result = sf.result();
    if (result.empty())
    {
        return k_nullopt;
    }
    else
    {
        return result;
    }
}

Optional<fs::path> OpenFolderDialog(
    const StringView _title,
    const StringView _defaultPathOrEmpty)
{
    pfd::select_folder sf { std::string(_title), std::string(_defaultPathOrEmpty) };

    const std::string result = sf.result();
    if (result.empty())
    {
        return k_nullopt;
    }
    else
    {
        return result;
    }
}

}   // namespace jam