#include "pch.h"
#include "FileDialog.h"

#include "WindowsPlatform.h"

namespace Jam
{

bool FileDialog::Open()
{
    m_selectedPaths.clear();

    // 다이얼로그 표시
    const HWND    hWnd = GetWindowsPlatform().GetPlatformHandle();
    const HRESULT hr   = m_pfd->Show(hWnd);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        return false;   // 사용자가 취소
    }

    // 결과 수집
    if (m_bIsMultiSelect)
    {
        // 다중 선택 케이스
        Microsoft::WRL::ComPtr<IFileOpenDialog> pOpenDlg = nullptr;
        JAM_CHECK_HRESULT(m_pfd.As(&pOpenDlg));

        Microsoft::WRL::ComPtr<IShellItemArray> items = nullptr;
        JAM_CHECK_HRESULT(pOpenDlg->GetResults(&items));

        DWORD count = 0;
        JAM_CHECK_HRESULT(items->GetCount(&count));

        m_selectedPaths.reserve(count);

        for (DWORD i = 0; i < count; ++i)
        {
            Microsoft::WRL::ComPtr<IShellItem> item = nullptr;
            JAM_CHECK_HRESULT(items->GetItemAt(i, &item));

            PWSTR pszPath = nullptr;
            JAM_CHECK_HRESULT(item->GetDisplayName(SIGDN_FILESYSPATH, &pszPath));

            String path = {};
            path.Encode(pszPath);
            path.Simplify();
            m_selectedPaths.emplace_back(std::move(path));
            CoTaskMemFree(pszPath);
        }

        return !m_selectedPaths.empty();
    }
    else
    {
        // 단일 선택 케이스
        Microsoft::WRL::ComPtr<IShellItem> item = nullptr;
        JAM_CHECK_HRESULT(m_pfd->GetResult(&item));

        PWSTR pszPath = nullptr;
        JAM_CHECK_HRESULT(item->GetDisplayName(SIGDN_FILESYSPATH, &pszPath));

        String path = {};
        path.Encode(pszPath);
        path.Simplify();
        m_selectedPaths.emplace_back(std::move(path));

        CoTaskMemFree(pszPath);
        return true;
    }
}

FileDialog::FileDialog(
    const OpenFileDialogInfo& _info)
    : m_bIsMultiSelect(_info.bAllowMultiSelect)
{
    JAM_CHECK_HRESULT(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_pfd.ReleaseAndGetAddressOf())));

    DWORD options = 0;
    JAM_CHECK_HRESULT(m_pfd->GetOptions(&options));

    options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
    options |= FOS_FILEMUSTEXIST;
    options &= ~FOS_PICKFOLDERS;
    options &= ~FOS_OVERWRITEPROMPT;

    if (_info.bAllowMultiSelect)
    {
        options |= FOS_ALLOWMULTISELECT;
    }

    JAM_CHECK_HRESULT(m_pfd->SetOptions(options));

    SetTitle_(_info.title);

    if (!_info.defaultPathOrEmpty.IsEmpty())
    {
        SetDefaultPath_(_info.defaultPathOrEmpty);
    }

    if (!_info.filtersOrEmpty.empty())
    {
        SetFilters_(_info.filtersOrEmpty);
    }
}

FileDialog::FileDialog(
    const SaveFileDialogInfo& _info)
    : m_bIsMultiSelect(false)
{
    JAM_CHECK_HRESULT(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_pfd.ReleaseAndGetAddressOf())));

    DWORD options = 0;
    JAM_CHECK_HRESULT(m_pfd->GetOptions(&options));

    options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
    options |= FOS_FILEMUSTEXIST;
    options &= ~FOS_PICKFOLDERS;
    options &= ~FOS_OVERWRITEPROMPT;
    JAM_CHECK_HRESULT(m_pfd->SetOptions(options));

    SetTitle_(_info.title);

    if (!_info.defaultPathOrEmpty.IsEmpty())
    {
        SetDefaultPath_(_info.defaultPathOrEmpty);
    }

    if (!_info.filtersOrEmpty.empty())
    {
        SetFilters_(_info.filtersOrEmpty);
    }
}

FileDialog::FileDialog(
    const OpenDirectoryDialogInfo& _info)
    : m_bIsMultiSelect(_info.bAllowMultiSelect)
{
    JAM_CHECK_HRESULT(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_pfd.ReleaseAndGetAddressOf())));

    DWORD options = 0;
    JAM_CHECK_HRESULT(m_pfd->GetOptions(&options));

    options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
    options |= FOS_PICKFOLDERS;
    options &= ~FOS_FILEMUSTEXIST;

    if (_info.bAllowMultiSelect)
    {
        options |= FOS_ALLOWMULTISELECT;
    }

    JAM_CHECK_HRESULT(m_pfd->SetOptions(options));

    SetTitle_(_info.title);

    if (!_info.defaultPathOrEmpty.IsEmpty())
    {
        SetDefaultPath_(_info.defaultPathOrEmpty);
    }
}

const Vector<String>& FileDialog::GetSelectedPaths() const
{
    return m_selectedPaths;
}

StringView FileDialog::GetFirstSelectedPath() const
{
    JAM_ASSERT(!m_selectedPaths.empty(), "no selected paths available");
    return m_selectedPaths[0];
}

void FileDialog::SetTitle_(
    const StringView _title) const
{
    JAM_ASSERT(!_title.IsEmpty(), "Title cannot be empty.");

    const WString title = WString::Encoded(_title);
    JAM_CHECK_HRESULT(m_pfd->SetTitle(title.GetPtr()));
}

void FileDialog::SetDefaultPath_(
    const StringView _dir) const
{
    JAM_ASSERT(!_dir.IsEmpty(), "Default path cannot be empty.");
    JAM_ASSERT(_dir.IsDirectory(), "Default path must be a directory.");

    const WString                      dir        = WString::Encoded(_dir);
    Microsoft::WRL::ComPtr<IShellItem> folderItem = nullptr;
    JAM_CHECK_HRESULT(SHCreateItemFromParsingName(dir.GetPtr(), nullptr, IID_PPV_ARGS(folderItem.ReleaseAndGetAddressOf())));
    JAM_CHECK_HRESULT(m_pfd->SetDefaultFolder(folderItem.Get()));
}

void FileDialog::SetFilters_(
    const Span<const FileDialogFilter> _filters) const
{
    JAM_ASSERT(!_filters.empty(), "Filters cannot be empty.");

    struct Filter
    {
        WString desc = {};
        WString ext  = {};
    };

    Vector<Filter> filters = {};
    filters.reserve(_filters.size());

    Vector<COMDLG_FILTERSPEC> comdlgFilters = {};
    comdlgFilters.reserve(_filters.size());

    // must loop twice because avoid realloc of comdlgFilters which contains pointer to WString's internal buffer
    for (const FileDialogFilter& filter: _filters)
    {
        filters.emplace_back(WString::Encoded(filter.desc), WString::Encoded(filter.ext));
    }

    for (const Filter& filter: filters)
    {
        comdlgFilters.emplace_back(filter.desc.GetPtr(), filter.ext.GetPtr());
    }

    JAM_CHECK_HRESULT(m_pfd->SetFileTypes(static_cast<UINT>(_filters.size()), comdlgFilters.data()));
}

}   // namespace Jam