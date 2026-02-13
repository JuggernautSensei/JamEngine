#include "pch.h"

#include "WindowsUtilities.h"

#include <comdef.h>

#include "WindowsPlatform.h"

namespace Jam
{

void FormatErrnoMessage(
    ScratchBuffer& _buf,
    const UInt32   _errnum)
{
    FormatHRESULTMessage(_buf, HRESULT_FROM_WIN32(_errnum));
}

void FormatHRESULTMessage(
    ScratchBuffer& _buf,
    const HRESULT  _hr)
{
    char*       msg = nullptr;
    const DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL,
                                     _hr,
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                     reinterpret_cast<LPSTR>(&msg),
                                     0,
                                     NULL);

    if (len > 0 && msg)
    {
        _buf.Set(msg);
        LocalFree(msg);
    }
    else
    {
        _buf.Format("Unknown error (HRESULT=0x{:08X})", NumericCast<UInt32>(_hr));
    }
}

FileDialog::FileDialog(
    const eFileDialogMode _mode)
    : m_mode(_mode)
{
    const CLSID clsid = (m_mode == eFileDialogMode::SaveFile) ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
    JAM_CHECK_HRESULT(CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_pfd.ReleaseAndGetAddressOf())));
}

bool FileDialog::Open()
{
    m_selectedPaths.clear();

    DWORD options = 0;
    JAM_CHECK_HRESULT(m_pfd->GetOptions(&options));

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

    JAM_CHECK_HRESULT(m_pfd->SetOptions(options));

    // 다이얼로그 표시
    const HWND    hWnd   = JAM_WIN.GetPlatformHandle();
    const HRESULT hrShow = m_pfd->Show(hWnd);
    if (hrShow == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        return false;   // 사용자가 취소
    }
    JAM_CHECK_HRESULT(hrShow);

    // 결과 수집
    if ((options & FOS_ALLOWMULTISELECT) != 0)
    {
        // 다중 선택 케이스
        ComPtr<IFileOpenDialog> pOpenDlg = nullptr;
        JAM_CHECK_HRESULT(m_pfd.As(&pOpenDlg));

        ComPtr<IShellItemArray> items = nullptr;
        JAM_CHECK_HRESULT(pOpenDlg->GetResults(&items));

        DWORD count = 0;
        JAM_CHECK_HRESULT(items->GetCount(&count));

        m_selectedPaths.reserve(count);

        for (DWORD i = 0; i < count; ++i)
        {
            ComPtr<IShellItem> item = nullptr;
            JAM_CHECK_HRESULT(items->GetItemAt(i, &item));

            PWSTR pszPath = nullptr;
            JAM_CHECK_HRESULT(item->GetDisplayName(SIGDN_FILESYSPATH, &pszPath));

            m_selectedPaths.emplace_back(pszPath);
            CoTaskMemFree(pszPath);
        }

        return !m_selectedPaths.empty();
    }
    else
    {
        // 단일 선택 케이스
        ComPtr<IShellItem> item = nullptr;
        JAM_CHECK_HRESULT(m_pfd->GetResult(&item));

        PWSTR pszPath = nullptr;
        JAM_CHECK_HRESULT(item->GetDisplayName(SIGDN_FILESYSPATH, &pszPath));

        m_selectedPaths.emplace_back(Path(pszPath));
        CoTaskMemFree(pszPath);

        return true;
    }
}

void FileDialog::SetTitle(
    const StringView _title) const
{
    ScratchWBuffer buf;
    buf.SetUtf8(_title);
    JAM_CHECK_HRESULT(m_pfd->SetTitle(buf.GetPtr()));
}

void FileDialog::SetDefaultPath(
    const Path& _path) const
{
    const StringView dir = ParentPathViewOf(_path);

    ScratchWBuffer wStr;
    wStr.SetUtf8(dir);

    ComPtr<IShellItem> folderItem = nullptr;
    JAM_CHECK_HRESULT(SHCreateItemFromParsingName(wStr.GetPtr(), nullptr, IID_PPV_ARGS(folderItem.ReleaseAndGetAddressOf())));
    JAM_CHECK_HRESULT(m_pfd->SetDefaultFolder(folderItem.Get()));
}

void FileDialog::SetFilters(
    const Span<const FileDialogFilter> _filters) const
{
    JAM_ASSERT(_filters.size() < 64, "Too many filters specified");

    BasicScratchWBuffer<8192>    wbuf;
    Array<COMDLG_FILTERSPEC, 64> filterSpecs;

    for (SizeT i = 0; i < _filters.size(); ++i)
    {
        JAM_ASSERT(wbuf.GetLength() < wbuf.GetCapacity() - 256, "Filter description and extension strings are too long");

        const FileDialogFilter& filter = _filters[i];

        const wchar_t* pDesc = wbuf.GetPtr() + wbuf.GetLength();
        wbuf.AppendUtf8(filter.desc);
        wbuf.Append(L"\0");

        const wchar_t* pExt = wbuf.GetPtr() + wbuf.GetLength();
        wbuf.AppendUtf8(filter.ext);
        wbuf.Append(L"\0");

        filterSpecs[i] = { pDesc, pExt };
    }

    const UInt32 numFilters = NumericCast<UInt32>(_filters.size());
    JAM_CHECK_HRESULT(m_pfd->SetFileTypes(numFilters, filterSpecs.data()));
}

void FileDialog::SetDefaultFileName(
    const StringView _fileName) const
{
    JAM_ASSERT(m_mode == eFileDialogMode::SaveFile, "Default file name can only be set in SaveFileAsByte mode");

    ScratchWBuffer buf;
    buf.SetUtf8(_fileName);
    JAM_CHECK_HRESULT(m_pfd->SetFileName(buf.GetPtr()));
}

void FileDialog::SetDefaultExtension(
    const StringView _ext) const
{
    JAM_ASSERT(m_mode == eFileDialogMode::SaveFile, "Default extension can only be set in SaveFileAsByte mode");

    ScratchWBuffer buf;
    buf.SetUtf8(_ext);
    JAM_CHECK_HRESULT(m_pfd->SetDefaultExtension(buf.GetPtr()));
}

const Vector<Path>& FileDialog::GetSelectedPaths() const
{
    return m_selectedPaths;
}

const Path& FileDialog::GetFirstSelectedPath() const
{
    JAM_ASSERT(!m_selectedPaths.empty(), "no selected paths available");
    return m_selectedPaths[0];
}

}   // namespace Jam