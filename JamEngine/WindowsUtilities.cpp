#include "pch.h"

#include "WindowsUtilities.h"

#include "vendor/tinyfiledialogs/tinyfiledialogs.h"
#include <comdef.h>

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
        SetErrorString("Invalid path (does not exist): {}", absPath.string());
        return eErrorCode::InvalidArgument;
    }

    const INT_PTR ret = reinterpret_cast<INT_PTR>(result);
    if (ret <= 32)
    {
        SetErrorString("Failed to open file explorer (code {}): {}", ret, absPath.string());
        return eErrorCode::RuntimeError;
    }

    return eErrorCode::Ok;
}

std::optional<fs::path> OpenFileDialog(
    const char*            _titleOrNull,
    const char*            _defaultPathOrNull,
    std::span<const char*> _filtersOrEmpty,
    const char*            _filterDescriptionOrNull)
{
    Int32        filterCount = NarrowCast<Int32>(_filtersOrEmpty.size());
    const char** pFilters    = filterCount > 0 ? _filtersOrEmpty.data() : NULL;
    const char*  selectPath  = tinyfd_openFileDialog(_titleOrNull, _defaultPathOrNull, filterCount, pFilters, _filterDescriptionOrNull, 0);
    if (selectPath == NULL)
    {
        return std::nullopt;
    }
    return fs::path { selectPath };
}

std::optional<std::vector<fs::path>> OpenMultipleFileDialog(
    const char*            _titleOrNull,
    const char*            _defaultPathOrNull,
    std::span<const char*> _filtersOrEmpty,
    const char*            _filterDescriptionOrNull)
{
    Int32        filterCount  = NarrowCast<Int32>(_filtersOrEmpty.size());
    const char** pFilters     = filterCount > 0 ? _filtersOrEmpty.data() : NULL;
    const char*  selectPathes = tinyfd_openFileDialog(_titleOrNull, _defaultPathOrNull, filterCount, pFilters, _filterDescriptionOrNull, 1);
    if (selectPathes == NULL)
    {
        return std::nullopt;
    }

    std::vector<fs::path> paths;

    std::string_view selectPathesView { selectPathes };
    size_t           pos   = selectPathesView.find_first_of('|');
    size_t           start = 0;
    while (pos != std::string_view::npos)
    {
        paths.emplace_back(selectPathesView.substr(start, pos - start));
        start = pos + 1;
        pos   = selectPathesView.find_first_of('|', start);
    }
    paths.emplace_back(selectPathesView.substr(start));
    return paths;
}

std::optional<fs::path> SaveFileDialog(
    const char*            _titleOrNull,
    const char*            _defaultPathOrNull,
    std::span<const char*> _filtersOrEmpty,
    const char*            _filterDescriptionOrNull)
{
    Int32        filterCount = NarrowCast<Int32>(_filtersOrEmpty.size());
    const char** pFilters    = filterCount > 0 ? _filtersOrEmpty.data() : NULL;
    const char*  selectPath  = tinyfd_saveFileDialog(_titleOrNull, _defaultPathOrNull, filterCount, pFilters, _filterDescriptionOrNull);
    if (selectPath == NULL)
    {
        return std::nullopt;
    }
    return fs::path { selectPath };
}

}   // namespace jam