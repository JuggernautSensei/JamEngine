#include "pch.h"

#include "WindowsUtilities.h"
#include "WindowsPlatform.h"
#include "StackString.h"

#include <comdef.h>

namespace Jam
{

String CreateWindowsErrorMessage(UInt32 _errorCode, const eWindowsErrorCodeType _errorType)
{
    LPCVOID pSource = NULL;
    switch (_errorType)
    {
        case eWindowsErrorCodeType::Errno:
            _errorCode = HRESULT_FROM_WIN32(_errorCode);
            pSource    = NULL;
            break;

        case eWindowsErrorCodeType::HRESULT:
            pSource = NULL;
            break;

        case eWindowsErrorCodeType::Shell:
            pSource = ::GetModuleHandleW(L"shell32.dll");
            break;

        default:
            JAM_UNREACHABLE;
    }

    WStackString<512> buf;
    const DWORD       len = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        pSource,
        _errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf.GetPtr(),
        NumericCast<DWORD>(buf.GetCapacity() + 1),
        NULL);

    if (len == 0)
    {
        return String::Formatted("Unknown error code: 0x{:08X}", _errorCode);
    }
    else
    {
        return String::Encoded(buf.ToStringView());
    }
}

String CreateWindowsLastErrorMessage()
{
    return CreateWindowsErrorMessage(GetLastError(), eWindowsErrorCodeType::Errno);
}

WString CreateWindowsErrorMessageW(
    UInt32                      _errorCode,
    const eWindowsErrorCodeType _errorType)
{
    LPCVOID pSource = NULL;
    switch (_errorType)
    {
        case eWindowsErrorCodeType::Errno:
            _errorCode = HRESULT_FROM_WIN32(_errorCode);
            pSource    = NULL;
            break;

        case eWindowsErrorCodeType::HRESULT:
            pSource = NULL;
            break;

        case eWindowsErrorCodeType::Shell:
            pSource = ::GetModuleHandleW(L"shell32.dll");
            break;

        default:
            JAM_UNREACHABLE;
    }

    WStackString<512> buf;
    const DWORD       len = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        pSource,
        _errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf.GetPtr(),
        NumericCast<DWORD>(buf.GetCapacity() + 1),
        NULL);

    if (len == 0)
    {
        return WString::Formatted(L"Unknown error code: 0x{:08X}", _errorCode);
    }
    else
    {
        return buf.ToStringView();
    }
}

WString CreateWindowsLastErrorMessageW()
{
    return CreateWindowsErrorMessageW(GetLastError(), eWindowsErrorCodeType::Errno);
}

Error ErrorFromPlatformErrorCode(
    const UInt32 _err)
{
    switch (_err)
    {
        case ERROR_SUCCESS:
            return Error::None;

        case ERROR_FILE_NOT_FOUND:   // 파일을 찾을 수 없음
        case ERROR_PATH_NOT_FOUND:   // 경로를 찾을 수 없음
            return Error::NotFound;

        case ERROR_BAD_NETPATH:   // 네트워크 경로를 찾을 수 없음
            return Error::BadNetworkPath;

        case ERROR_DEV_NOT_EXIST:   // 장치가 존재하지 않음
            return Error::NotFoundHardwareAdapter;

        case ERROR_ACCESS_DENIED:    // 액세스 거부
        case ERROR_INVALID_ACCESS:   // 권한이 없는 접근 시도
        case ERROR_WRITE_PROTECT:    // 디스크가 쓰기 금지 상태
            return Error::AccessDenied;

        case ERROR_ALREADY_EXISTS:   // 파일이 이미 존재함
        case ERROR_FILE_EXISTS:      // 파일이 이미 존재함
            return Error::AlreadyExists;

        case ERROR_SHARING_VIOLATION:   // 공유 위반
        case ERROR_LOCK_VIOLATION:      // 파일 락 위반
        case ERROR_BUSY:                // 리소스 바쁨
            return Error::FileInUse;

        case ERROR_DISK_FULL:          // 디스크가 가득 참
        case ERROR_HANDLE_DISK_FULL:   // 디스크가 가득 참
        case ERROR_NOT_ENOUGH_QUOTA:   // 디스크 할당량 부족
            return Error::DiskFull;

        case ERROR_DIR_NOT_EMPTY:   // 디렉토리가 비어 있지 않음
            return Error::DirectoryNotEmpty;

        case ERROR_DIRECTORY:              // 디렉토리 이름이 잘못되었거나 파일이 디렉토리가 아님
        case ERROR_FILENAME_EXCED_RANGE:   // 경로가 MAX_PATH를 초과함
            return Error::InvalidPath;

        case ERROR_INVALID_NAME:        // 파일 이름에 허용되지 않는 문자 포함
        case ERROR_BAD_ARGUMENTS:       // 잘못된 인수
        case ERROR_INVALID_PARAMETER:   // 잘못된 매개변수
            return Error::InvalidArgument;

        case ERROR_TOO_MANY_OPEN_FILES:   // 시스템에서 열린 파일 핸들의 최대 수를 초과하여 더 이상 열 수 없음
            return Error::TooManyOpenFiles;

        case ERROR_OUTOFMEMORY:         // 시스템에 메모리가 부족하여 작업을 완료할 수 없음
        case ERROR_NOT_ENOUGH_MEMORY:   // 메모리가 부족하여 작업을 완료할 수 없음
            return Error::OutOfMemory;

        case ERROR_NOT_SUPPORTED:   // 기능이 지원되지 않음
            return Error::NotSupported;

        default:
            return Error::Unknown;
    }
}

}   // namespace Jam