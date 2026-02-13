#pragma once

namespace Jam
{

template<typename T>
constexpr bool k_alwaysFalseV = false;

class ArchiveWriter;
class ArchiveReader;

template<typename T>
concept HasIntrusiveWriteFunc = requires(
    ArchiveWriter& _archive,
    T              _value) {
    { _value.WriteToArchive(_archive) } -> std::same_as<void>;
};

template<typename T>
concept HasIntrusiveReadFunc = requires(
    ArchiveReader& _archive,
    T&             _value) {
    { _value.ReadFromArchive(_archive) } -> std::same_as<Error>;

    requires !requires(const T& _cvlaue) {
        _cvlaue.ReadFromArchive(_archive);
    };
};

template<typename T>
concept HasNonIntrusiveWriteFunc = requires(
    ArchiveWriter& _archive,
    T              _value) {
    { WriteToArchive(_archive, _value) } -> std::same_as<void>;
};

template<typename T>
concept HasNonIntrusiveReadFunc = requires(
    ArchiveReader& _archive,
    T&             _value) {
    { ReadFromArchive(_archive, _value) } -> std::same_as<Error>;

    requires !requires(T&& _rvalue) {
        ReadFromArchive(_archive, std::move(_rvalue));
    };
};

template<typename T>
concept IsArchivePrimitive = std::is_null_pointer_v<T>
                          || std::is_same_v<T, bool>
                          || std::is_integral_v<T>
                          || std::is_floating_point_v<T>
                          || std::is_same_v<T, const char*>
                          || std::is_same_v<T, std::string>
                          || std::is_same_v<T, std::string_view>
                          || std::is_same_v<T, String>
                          || std::is_same_v<T, StringView>;

template<typename T>
concept IsArchiveSerializable = ((HasIntrusiveWriteFunc<T> || HasNonIntrusiveWriteFunc<T>) && (HasIntrusiveReadFunc<T> || HasNonIntrusiveReadFunc<T>))
                             || IsArchivePrimitive<T>;

#define JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(T) \
    static_assert(Jam::IsArchiveSerializable<T>, "Type '" #T "' is not _archive serializable. Please implement either intrusive or non-intrusive WriteToArchive() and ReadFromArchive() functions, or make it a primitive type.")

enum class eErrorPolicy
{
    Ignore,
    Stop,
};

class ArchiveWriter
{
public:
    ArchiveWriter()          = default;
    virtual ~ArchiveWriter() = default;

    ArchiveWriter(const ArchiveWriter&)            = default;
    ArchiveWriter& operator=(const ArchiveWriter&) = default;
    ArchiveWriter(ArchiveWriter&&)                 = default;
    ArchiveWriter& operator=(ArchiveWriter&&)      = default;

    virtual void OpenField(StringView _key) = 0;
    virtual void OpenAtBack()               = 0;
    virtual void Close()                    = 0;

    template<typename T>
    void Write(T&& _value)
    {
        using TDecayed = std::decay_t<T>;
        if constexpr (HasIntrusiveWriteFunc<TDecayed>)
        {
            _value.WriteToArchive(*this);
        }
        else if constexpr (HasNonIntrusiveWriteFunc<TDecayed>)
        {
            WriteToArchive(*this, _value);
        }
        else
        {
            WritePrimitive_(std::forward<T>(_value));
        }
    }

    template<typename T>
    void WriteField(const StringView _key, T&& _value)
    {
        OpenField(_key);
        Write(std::forward<T>(_value));
        Close();
    }

    template<typename T>
    void PushBack(T&& _value)
    {
        OpenAtBack();
        Write(std::forward<T>(_value));
        Close();
    }

protected:
    virtual void WritePrimitive_(std::nullptr_t)            = 0;
    virtual void WritePrimitive_(bool _value)               = 0;
    virtual void WritePrimitive_(Int32 _value)              = 0;
    virtual void WritePrimitive_(UInt32 _value)             = 0;
    virtual void WritePrimitive_(Int64 _value)              = 0;
    virtual void WritePrimitive_(UInt64 _value)             = 0;
    virtual void WritePrimitive_(float _value)              = 0;
    virtual void WritePrimitive_(double _value)             = 0;
    virtual void WritePrimitive_(const char* _value)        = 0;
    virtual void WritePrimitive_(const std::string& _value) = 0;
    virtual void WritePrimitive_(std::string_view _value)   = 0;
    virtual void WritePrimitive_(const String& _value)      = 0;
    virtual void WritePrimitive_(StringView _value)         = 0;

    Error m_lastError = Error::None;
};

class ArchiveReader
{
public:
    ArchiveReader()                                = default;
    virtual ~ArchiveReader()                       = default;
    ArchiveReader(const ArchiveReader&)            = default;
    ArchiveReader& operator=(const ArchiveReader&) = default;
    ArchiveReader(ArchiveReader&&)                 = default;
    ArchiveReader& operator=(ArchiveReader&&)      = default;

    virtual Error EnterField(StringView _key) = 0;
    virtual Error EnterAt(SizeT _idx)         = 0;
    virtual void  Exit()                      = 0;
    virtual SizeT GetSize() const             = 0;
    virtual bool  IsNull() const              = 0;
    virtual bool  IsFine() const              = 0;
    virtual void  MarkError(Error _err)       = 0;
    virtual Error GetLastError() const        = 0;
    virtual bool  HasError() const            = 0;
    virtual void  ClearLastError()            = 0;

    template<typename T>
    Error Read(T& _value)
    {
        using TDecayed = std::decay_t<T>;
        if constexpr (HasIntrusiveReadFunc<TDecayed>)
        {
            const Error err = _value.ReadFromArchive(*this);
            if (err)
            {
                JAM_LOG.Error("Archive read error: failed to read '{}' data via intrusive ReadFromArchive().", NameOf<TDecayed>());
            }

            return err;
        }
        else if constexpr (HasNonIntrusiveReadFunc<TDecayed>)
        {
            const Error err = ReadFromArchive(*this, _value);
            if (err)
            {
                JAM_LOG.Error("Archive read error: failed to read '{}' data via non-intrusive ReadFromArchive().", NameOf<TDecayed>());
            }

            return err;
        }
        else
        {
            return ReadPrimitive_(_value);
        }
    }

    template<typename T>
    Error ReadField(const StringView _key, T& _value)
    {
        if (const Error err = EnterField(_key))
        {
            return err;
        }

        const Error err = Read(_value);
        Exit();
        return err;
    }

    template<typename T>
    Error ReadAt(const SizeT _idx, T& _value)
    {
        if (const Error err = EnterAt(_idx))
        {
            return err;
        }

        const Error err = Read(_value);
        Exit();
        return err;
    }

    template<typename Func>
        requires std::is_invocable_r_v<Error, Func, SizeT, ArchiveReader&>
    Error ForEach(Func&& _func)
    {
        const SizeT size = GetSize();
        for (SizeT i = 0; i < size; ++i)
        {
            if (const Error err = EnterAt(i))
            {
                return err;
            }

            const Error err = _func(i, *this);
            Exit();
            if (err)
            {
                return err;
            }
        }
        return Error::None;
    }

protected:
    virtual Error ReadPrimitive_(std::nullptr_t&)          = 0;
    virtual Error ReadPrimitive_(bool& _value)             = 0;
    virtual Error ReadPrimitive_(Int32& _value)            = 0;
    virtual Error ReadPrimitive_(UInt32& _value)           = 0;
    virtual Error ReadPrimitive_(Int64& _value)            = 0;
    virtual Error ReadPrimitive_(UInt64& _value)           = 0;
    virtual Error ReadPrimitive_(float& _value)            = 0;
    virtual Error ReadPrimitive_(double& _value)           = 0;
    virtual Error ReadPrimitive_(const char*& _value)      = 0;
    virtual Error ReadPrimitive_(std::string& _value)      = 0;
    virtual Error ReadPrimitive_(std::string_view& _value) = 0;
    virtual Error ReadPrimitive_(String& _value)           = 0;
    virtual Error ReadPrimitive_(StringView& _value)       = 0;
};

////////// jam engine glue //////////

template<typename TEnum>
    requires std::is_enum_v<TEnum>
void WriteToArchive(ArchiveWriter& _writer, const TEnum _value)
{
    _writer.Write(NameOf(_value));
}

template<typename TEnum>
    requires std::is_enum_v<TEnum>
Error ReadFromArchive(ArchiveReader& _reader, TEnum& _value)
{
    StringView name = {};
    if (const Error err = _reader.Read(name))
    {
        return err;
    }

    Optional<TEnum> value = ParseAs<TEnum>(name);
    if (!value)
    {
        JAM_LOG.Error("Archive read error: failed to parse '{}' as enum _value of type '{}'.", name, NameOf<TEnum>());
        _reader.MarkError(Error::ParseError);
        return Error::ParseError;
    }

    _value = value.GetValue();
    return Error::None;
}

template<typename T>
void WriteToArchive(ArchiveWriter& _writer, const Vector<T>& _vec)
{
    for (const T& item: _vec)
    {
        _writer.PushBack(item);
    }
}

template<typename T>
Error ReadFromArchive(ArchiveReader& _reader, Vector<T>& _vec)
{
    const SizeT size = _reader.GetSize();
    _vec.clear();
    _vec.reserve(size);
    return _reader.ForEach(
        [&](SizeT, ArchiveReader& reader) -> Error
        {
            if (const Error err = reader.Read(_vec.emplace_back()))
            {
                return err;
            }

            return Error::None;
        });
}

template<typename T, SizeT N>
void WriteToArchive(ArchiveWriter& _writer, const Array<T, N>& _arr)
{
    for (const T& item: _arr)
    {
        _writer.PushBack(item);
    }
}

template<typename T, SizeT N>
Error ReadFromArchive(ArchiveReader& _reader, Array<T, N>& _arr)
{
    const SizeT size = _reader.GetSize();
    if (CmpNotEqual(size, N))
    {
        JAM_LOG.Error("Archive read error: expected array size {} but got {}.", N, size);
        _reader.MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }

    return _reader.ForEach(
        [&](SizeT idx, ArchiveReader& reader) -> Error
        {
            if (const Error err = reader.Read(_arr[idx]))
            {
                return err;
            }

            return Error::None;
        });
}

// math
void  WriteToArchive(ArchiveWriter& _writer, Vec3 _value);
Error ReadFromArchive(ArchiveReader& _reader, Vec3& _value);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Vec3);

void  WriteToArchive(ArchiveWriter& _writer, Vec2 _value);
Error ReadFromArchive(ArchiveReader& _reader, Vec2& _value);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Vec2);

void  WriteToArchive(ArchiveWriter& _writer, const Mtx4& _value);
Error ReadFromArchive(ArchiveReader& _reader, Mtx4& _value);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Mtx4);

}   // namespace Jam