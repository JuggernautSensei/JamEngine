#pragma once
#include "EnumArray.h"

namespace Jam
{

class ArchiveWriter;
class ArchiveReader;

template<typename T>
concept HasIntrusiveWriteFunc =
    requires(ArchiveWriter& ar, T value) { value.WriteToArchive(ar); };

template<typename T>
concept HasIntrusiveReadFunc =
    requires(ArchiveReader& ar, T value) { { value.ReadFromArchive(ar) } -> std::same_as<Error>; };

template<typename T>
concept HasNonIntrusiveWriteFunc =
    requires(ArchiveWriter& ar, T value) { WriteToArchive(ar, value); };

template<typename T>
concept HasNonIntrusiveReadFunc =
    requires(ArchiveReader& ar, T& value) { { ReadFromArchive(ar, value) } -> std::same_as<Error>; };

template<typename T>
concept IsArchivePrimitive =
    std::is_null_pointer_v<T>
    || std::is_same_v<T, bool>
    || std::is_integral_v<T>
    || std::is_floating_point_v<T>
    || std::is_same_v<T, const char*>
    || std::is_same_v<T, std::string>
    || std::is_same_v<T, std::string_view>
    || std::is_same_v<T, String>
    || std::is_same_v<T, StringView>;

template<typename T>
concept IsArchiveSerializable =
    ((HasIntrusiveWriteFunc<T> || HasNonIntrusiveWriteFunc<T>)
     && (HasIntrusiveReadFunc<T> || HasNonIntrusiveReadFunc<T>))
    || IsArchivePrimitive<T>;

#define JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(T)               \
    static_assert(Jam::IsArchiveSerializable<T>,                        \
                  "Type '" #T "' is not archive serializable. "         \
                  "Please implement either intrusive or non-intrusive " \
                  "WriteToArchive() and ReadFromArchive() functions, "  \
                  "or make it a primitive type.")

enum class eErrorPolicy
{
    Ignore,
    Stop,
};

class ArchiveWriter
{
public:
    ArchiveWriter()                                = default;
    virtual ~ArchiveWriter()                       = default;
    ArchiveWriter(const ArchiveWriter&)            = default;
    ArchiveWriter& operator=(const ArchiveWriter&) = default;
    ArchiveWriter(ArchiveWriter&&)                 = default;
    ArchiveWriter& operator=(ArchiveWriter&&)      = default;

    virtual void BeginObject() = 0;
    virtual void EndObject()   = 0;
    virtual void BeginArray()  = 0;
    virtual void EndArray()    = 0;

    virtual void AddField(StringView _key) = 0;
    virtual void AddElement()              = 0;

    template<typename T>
    void Write(T&& _value)
    {
        using U = std::decay_t<T>;
        if constexpr (HasIntrusiveWriteFunc<U>)
        {
            _value.WriteToArchive(*this);
        }
        else if constexpr (HasNonIntrusiveWriteFunc<U>)
        {
            WriteToArchive(*this, _value);
        }
        else if constexpr (IsArchivePrimitive<U>)
        {
            WritePrimitive_(std::forward<T>(_value));
        }
        else
        {
            static_assert(AlwaysFalseV<T>, "Type is not serializable. Implement WriteToArchive() or make it a primitive.");
        }
    }

    template<typename T>
    void WriteField(const StringView _key, T&& _value)   // AddField + Write shortcut
    {
        AddField(_key);
        Write(std::forward<T>(_value));
    }

    template<typename T>
    void WriteElement(T&& _value)   // AddElement + Write shortcut
    {
        AddElement();
        Write(std::forward<T>(_value));
    }

protected:
    enum class eMode
    {
        None,
        Object,
        Array,
    };

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

    virtual SizeT GetSize() const = 0;
    virtual bool  IsNull() const  = 0;

    virtual bool  IsFine() const        = 0;
    virtual void  MarkError(Error _err) = 0;
    virtual Error GetLastError() const  = 0;
    virtual bool  HasError() const      = 0;
    virtual void  ClearLastError()      = 0;

    virtual Error ForEachArray(const Delegate<Error(SizeT, ArchiveReader&)>& _func)       = 0;
    virtual Error ForEachObject(const Delegate<Error(StringView, ArchiveReader&)>& _func) = 0;

    template<typename T>
        requires(!std::is_const_v<T>)
    Error Read(T& _value)
    {
        using U = std::decay_t<T>;
        if constexpr (HasIntrusiveReadFunc<U>)
        {
            const Error err = _value.ReadFromArchive(*this);
            if (err)
            {
                JAM_LOG_ERROR("Archive read error: intrusive ReadFromArchive() failed for '{}'.", NameOf<U>());
            }

            return err;
        }
        else if constexpr (HasNonIntrusiveReadFunc<U>)
        {
            const Error err = ReadFromArchive(*this, _value);
            if (err)
            {
                JAM_LOG_ERROR("Archive read error: non-intrusive ReadFromArchive() failed for '{}'.", NameOf<U>());
            }

            return err;
        }
        else if constexpr (IsArchivePrimitive<U>)
        {
            return ReadPrimitive_(_value);
        }
        else
        {
            static_assert(AlwaysFalseV<T>,
                          "Type is not deserializable. "
                          "Implement ReadFromArchive() or make it a primitive.");
            return Error::NotImplemented;
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
};

////////// ENUM //////////

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

    const Optional<TEnum> value = ParseAs<TEnum>(name);
    if (!value)
    {
        JAM_LOG_ERROR("Archive read error: failed to parse '{}' as enum of type '{}'.", name, NameOf<TEnum>());
        _reader.MarkError(Error::ParseError);
        return Error::ParseError;
    }

    _value = value.GetValue();
    return Error::None;
}

////////// VECTOR<T> //////////

template<typename T>
void WriteToArchive(ArchiveWriter& _writer, const Vector<T>& _vec)
{
    _writer.BeginArray();
    for (const T& item: _vec)
    {
        _writer.WriteElement(item);
    }
    _writer.EndArray();
}

template<typename T>
Error ReadFromArchive(ArchiveReader& _reader, Vector<T>& _vec)
{
    _vec.clear();
    _vec.reserve(_reader.GetSize());
    return _reader.ForEachArray(Bind(
        [&](SizeT, ArchiveReader& ar) -> Error {
            return ar.Read(_vec.emplace_back());
        }));
}

////////// Array<T, N> //////////

template<typename T, SizeT N>
void WriteToArchive(ArchiveWriter& _writer, const Array<T, N>& _arr)
{
    _writer.BeginArray();
    for (const T& value: _arr)
    {
        _writer.WriteElement(value);
    }
    _writer.EndArray();
}

template<typename T, SizeT k_size>
Error ReadFromArchive(ArchiveReader& _reader, Array<T, k_size>& _arr)
{
    const SizeT n = _reader.GetSize();
    if (n > k_size)
    {
        JAM_LOG_ERROR("Archive read error: array size {} exceeds fixed capacity {}.", n, k_size);
        _reader.MarkError(Error::InvalidData);
        return Error::InvalidData;
    }

    return _reader.ForEachArray(Bind(
        [&](SizeT idx, ArchiveReader& ar) -> Error {
            return ar.Read(_arr[idx]);
        }));
}

template<typename TEnum, typename TValue>
void WriteToArchive(ArchiveWriter& _writer, const EnumArray<TEnum, TValue>& _enumArray)
{
    _writer.BeginObject();
    for (const TEnum key: RangesOf<TEnum>())
    {
        _writer.WriteField(NameOf(key), _enumArray[key]);
    }
    _writer.EndObject();
}

template<typename TEnum, typename TValue>
Error ReadFromArchive(ArchiveReader& _reader, EnumArray<TEnum, TValue>& _enumArray)
{
    return _reader.ForEachObject(Bind(
        [&](StringView _key, ArchiveReader& ar) -> Error {
            const Optional<TEnum> e = ParseAs<TEnum>(_key);
            if (!e)
            {
                JAM_LOG_ERROR("Archive read error: failed to parse '{}' as enum key of type '{}'.", _key, NameOf<TEnum>());
                ar.MarkError(Error::ParseError);
                return Error::ParseError;
            }
            return ar.Read(_enumArray[e.GetValue()]);
        }));
}

////////// misc ////////

void  WriteToArchive(ArchiveWriter& _writer, const String& _value);
Error ReadFromArchive(ArchiveReader& _reader, String& _value);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(String);

void  WriteToArchive(ArchiveWriter& _writer, StringView _value);
Error ReadFromArchive(ArchiveReader& _reader, StringView& _value);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(StringView);

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