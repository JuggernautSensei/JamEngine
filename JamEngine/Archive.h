#pragma once

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

    virtual void WritePrimitive_(std::nullptr_t)  = 0;
    virtual void WritePrimitive_(bool _value)     = 0;
    virtual void WritePrimitive_(Int32 _value)    = 0;
    virtual void WritePrimitive_(UInt32 _value)   = 0;
    virtual void WritePrimitive_(Int64 _value)    = 0;
    virtual void WritePrimitive_(UInt64 _value)   = 0;
    virtual void WritePrimitive_(float _value)    = 0;
    virtual void WritePrimitive_(double _value)   = 0;
    virtual void WritePrimitive_(StringView _str) = 0;
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
    virtual Error EnterAt(SizeT _index)       = 0;
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
    Error ReadAt(const SizeT _index, T& _value)
    {
        if (const Error err = EnterAt(_index))
        {
            return err;
        }

        const Error err = Read(_value);
        Exit();
        return err;
    }

protected:
    virtual Error ReadPrimitive_(std::nullptr_t&)    = 0;
    virtual Error ReadPrimitive_(bool& _value)       = 0;
    virtual Error ReadPrimitive_(Int32& _value)      = 0;
    virtual Error ReadPrimitive_(UInt32& _value)     = 0;
    virtual Error ReadPrimitive_(Int64& _value)      = 0;
    virtual Error ReadPrimitive_(UInt64& _value)     = 0;
    virtual Error ReadPrimitive_(float& _value)      = 0;
    virtual Error ReadPrimitive_(double& _value)     = 0;
    virtual Error ReadPrimitive_(String& _value)     = 0;
    virtual Error ReadPrimitive_(StringView& _value) = 0;
};

////////// SPECIALIZATIONS //////////

////////// ENUM ///////////

template<typename TEnum>
    requires(std::is_enum_v<TEnum>)
void WriteToArchive(ArchiveWriter& _ar, TEnum _value)
{
    _ar.Write(NameOf(_value));
}

template<typename TEnum>
    requires(std::is_enum_v<TEnum>)
Error ReadFromArchive(ArchiveReader& _ar, TEnum& _value)
{
    StringView str = {};
    if (const Error err = _ar.Read(str))
    {
        return err;
    }

    Optional<TEnum> e = ParseAs<TEnum>(str);
    if (!e)
    {
        _ar.MarkError(Error::InvalidData);
        return Error::InvalidData;
    }

    _value = e.GetValue();
    return Error::None;
}

/////// CONTAINERS ////////

template<typename T>
void WriteToArchive(ArchiveWriter& _ar, const Vector<T>& _vec)
{
    _ar.BeginArray();
    for (const T& elem: _vec)
    {
        _ar.WriteElement(elem);
    }
    _ar.EndArray();
}

template<typename T>
Error ReadFromArchive(ArchiveReader& _ar, Vector<T>& _vec)
{
    _vec.clear();
    _vec.reserve(_ar.GetSize());
    return _ar.ForEachArray([&](SizeT, ArchiveReader& elemAr) -> Error {
        T elem = {};
        if (const Error err = elemAr.Read(elem))
        {
            return err;
        }
        _vec.emplace_back(std::move(elem));
        return Error::None;
    });
}

template<typename TKey, typename TValue>
void WriteToArchive(ArchiveWriter& _ar, const HashMap<TKey, TValue>& _map)
{
    _ar.BeginObject();
    for (const auto& [key, value]: _map)
    {
        _ar.WriteField(key, value);
    }
    _ar.EndObject();
}

template<typename TKey>
void WriteToArchive(ArchiveWriter& _ar, const HashSet<TKey>& _set)
{
    _ar.BeginArray();
    for (const TKey& key: _set)
    {
        _ar.WriteElement(key);
    }
    _ar.EndArray();
}

template<typename TKey, typename TValue>
Error ReadFromArchive(ArchiveReader& _ar, HashMap<TKey, TValue>& _map)
{
    _map.clear();
    _map.reserve(_ar.GetSize());
    return _ar.ForEachObject([&](StringView key, ArchiveReader& valueAr) -> Error {
        TValue value = {};
        if (const Error err = valueAr.Read(value))
        {
            return err;
        }
        _map.emplace(key, std::move(value));
        return Error::None;
    });
}

template<typename TKey>
Error ReadFromArchive(ArchiveReader& _ar, HashSet<TKey>& _set)
{
    _set.clear();
    return _ar.ForEachArray([&](SizeT, ArchiveReader& elemAr) -> Error {
        TKey key = {};
        if (const Error err = elemAr.Read(key))
        {
            return err;
        }
        _set.emplace(std::move(key));
        return Error::None;
    });
}

template<typename T, SizeT kSize>
void WriteToArchive(ArchiveWriter& _ar, const Array<T, kSize>& _arr)
{
    _ar.BeginArray();
    for (const T& elem: _arr)
    {
        _ar.WriteElement(elem);
    }
    _ar.EndArray();
}

template<typename T, SizeT kSize>
Error ReadFromArchive(ArchiveReader& _ar, Array<T, kSize>& _arr)
{
    if (_ar.GetSize() != kSize)
    {
        _ar.MarkError(Error::InvalidData);
        return Error::InvalidData;
    }

    return _ar.ForEachArray(Bind([&](SizeT _index, ArchiveReader& _elem) -> Error {
        if (const Error err = _elem.Read(_arr[_index]))
        {
            return err;
        }
        return Error::None;
    }));
}

////////// MATH TYPES //////////

void WriteToArchive(ArchiveWriter& _ar, Vec2 _v);
void WriteToArchive(ArchiveWriter& _ar, Vec3 _v);
void WriteToArchive(ArchiveWriter& _ar, Vec4 _v);
void WriteToArchive(ArchiveWriter& _ar, Vec2i _v);
void WriteToArchive(ArchiveWriter& _ar, Vec3i _v);
void WriteToArchive(ArchiveWriter& _ar, Vec4i _v);
void WriteToArchive(ArchiveWriter& _ar, Quat _q);
void WriteToArchive(ArchiveWriter& _ar, const Mtx4& _m);

Error ReadFromArchive(ArchiveReader& _ar, Vec2& _v);
Error ReadFromArchive(ArchiveReader& _ar, Vec3& _v);
Error ReadFromArchive(ArchiveReader& _ar, Vec4& _v);
Error ReadFromArchive(ArchiveReader& _ar, Vec2i& _v);
Error ReadFromArchive(ArchiveReader& _ar, Vec3i& _v);
Error ReadFromArchive(ArchiveReader& _ar, Vec4i& _v);
Error ReadFromArchive(ArchiveReader& _ar, Quat& _q);
Error ReadFromArchive(ArchiveReader& _ar, Mtx4& _m);

}   // namespace Jam