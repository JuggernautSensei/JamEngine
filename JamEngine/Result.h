#pragma once

namespace jam
{

enum class eErrorCode : Int32
{
    Ok = 0,

    // 이하 에러
    UnknownError,

    RuntimeError,      // 실행 중 오류
    InvalidArgument,   // 잘못된 인자
    DomainError,       // 수학적 정의역 오류
    OutOfRange,        // 범위 초과
    LengthMismatch,    // 길이 불일치
    Timeout,           // 시간 초과

    FileNotFound,        // 파일을 찾을 수 없음
    FileOpenFailed,      // 파일 열기 실패
    FileReadFailed,      // 파일 읽기 실패
    FileWriteFailed,     // 파일 쓰기 실패
    FileParseFailed,     // 파일 파싱 실패
    FileVertifyFailed,   // 파일 검증 실패

    UnsupportedFile,   // 지원하지 않는 파일 형식
    ShaderCompileFailed,
    VersionMismatch,
};

JAM_NODISCARD constexpr bool IsOk(const eErrorCode _result)
{
    return _result == eErrorCode::Ok;
}

JAM_NODISCARD constexpr bool IsError(const eErrorCode _result)
{
    return _result != eErrorCode::Ok;
}

/// @brief 함수 실행의 결과를 나타내는 템플릿 구조체
/// @tparam T 성공 시 반환되는 값의 타입
/// @note 단순하게 성공 여부와 값을 반환하는게 목적.
/// 그 외의 사용은 권장하지 않음.
template<typename T>
    requires(!std::is_reference_v<T>) && (!std::is_void_v<T>)
struct Result
{
    constexpr Result() noexcept
    {
    }

    /* implicit */ constexpr Result(const eErrorCode _error) noexcept
        : errorCode(_error)
        , value()
    {
        JAM_ASSERT(IsError(_error), "Result must be constructed with an error code when no value is provided.");
    }

    /* implicit */ constexpr Result(const T& v) noexcept
        : errorCode(eErrorCode::Ok)
        , value(v)
    {
    }

    /* implicit */ constexpr Result(T&& v) noexcept
        : errorCode(eErrorCode::Ok)
        , value(std::move(v))
    {
    }

    template<typename U>
        requires std::is_constructible_v<T, U> && !std::is_same_v<std::decay_t<U>, Result>
                 /* implicit */ constexpr Result(U && v) noexcept(std::is_nothrow_constructible_v<T, U>)
        : errorCode(eErrorCode::Ok)
    , value(std::forward<U>(v))
    {
    }

    JAM_NODISCARD constexpr bool IsOk() const noexcept
    {
        return jam::IsOk(errorCode);
    }

    JAM_NODISCARD constexpr bool IsError() const noexcept
    {
        return jam::IsError(errorCode);
    }

    JAM_NODISCARD explicit operator bool() const noexcept
    {
        return IsOk();
    }

    eErrorCode errorCode = eErrorCode::UnknownError;
    T          value     = {};
};

}   // namespace jam