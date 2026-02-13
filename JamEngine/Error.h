#pragma once

namespace Jam
{

class Error
{
public:
    enum Type
    {
        None,

        // general
        Unknown,
        InvalidArgument,
        OutOfRange,
        RuntimeError,
        InternalError,
        NotImplemented,

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

        // file system
        DirectoryCreationFailed,
        FileRemovalFailed,
    };

    constexpr Error()
        : m_type(Unknown)
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
        switch (m_type)
        {
            case Error::None:
                return "no error occurred";
            case Error::Unknown:
                return "an unspecified unknown error has occurred";
            case Error::InvalidArgument:
                return "one or more arguments are invalid";
            case Error::OutOfRange:
                return "index or value is out of the valid range";
            case Error::RuntimeError:
                return "a generic error occurred during execution";
            case Error::InternalError:
                return "an unexpected internal logic error was detected";
            case Error::NotImplemented:
                return "the requested feature is not yet implemented";
            case Error::ShaderCompileFailed:
                return "failed to compile the graphics shader";
            case Error::NotFound:
                return "the requested file or resource could not be found";
            case Error::AccessDenied:
                return "permission denied while accessing the resource";
            case Error::AlreadyExists:
                return "cannot create because the file or resource already exists";
            case Error::OpenFailed:
                return "failed to open the file stream or handle";
            case Error::ReadFailed:
                return "failed to read data from the storage device";
            case Error::WriteFailed:
                return "failed to write data to the storage device";
            case Error::UnsupportedFormat:
                return "the provided data format is not supported";
            case Error::ImportFailed:
                return "failed to import the external asset or data";
            case Error::ExportFailed:
                return "failed to export the asset to the specified format";
            case Error::InvalidData:
                return "the data content is corrupted or invalid";
            case Error::ParseError:
                return "failed to parse the structured data (json/xml/etc)";
            case Error::MissingField:
                return "a required data field is missing from the source";
            case Error::TypeMismatch:
                return "the data type does not match the expected type";
            case Error::DirectoryCreationFailed:
                return "failed to create the requested directory path";
            case Error::FileRemovalFailed:
                return "failed to delete the specified file or directory";
            case ReflectionFailed:
                return "failed to perform reflection on the specified type or object";
            default:
                JAM_UNREACHABLE;
        }
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
        JAM_ASSERT(m_bHasValue, "Expect does not contain a value");
        return m_value;
    }

    [[nodiscard]] T& GetValue()
    {
        JAM_ASSERT(m_bHasValue, "Expect does not contain a value");
        return m_value;
    }

    [[nodiscard]] T Take()
    {
        JAM_ASSERT(m_bHasValue, "Expect does not contain a value");
        T value = std::move(m_value);
        Reset_();
        return value;
    }

    [[nodiscard]] Error GetError() const
    {
        JAM_ASSERT(!m_bHasValue, "Expect does not contain an error");
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
