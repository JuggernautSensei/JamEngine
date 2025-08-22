#include "pch.h"

#include "WindowsUtilities.h"

#include <nfd.h>

namespace
{

NODISCARD std::vector<nfdu8filteritem_t> CreateFilters(const std::span<const jam::FileDialogFilter> _filtersOrEmpty)
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

std::string GetSystemErrorMessage(const DWORD _errorCode)
{
    constexpr DWORD k_dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

    LPVOID      lpMsgBuf = nullptr;
    const DWORD dwSize   = FormatMessageA(k_dwFlags, nullptr, _errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, nullptr);
    if (dwSize == 0)
    {
        return "Unknown error";
    }
    std::string message(static_cast<char*>(lpMsgBuf), dwSize);
    LocalFree(lpMsgBuf);
    return message;
}

std::string GetSystemLastErrorMessage()
{
    return GetSystemErrorMessage(GetLastError());
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
        std::string cmd = std::format("explorer.exe \"{}\"", path.string());
        std::system(cmd.c_str());   // 시스템 명령어로 실행
    }
    else
    {
        JAM_ERROR("OpenInFileExplorer() - Path does not exist: {}", path.string());
    }
}

Result<fs::path> OpenFileDialog(const std::span<const FileDialogFilter> _filtersOrEmpty, const std::string_view _defaltPathOrEmpty)
{
    std::vector<nfdu8filteritem_t> filters = CreateFilters(_filtersOrEmpty);   // 필터 생성
    nfdu8char_t*                   outPath = nullptr;

    nfdresult_t result = NFD_OpenDialog(&outPath, filters.data(), static_cast<nfdfiltersize_t>(filters.size()), _defaltPathOrEmpty.data());

    if (result == NFD_OKAY)   // 성공
    {
        fs::path path = outPath;   // 복사
        NFD_FreePathU8(outPath);   // 내부적으로 메모리는 해제
        return path;               // 성공적으로 경로를 반환
    }
    else
    {
        if (result == NFD_ERROR)
        {
            JAM_ERROR("Failed to open file dialog: {}", NFD_GetError());
        }

        return Fail;
    }
}

Result<std::vector<fs::path>> OpenFileDialogMulti(const std::span<const FileDialogFilter> _filtersOrEmpty, const std::string_view _defaltPathOrEmpty)
{
    // 필터 생성
    std::vector<nfdu8filteritem_t> filters = CreateFilters(_filtersOrEmpty);
    const nfdpathset_t*            pOutPaths;

    nfdresult_t result = NFD_OpenDialogMultiple(&pOutPaths, filters.data(), static_cast<nfdfiltersize_t>(filters.size()), _defaltPathOrEmpty.data());

    if (result == NFD_OKAY)
    {
        nfdpathsetsize_t count;
        NFD_PathSet_GetCount(pOutPaths, &count);

        std::vector<fs::path> paths;
        for (nfdpathsetsize_t i = 0; i < count; ++i)
        {
            nfdu8char_t* nfdPath;
            NFD_PathSet_GetPath(pOutPaths, i, &nfdPath);
            paths.emplace_back(nfdPath);
            NFD_PathSet_FreePathU8(nfdPath);
        }
        return paths;
    }
    else
    {
        if (result == NFD_ERROR)
        {
            JAM_ERROR("Failed to open multiple file dialog: {}", NFD_GetError());
        }
        return Fail;
    }
}

Result<fs::path> SaveFileDialog(const std::span<const FileDialogFilter> _filtersOrEmpty, const std::string_view _defaltPathOrEmpty)
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
            JAM_ERROR("Failed to save file dialog: {}", NFD_GetError());
        }
        return Fail;
    }
}

}   // namespace jam