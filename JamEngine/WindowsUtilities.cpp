#include "pch.h"

#include "WindowsUtilities.h"

#include "Application.h"

namespace jam
{

std::string CreateErrorMessageFromWin32LastError()
{
    const DWORD   errCode = GetLastError();
    const HRESULT hr      = HRESULT_FROM_WIN32(errCode);
    return CreateErrorMessageFromHRESULT(hr);
}

std::string CreateErrorMessageFromHRESULT(
    const HRESULT _hr)
{
    if (SUCCEEDED(_hr))
    {
        return "succeeded.";
    }

    return std::system_category().message(_hr);
}

bool OpenFileExplorer(const fs::path& _path)
{
    const fs::path absPath = fs::absolute(_path);
    std::wstring   wPath   = absPath.wstring();

    HINSTANCE result;

    if (fs::is_directory(absPath))
    {
        result = ShellExecute(nullptr, L"open", L"explorer.exe", wPath.c_str(), nullptr, SW_SHOW);
    }
    else if (fs::exists(absPath))
    {
        const std::wstring arg = std::format(L"/select,\"{}\"", wPath);
        result                 = ShellExecute(nullptr, L"open", L"explorer.exe", arg.c_str(), nullptr, SW_SHOW);
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

bool OpenFileAsDefaultApplication(const fs::path& _path)
{
    JAM_ASSERT(!fs::is_directory(_path), "Cannot open a directory with default application");

    const fs::path     absPath = fs::absolute(_path);
    const std::wstring wPath   = absPath.wstring();

    const HINSTANCE result = ShellExecute(nullptr, L"open", wPath.c_str(), nullptr, nullptr, SW_SHOW);
    const INT_PTR   ret    = reinterpret_cast<INT_PTR>(result);
    if (ret <= 32)
    {
        GlobalBuffer::SetLastError("Failed to open file with default application (code {}): {}", ret, absPath.string());
        return false;
    }

    return true;
}

FileDialog::FileDialog(
    const eFileDialogMode _mode)
    : m_mode(_mode)
{
    const CLSID clsid = (m_mode == eFileDialogMode::SaveFile) ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;

    JAM_CHECK_COM(
        CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_pfd.ReleaseAndGetAddressOf())),
        "Failed to create file dialog instance");
}

bool FileDialog::Open()
{
    m_selectedPaths.clear();

    DWORD options = 0;
    JAM_CHECK_COM(m_pfd->GetOptions(&options), "Failed to get file dialog options");

    // 옵션설정
    options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;

    // 모드 추가 옵션
    switch (m_mode)
    {
        case eFileDialogMode::OpenFile:
            options |= FOS_FILEMUSTEXIST;
            options &= ~FOS_PICKFOLDERS;
            options &= ~FOS_OVERWRITEPROMPT;
            break;

        case eFileDialogMode::OpenMultipleFiles:
            options |= FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT;
            options &= ~FOS_PICKFOLDERS;
            options &= ~FOS_OVERWRITEPROMPT;
            break;

        case eFileDialogMode::SaveFile:
            options |= FOS_OVERWRITEPROMPT;
            options &= ~FOS_PICKFOLDERS;
            options &= ~FOS_FILEMUSTEXIST;
            break;

        case eFileDialogMode::SelectFolder:
            options |= FOS_PICKFOLDERS;
            options &= ~FOS_FILEMUSTEXIST;
            options &= ~FOS_OVERWRITEPROMPT;
            break;

        default:
            break;
    }

    JAM_CHECK_COM(m_pfd->SetOptions(options), "Failed to set file dialog options");

    // 다이얼로그 표시
    const WindowsPlatform& platform = Application::GetInstance().GetPlatform();
    const HWND             hWnd     = platform.GetPlatformHandle();
    const HRESULT          hrShow   = m_pfd->Show(hWnd);
    if (hrShow == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        return false;   // 사용자가 취소
    }
    JAM_CHECK_COM(hrShow, "Failed to show file dialog");

    // 결과 수집
    if ((options & FOS_ALLOWMULTISELECT) != 0)
    {
        // 다중 선택 케이스
        ComPtr<IFileOpenDialog> pOpenDlg = nullptr;
        JAM_CHECK_COM(m_pfd.As(&pOpenDlg), "Failed to query IFileOpenDialog from IFileDialog");

        ComPtr<IShellItemArray> items = nullptr;
        JAM_CHECK_COM(pOpenDlg->GetResults(&items), "Failed to get multi-selected results");

        DWORD count = 0;
        JAM_CHECK_COM(items->GetCount(&count), "Failed to get result count");

        m_selectedPaths.reserve(count);

        for (DWORD i = 0; i < count; ++i)
        {
            ComPtr<IShellItem> item = nullptr;
            JAM_CHECK_COM(items->GetItemAt(i, &item), "Failed to get result item");

            PWSTR pszPath = nullptr;
            JAM_CHECK_COM(item->GetDisplayName(SIGDN_FILESYSPATH, &pszPath), "Failed to get selected item path");

            m_selectedPaths.emplace_back(pszPath);
            CoTaskMemFree(pszPath);
        }

        return !m_selectedPaths.empty();
    }
    else
    {
        // 단일 선택 케이스
        ComPtr<IShellItem> item = nullptr;
        JAM_CHECK_COM(m_pfd->GetResult(&item), "Failed to get file dialog result");

        PWSTR pszPath = nullptr;
        JAM_CHECK_COM(item->GetDisplayName(SIGDN_FILESYSPATH, &pszPath), "Failed to get selected path");

        m_selectedPaths.emplace_back(fs::path(pszPath));
        CoTaskMemFree(pszPath);

        return true;
    }
}

void FileDialog::SetTitle(
    const StringView _title) const
{
    Optional<std::wstring> optWTitle = ToUtf16(_title);
    JAM_ASSERT(optWTitle, "Failed to convert title to UTF-16");
    const std::wstring& wTitle = optWTitle.value();
    JAM_CHECK_COM(m_pfd->SetTitle(wTitle.c_str()), "Failed to set file dialog title");
}

void FileDialog::SetDefaultPath(
    const fs::path& _path) const
{
    // 부모 폴더로 보정
    fs::path dir = _path.parent_path();

    // 폴더 ShellItem 생성
    std::wstring wStr = dir.wstring();

    ComPtr<IShellItem> folderItem = nullptr;
    JAM_CHECK_COM(SHCreateItemFromParsingName(wStr.c_str(), nullptr, IID_PPV_ARGS(folderItem.ReleaseAndGetAddressOf())), "Failed to create IShellItem from default path");
    JAM_CHECK_COM(m_pfd->SetDefaultFolder(folderItem.Get()), "Failed to set default folder");
}

void FileDialog::SetFilters(
    const Span<const FileDialogFilter> _filters) const
{
    struct Filter
    {
        std::wstring name;
        std::wstring spec;
    };

    Vector<Filter> filters;
    filters.reserve(_filters.size());

    Vector<COMDLG_FILTERSPEC> filterSpecs;
    filterSpecs.reserve(_filters.size());

    for (const FileDialogFilter& filter: _filters)
    {
        Optional<std::wstring> optWName = ToUtf16(filter.desc);
        JAM_ASSERT(optWName, "Failed to convert filter name to UTF-16");
        Optional<std::wstring> optWSpec = ToUtf16(filter.ext);
        JAM_ASSERT(optWSpec, "Failed to convert filter spec to UTF-16");

        const Filter& result = filters.emplace_back(optWName.value(), optWSpec.value());
        filterSpecs.emplace_back(result.name.c_str(), result.spec.c_str());
    }

    JAM_CHECK_COM(
        m_pfd->SetFileTypes(static_cast<UINT>(filterSpecs.size()), filterSpecs.data()),
        "Failed to set file dialog filters");
}

void FileDialog::SetDefaultFileName(
    const StringView _fileName) const
{
    JAM_ASSERT(m_mode == eFileDialogMode::SaveFile, "Default file name can only be set in SaveFile mode");
    Optional<std::wstring> optWFileName = ToUtf16(_fileName);
    JAM_ASSERT(optWFileName, "Failed to convert default file name to UTF-16");
    const std::wstring& wFileName = optWFileName.value();
    JAM_CHECK_COM(m_pfd->SetFileName(wFileName.c_str()), "Failed to set default file name");
}

void FileDialog::SetDefaultExtension(
    const StringView _ext) const
{
    JAM_ASSERT(m_mode == eFileDialogMode::SaveFile, "Default extension can only be set in SaveFile mode");
    Optional<std::wstring> optWExt = ToUtf16(_ext);
    JAM_ASSERT(optWExt, "Failed to convert default extension to UTF-16");
    const std::wstring& wExt = optWExt.value();
    JAM_CHECK_COM(m_pfd->SetDefaultExtension(wExt.c_str()), "Failed to set default extension");
}

const Vector<fs::path>& FileDialog::GetSelectedPaths() const
{
    return m_selectedPaths;
}

const fs::path& FileDialog::GetFirstSelectedPath() const
{
    JAM_ASSERT(!m_selectedPaths.empty(), "no selected paths available");
    return m_selectedPaths[0];
}

}   // namespace jam