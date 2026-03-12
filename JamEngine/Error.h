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
    struct ErrorMessage
    {
        Type        type = {};
        StringView  msg  = {};
        WStringView msgW = {};
    };

#define ERROR_MSG(_type, _msg) \
    case _type:                \
        return { _type, _msg, L##_msg }

    [[nodiscard]] constexpr ErrorMessage GetErrorMessage_() const
    {
        switch (m_type)
        {
            ERROR_MSG(None, "no error occurred");
            ERROR_MSG(Unknown, "an unspecified unknown error has occurred");
            ERROR_MSG(InvalidArgument, "one or more arguments are invalid");
            ERROR_MSG(OutOfRange, "index or m_value is out of the valid range");
            ERROR_MSG(RuntimeError, "a generic error occurred during execution");
            ERROR_MSG(InternalError, "an unexpected internal logic error was detected");
            ERROR_MSG(NotImplemented, "the requested feature is not yet implemented");
            ERROR_MSG(ShaderCompileFailed, "failed to compile the graphics shader");
            ERROR_MSG(NotFound, "the requested file or resource could not be found");
            ERROR_MSG(AccessDenied, "permission denied while accessing the resource");
            ERROR_MSG(AlreadyExists, "cannot create because the file or resource already exists");
            ERROR_MSG(OpenFailed, "failed to open the file stream or handle");
            ERROR_MSG(ReadFailed, "failed to read data from the storage device");
            ERROR_MSG(WriteFailed, "failed to write data to the storage device");
            ERROR_MSG(UnsupportedFormat, "the provided data format is not supported");
            ERROR_MSG(ImportFailed, "failed to import the external asset or data");
            ERROR_MSG(ExportFailed, "failed to export the asset to the specified format");
            ERROR_MSG(InvalidData, "the data content is corrupted or invalid");
            ERROR_MSG(ParseError, "failed to parse the structured data (json/xml/etc)");
            ERROR_MSG(MissingField, "a required data field is missing from the source");
            ERROR_MSG(TypeMismatch, "the data exportType does not match the expected exportType");
            ERROR_MSG(ReflectionFailed, "failed to perform reflection on the specified exportType or object");
            ERROR_MSG(InvalidState, "the operation cannot be performed in the current state");
            ERROR_MSG(DirectoryNotEmpty, "the directory is not empty");
            ERROR_MSG(FileInUse, "the file is currently in use by another process");
            ERROR_MSG(DiskFull, "the disk is full, not enough space to complete the operation");
            ERROR_MSG(NotIdealAssetPath, "the asset path is not ideal (e.g. missing expected file extension or located outside of the assets directory)");
            ERROR_MSG(OutOfMemory, "the system ran out of memory to complete the operation");
            ERROR_MSG(NotSupported, "the requested operation or feature is not supported on the current platform or configuration");
            ERROR_MSG(InvalidPath, "the specified file path is invalid or contains illegal characters");
            ERROR_MSG(TooManyOpenFiles, "the process has too many open files, cannot open more");
            ERROR_MSG(NotFoundHardwareAdapter, "no compatible hardware adapter was found for graphics operations");
            ERROR_MSG(BadNetworkPath, "the network path is invalid or unreachable");
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

    [[nodiscard]] constexpr StringView ToErrorMessage() const
    {
        return GetErrorMessage_().msg;
    }

    [[nodiscard]] constexpr WStringView ToErrorMessageW() const
    {
        return GetErrorMessage_().msgW;
    }

    /* implicit */ constexpr operator bool() const
    {
        return IsError();
    }

    [[nodiscard]] constexpr Type GetType() const
    {
        return m_type;
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

    /* implicit */ operator bool() const
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
        else
        {
            std::destroy_at(&m_error);
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
