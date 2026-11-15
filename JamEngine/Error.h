#pragma once

namespace Jam
{

class Error
{
public:
    enum Type
    {
        None = 0,

        // general
        Unknown,
        InvalidArgument,
        OutOfRange,
        RuntimeError,
        InternalError,
        NotImplemented,
        OutOfMemory,
        NotSupported,

        // graphics
        ShaderCompileFailed,

        // file io
        NotFound,
        AccessDenied,
        AlreadyExists,
        OpenFailed,
        ReadFailed,
        WriteFailed,
        UnsupportedFormat,
        ImportFailed,
        ExportFailed,

        // serialization
        InvalidData,
        ParseError,
        MissingField,
        TypeMismatch,

        // runtime
        ReflectionFailed,
        InvalidState,

        // OS
        DirectoryNotEmpty,
        FileInUse,
        DiskFull,
        InvalidPath,
        TooManyOpenFiles,
        NotFoundHardwareAdapter,

        // asset
        NotIdealAssetPath,

        // network
        BadNetworkPath,
    };

private:
    struct ErrorInfo
    {
        Type        type  = {};
        StringView  name  = {};
        WStringView nameW = {};
        StringView  msg   = {};
        WStringView msgW  = {};
    };

#define ERROR_MSG(_type, _msg) \
    case _type:                \
        return { _type, #_type, L## #_type, _msg, L##_msg };

    [[nodiscard]] constexpr ErrorInfo GetErrorMessage_() const
    {
        switch (m_type)
        {
            ERROR_MSG(None, "No error occurred");
            ERROR_MSG(Unknown, "An unknown error occurred");
            ERROR_MSG(InvalidArgument, "One or more arguments are invalid");
            ERROR_MSG(OutOfRange, "Value is outside the valid range");
            ERROR_MSG(RuntimeError, "A runtime error occurred");
            ERROR_MSG(InternalError, "An internal logic error occurred; this is likely a bug");
            ERROR_MSG(NotImplemented, "This feature is not yet implemented");
            ERROR_MSG(ShaderCompileFailed, "Shader compilation failed");
            ERROR_MSG(NotFound, "The requested resource could not be found");
            ERROR_MSG(AccessDenied, "Access denied; insufficient permissions");
            ERROR_MSG(AlreadyExists, "The resource already exists");
            ERROR_MSG(OpenFailed, "Failed to open the file or handle");
            ERROR_MSG(ReadFailed, "Failed to read from the file or device");
            ERROR_MSG(WriteFailed, "Failed to write to the file or device");
            ERROR_MSG(UnsupportedFormat, "The format is not supported");
            ERROR_MSG(ImportFailed, "Failed to import the asset");
            ERROR_MSG(ExportFailed, "Failed to export the asset");
            ERROR_MSG(InvalidData, "The data is corrupted or malformed");
            ERROR_MSG(ParseError, "Failed to parse the data");
            ERROR_MSG(MissingField, "A required field is missing");
            ERROR_MSG(TypeMismatch, "Type mismatch; the actual type does not match the expected type");
            ERROR_MSG(ReflectionFailed, "Reflection failed on the specified type or object");
            ERROR_MSG(InvalidState, "The object is in an invalid state for this operation");
            ERROR_MSG(DirectoryNotEmpty, "The directory is not empty");
            ERROR_MSG(FileInUse, "The file is in use by another process");
            ERROR_MSG(DiskFull, "The disk is full");
            ERROR_MSG(NotIdealAssetPath, "The asset path is not ideal; check the file extension or directory");
            ERROR_MSG(OutOfMemory, "Out of memory");
            ERROR_MSG(NotSupported, "This operation is not supported on the current platform");
            ERROR_MSG(InvalidPath, "The path is invalid or contains illegal characters");
            ERROR_MSG(TooManyOpenFiles, "Too many open files");
            ERROR_MSG(NotFoundHardwareAdapter, "No compatible graphics adapter was found");
            ERROR_MSG(BadNetworkPath, "The network path is invalid or unreachable");
            default:
                JAM_UNREACHABLE;
        }
    }

#undef ERROR_MSG

public:
    constexpr Error()
        : m_type(None)
    {
    }

    /* implicit */ constexpr Error(
        const Type _type)
        : m_type(_type)
    {
    }

    constexpr bool operator==(const Error _other) const
    {
        return m_type == _other.m_type;
    }

    constexpr bool operator!=(const Error _other) const
    {
        return m_type != _other.m_type;
    }
    constexpr bool operator==(const Error::Type _type) const
    {
        return m_type == _type;
    }

    constexpr bool operator!=(const Error::Type _type) const
    {
        return m_type != _type;
    }

    [[nodiscard]] constexpr bool IsOk() const
    {
        return m_type == None;
    }

    [[nodiscard]] constexpr bool IsError() const
    {
        return m_type != None;
    }

    explicit constexpr operator bool() const
    {
        return IsError();
    }

    [[nodiscard]] constexpr Type GetType() const
    {
        return m_type;
    }

    [[nodiscard]] constexpr StringView ToErrorMessage() const
    {
        return GetErrorMessage_().msg;
    }

    [[nodiscard]] constexpr WStringView ToErrorMessageW() const
    {
        return GetErrorMessage_().msgW;
    }

    [[nodiscard]] constexpr StringView ToErrorName() const
    {
        return GetErrorMessage_().name;
    }

    [[nodiscard]] constexpr WStringView ToErrorNameW() const
    {
        return GetErrorMessage_().nameW;
    }

private:
    Type m_type = None;
};

template<typename T>
class Expect
{
public:
    Expect()
        : m_bHasValue(false)
    {
        std::construct_at(&m_error, Error::Unknown);
    }

    ~Expect()
    {
        Reset_();
    }

    /* implicit */ Expect(const T& _value)
        : m_bHasValue(true)
    {
        std::construct_at(&m_value, _value);
    }

    /* implicit */ Expect(T&& _value)
        : m_bHasValue(true)
    {
        std::construct_at(&m_value, std::move(_value));
    }

    /* implicit */ Expect(const Error _error)
        : m_bHasValue(false)
    {
        std::construct_at(&m_error, _error);
    }

    /* implicit */ Expect(const Error::Type _error)
        : m_bHasValue(false)
    {
        std::construct_at(&m_error, _error);
    }

    Expect(Expect&& _other) noexcept
        : m_bHasValue(_other.m_bHasValue)
    {
        if (m_bHasValue)
        {
            std::construct_at(&m_value, std::move(_other.m_value));
        }
        else
        {
            std::construct_at(&m_error, _other.m_error);
        }
        _other.Reset_();
    }

    Expect& operator=(Expect&& _other) noexcept
    {
        if (this != &_other)
        {
            Reset_();
            m_bHasValue = _other.m_bHasValue;
            if (m_bHasValue)
            {
                std::construct_at(&m_value, std::move(_other.m_value));
            }
            else
            {
                std::construct_at(&m_error, _other.m_error);
            }
            _other.Reset_();
        }
        return *this;
    }

    Expect(const Expect& _other)
        : m_bHasValue(_other.m_bHasValue)
    {
        if (m_bHasValue)
        {
            std::construct_at(&m_value, _other.m_value);
        }
        else
        {
            std::construct_at(&m_error, _other.m_error);
        }
    }

    Expect& operator=(const Expect& _other)
    {
        if (this != &_other)
        {
            Reset_();
            m_bHasValue = _other.m_bHasValue;
            if (m_bHasValue)
            {
                std::construct_at(&m_value, _other.m_value);
            }
            else
            {
                std::construct_at(&m_error, _other.m_error);
            }
        }
        return *this;
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    /* implicit */ Expect(Expect<U>&& _other) noexcept(std::is_nothrow_constructible_v<T, U&&>)
        : m_bHasValue(_other.m_bHasValue)
    {
        if (m_bHasValue)
        {
            std::construct_at(&m_value, std::move(_other.m_value));
        }
        else
        {
            std::construct_at(&m_error, _other.m_error);
        }

        _other.Reset_();
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    /* implicit */ Expect(const Expect<U>& _other)
        : m_bHasValue(_other.m_bHasValue)
    {
        if (m_bHasValue)
        {
            std::construct_at(&m_value, _other.m_value);
        }
        else
        {
            std::construct_at(&m_error, _other.m_error);
        }
    }

    [[nodiscard]] bool HasValue() const
    {
        return m_bHasValue;
    }

    explicit operator bool() const
    {
        return m_bHasValue;
    }

    [[nodiscard]] const T& GetValue() const
    {
        JAM_ASSERT(m_bHasValue, "Expect does not contain a m_value");
        return m_value;
    }

    [[nodiscard]] T& GetValue()
    {
        JAM_ASSERT(m_bHasValue, "Expect does not contain a m_value");
        return m_value;
    }

    [[nodiscard]] T Take()
    {
        JAM_ASSERT(m_bHasValue, "Expect does not contain a m_value");
        T value = std::move(m_value);
        Reset_();
        return value;
    }

    [[nodiscard]] Error GetError() const
    {
        return m_error;
    }

    const T& operator*() const
    {
        return GetValue();
    }

    T& operator*()
    {
        return GetValue();
    }

    const T* operator->() const
    {
        return &GetValue();
    }

    T* operator->()
    {
        return &GetValue();
    }

private:
    void Reset_()
    {
        if (m_bHasValue)
        {
            std::destroy_at(&m_value);
        }

        std::construct_at(&m_error, Error::Unknown);
        m_bHasValue = false;
    }

    template<typename U>
    friend class Expect;

    union
    {
        Error m_error = Error::None;
        T     m_value;
    };
    bool m_bHasValue = false;
};

}   // namespace Jam

template<>
struct fmt::formatter<Jam::Error>
{
    constexpr auto parse(const fmt::format_parse_context& _ctx) const
    {
        return _ctx.begin();
    }

    template<typename FormatContext>
    auto format(const Jam::Error& _error, FormatContext& _ctx) const
    {
        return fmt::format_to(_ctx.out(), "Error {}: {}", _error.ToErrorName(), _error.ToErrorMessage());
    }
};

template<>
struct fmt::formatter<Jam::Error, wchar_t>
{
    constexpr auto parse(const fmt::wformat_parse_context& _ctx) const
    {
        return _ctx.begin();
    }

    template<typename FormatContext>
    auto format(const Jam::Error& _error, FormatContext& _ctx) const
    {
        return fmt::format_to(_ctx.out(), L"Error {}: {}", _error.ToErrorNameW(), _error.ToErrorMessageW());
    }
};
