#include "pch.h"

#include "WindowsUtilities.h"

#include <comdef.h>
#include <nfd.h>

namespace
{

JAM_NODISCARD std::vector<nfdu8filteritem_t> CreateFilters(const std::span<const jam::FileDialogFilter> _filtersOrEmpty)
{
    std::vector<nfdu8filteritem_t> filters;
    for (const jam::FileDialogFilter& filter: _filtersOrEmpty)
    {
        filters.emplace_back(filter.name.data(), filter.ext.data());
    }
    return filters;
}

}   // namespace

namespace jam
{

std::string CreateComErrorMessage(const HRESULT _hr)
{
    _com_error err(_hr);
#ifdef JAM_TEXT_ENCODING_UNICODE
    std::wstring_view wmsg = err.ErrorMessage();
    auto [ec, msg]         = ToUTF8(wmsg);
    if (IsError(ec))
    {
        return "Unknown COM error";
    }
    return msg;
#else
    return err.ErrorMessage();
#endif
}

void OpenInFileExplorer(const fs::path& _path)
{
    fs::path path = _path;
    if (!fs::is_directory(path))
    {
        path = path.parent_path();   // 디렉토리가 아니면 부모 디렉토리로 변경
    }

    if (fs::exists(path))
    {
        std::string cmd = std::format("explorer.exe \"{}\"", path.native());
        std::system(cmd.c_str());   // 시스템 명령어로 실행
    }
    else
    {
        JAM_ASSERT(0, "OpenInFileExplorer() - Path does not exist: {}", path.native());
    }
}

std::optional<fs::path> OpenFileDialog(const std::span<const FileDialogFilter> _filtersOrEmpty, const std::string_view _defaltPathOrEmpty)
{
    std::vector<nfdu8filteritem_t> filters = CreateFilters(_filtersOrEmpty);   // 필터 생성

    nfdu8char_t* pPath  = nullptr;
    nfdresult_t  result = NFD_OpenDialog(&pPath, filters.data(), static_cast<nfdfiltersize_t>(filters.size()), _defaltPathOrEmpty.data());

    if (result == NFD_OKAY)   // 성공
    {
        fs::path output = pPath;   // 복사
        NFD_FreePathU8(pPath);     // 내부적으로 메모리는 해제
        return output;             // 성공적으로 경로를 반환
    }
    else   // 취소
    {
        if (result == NFD_ERROR)
        {
            JAM_ASSERT(0, "Failed to open file dialog: {}", NFD_GetError());
        }

        return std::nullopt;   // 취소시 nullopt 반환
    }
}

std::optional<std::vector<fs::path>> OpenFileDialogMulti(const std::span<const FileDialogFilter> _filtersOrEmpty, const std::string_view _defaltPathOrEmpty)
{
    // 필터 생성
    std::vector<nfdu8filteritem_t> filters = CreateFilters(_filtersOrEmpty);

    const nfdpathset_t* pPath;
    nfdresult_t         result = NFD_OpenDialogMultiple(&pPath, filters.data(), static_cast<nfdfiltersize_t>(filters.size()), _defaltPathOrEmpty.data());

    if (result == NFD_OKAY)
    {
        nfdpathsetsize_t count;
        NFD_PathSet_GetCount(pPath, &count);

        std::vector<fs::path> paths;
        for (nfdpathsetsize_t i = 0; i < count; ++i)
        {
            nfdu8char_t* nfdPath;
            NFD_PathSet_GetPath(pPath, i, &nfdPath);
            paths.emplace_back(nfdPath);
            NFD_PathSet_FreePathU8(nfdPath);
        }
        return paths;
    }
    else
    {
        if (result == NFD_ERROR)
        {
            JAM_ASSERT(0, "Failed to open multiple file dialog: {}", NFD_GetError());
        }

        return std::nullopt;
    }
}

std::optional<fs::path> SaveFileDialog(const std::span<const FileDialogFilter> _filtersOrEmpty, const std::string_view _defaltPathOrEmpty)
{
    // 필터 생성
    std::vector<nfdu8filteritem_t> filters  = CreateFilters(_filtersOrEmpty);
    nfdu8char_t*                   pOutPath = nullptr;

    nfdresult_t result = NFD_SaveDialog(&pOutPath, filters.data(), static_cast<nfdfiltersize_t>(filters.size()), _defaltPathOrEmpty.data(), nullptr);

    if (result == NFD_OKAY)
    {
        fs::path path = pOutPath;
        NFD_FreePathU8(pOutPath);
        return path;
    }
    else
    {
        if (result == NFD_ERROR)
        {
            JAM_ASSERT(0, "Failed to save file dialog: {}", NFD_GetError());
        }

        return std::nullopt;
    }
}

}   // namespace jam