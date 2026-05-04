#include "pch.h"
#include "JsonArchive.h"
#include "File.h"

namespace Jam
{

JsonWriter::JsonWriter()
    : m_ownsDoc(true)
{
    m_pDoc = yyjson_mut_doc_new(nullptr);
    Push_(nullptr, eMode::None);
}

JsonWriter::JsonWriter(const yyjson_alc* _alc)
    : m_ownsDoc(true)
{
    m_pDoc = yyjson_mut_doc_new(_alc);
    Push_(nullptr, eMode::None);
}

JsonWriter::JsonWriter(yyjson_mut_doc* _pDoc)
    : m_pDoc(_pDoc)
    , m_ownsDoc(false)
{
    Push_(yyjson_mut_doc_get_root(m_pDoc), eMode::None);
}

JsonWriter::~JsonWriter()
{
    if (m_ownsDoc && m_pDoc)
    {
        yyjson_mut_doc_free(m_pDoc);
    }
}

JsonWriter::JsonWriter(JsonWriter&& _other) noexcept
    : m_pDoc(_other.m_pDoc)
    , m_ownsDoc(_other.m_ownsDoc)
    , m_pendingKey(_other.m_pendingKey)
    , m_stackTop(_other.m_stackTop)
{
    m_stack           = _other.m_stack;
    _other.m_pDoc     = nullptr;
    _other.m_ownsDoc  = false;
    _other.m_stackTop = -1;
}

JsonWriter& JsonWriter::operator=(JsonWriter&& _other) noexcept
{
    if (this != &_other)
    {
        if (m_ownsDoc && m_pDoc)
        {
            yyjson_mut_doc_free(m_pDoc);
        }

        m_pDoc       = _other.m_pDoc;
        m_ownsDoc    = _other.m_ownsDoc;
        m_pendingKey = _other.m_pendingKey;
        m_stackTop   = _other.m_stackTop;
        m_stack      = _other.m_stack;

        _other.m_pDoc     = nullptr;
        _other.m_ownsDoc  = false;
        _other.m_stackTop = -1;
    }
    return *this;
}

Expect<String> JsonWriter::Stringify(const Flags<eJsonWriteFlags> _flags) const
{
    size_t len  = {};
    char*  json = yyjson_mut_write(m_pDoc, _flags.GetValue(), &len);
    if (!json)
    {
        JAM_LOG_ERROR("YYJson write error: failed to stringify document.");
        return Error::RuntimeError;
    }
    String result { json, static_cast<SizeT>(len) };
    free(json);
    return result;
}

Error JsonWriter::SaveToFile(
    const StringView                 _path,
    const Flags<eJsonWriteFlags>     _flags,
    const eFileWriterCollisionPolicy _policy) const
{
    Expect<FileWriter> writer = FileWriter::Open(_path, _policy);
    if (!writer)
    {
        return writer.GetError();
    }

    size_t len  = {};
    char*  json = yyjson_mut_write(m_pDoc, _flags.GetValue(), &len);
    if (!json)
    {
        JAM_LOG_ERROR("YYJson write error: failed to stringify document.");
        return Error::RuntimeError;
    }

    const Expect<SizeT> ret = writer->WriteString({ json, static_cast<SizeT>(len) });
    free(json);
    if (!ret)
    {
        JAM_LOG_ERROR("Failed to write JSON to file '{}': {}", _path, ret.GetError().ToErrorMessage());
        return ret.GetError();
    }
    return Error::None;
}

void JsonWriter::Attach_(yyjson_mut_val* _val)
{
    Frame& top = Top_();

    switch (top.mode)
    {
        case eMode::None:
        {
            yyjson_mut_doc_set_root(m_pDoc, _val);
            top.node = _val;
            break;
        }
        case eMode::Object:
        {
            JAM_ASSERT(!m_pendingKey.IsEmpty(), "JsonWriter: Object 컨텍스트에서 AddField() 없이 Write/Begin 호출.");
            yyjson_mut_val* key = yyjson_mut_strn(m_pDoc, m_pendingKey.GetPtr(), m_pendingKey.GetLength());
            yyjson_mut_obj_add(top.node, key, _val);
            m_pendingKey = {};
            break;
        }
        case eMode::Array:
        {
            yyjson_mut_arr_append(top.node, _val);
            break;
        }
    }
}

void JsonWriter::BeginObject()
{
    yyjson_mut_val* obj = yyjson_mut_obj(m_pDoc);
    Attach_(obj);
    Push_(obj, eMode::Object);
}

void JsonWriter::EndObject()
{
    JAM_ASSERT(m_stackTop > 0 && Top_().mode == eMode::Object, "JsonWriter: EndObject() 호출 시 현재 스코프가 Object 가 아님.");
    Pop_();
}

void JsonWriter::BeginArray()
{
    yyjson_mut_val* arr = yyjson_mut_arr(m_pDoc);
    Attach_(arr);
    Push_(arr, eMode::Array);
}

void JsonWriter::EndArray()
{
    JAM_ASSERT(m_stackTop > 0 && Top_().mode == eMode::Array, "JsonWriter: EndArray() 호출 시 현재 스코프가 Array 가 아님.");
    Pop_();
}

void JsonWriter::AddField(const StringView _key)
{
    JAM_ASSERT(Top_().mode == eMode::Object, "JsonWriter: AddField() 는 Object 컨텍스트에서만 호출 가능.");
    m_pendingKey = _key;
}

void JsonWriter::AddElement()
{
    JAM_ASSERT(Top_().mode == eMode::Array, "JsonWriter: AddElement() 는 Array 컨텍스트에서만 호출 가능.");
}

void JsonWriter::WritePrimitive_(std::nullptr_t)
{
    Attach_(yyjson_mut_null(m_pDoc));
}

void JsonWriter::WritePrimitive_(const bool _value)
{
    Attach_(yyjson_mut_bool(m_pDoc, _value));
}

void JsonWriter::WritePrimitive_(const Int32 _value)
{
    Attach_(yyjson_mut_int(m_pDoc, _value));
}

void JsonWriter::WritePrimitive_(const UInt32 _value)
{
    Attach_(yyjson_mut_uint(m_pDoc, _value));
}

void JsonWriter::WritePrimitive_(const Int64 _value)
{
    Attach_(yyjson_mut_int(m_pDoc, _value));
}

void JsonWriter::WritePrimitive_(const UInt64 _value)
{
    Attach_(yyjson_mut_uint(m_pDoc, _value));
}

void JsonWriter::WritePrimitive_(const float _value)
{
    Attach_(yyjson_mut_real(m_pDoc, static_cast<double>(_value)));
}

void JsonWriter::WritePrimitive_(const double _value)
{
    Attach_(yyjson_mut_real(m_pDoc, _value));
}

void JsonWriter::WritePrimitive_(const char* _value)
{
    Attach_(yyjson_mut_str(m_pDoc, _value));
}

void JsonWriter::WritePrimitive_(const std::string& _value)
{
    Attach_(yyjson_mut_strn(m_pDoc, _value.c_str(), _value.size()));
}

void JsonWriter::WritePrimitive_(const std::string_view _value)
{
    Attach_(yyjson_mut_strn(m_pDoc, _value.data(), _value.size()));
}

JsonWriter::Frame& JsonWriter::Top_()
{
    return m_stack[m_stackTop];
}

JsonWriter::Frame JsonWriter::Top_() const
{
    return m_stack[m_stackTop];
}

void JsonWriter::Push_(yyjson_mut_val* _node, const eMode _mode)
{
    m_stack[++m_stackTop] = { _node, _mode };
}

void JsonWriter::Pop_()
{
    --m_stackTop;
}

JsonReader::JsonReader(yyjson_doc* _pDoc)
    : m_doc(_pDoc)
{
    Push_(yyjson_doc_get_root(m_doc));
}

JsonReader::~JsonReader()
{
    if (m_doc)
        yyjson_doc_free(m_doc);
}

JsonReader::JsonReader(JsonReader&& _other) noexcept
    : m_doc(_other.m_doc)
    , m_stackTop(_other.m_stackTop)
    , m_lastError(_other.m_lastError)
{
    m_stack           = _other.m_stack;
    _other.m_doc      = nullptr;
    _other.m_stackTop = -1;
}

JsonReader& JsonReader::operator=(JsonReader&& _other) noexcept
{
    if (this != &_other)
    {
        if (m_doc)
        {
            yyjson_doc_free(m_doc);
        }

        m_doc       = _other.m_doc;
        m_stackTop  = _other.m_stackTop;
        m_lastError = _other.m_lastError;
        m_stack     = _other.m_stack;

        _other.m_doc      = nullptr;
        _other.m_stackTop = -1;
    }
    return *this;
}

Expect<JsonReader> JsonReader::Parse(
    const StringView            _json,
    const Flags<eJsonReadFlags> _flags)
{
    yyjson_read_err err  = {};
    yyjson_doc*     pDoc = yyjson_read_opts(const_cast<char*>(_json.GetPtr()), _json.GetLength(), _flags.GetValue(), nullptr, &err);

    if (!pDoc)
    {
        JAM_LOG_ERROR("YYJson parse error: {} at position {}", err.msg, err.pos);
        return Error::ParseError;
    }

    return JsonReader { pDoc };
}

Expect<JsonReader> JsonReader::ParseFromFile(
    const StringView            _path,
    const Flags<eJsonReadFlags> _flags)
{
    Expect<String> data = FileReader::ReadFileAsString(_path);
    if (!data)
    {
        return data.GetError();
    }

    return Parse(*data, _flags);
}

Error JsonReader::EnterField(const StringView _key)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_obj(cur))
    {
        JAM_LOG_ERROR("YYJson EnterField '{}': current node is not an object.", _key);
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }

    yyjson_val* val = yyjson_obj_getn(cur, _key.GetPtr(), _key.GetLength());
    if (!val)
    {
        JAM_LOG_ERROR("YYJson EnterField: key '{}' not found.", _key);
        MarkError(Error::MissingField);
        return Error::MissingField;
    }

    Push_(val);
    return Error::None;
}

Error JsonReader::EnterAt(const SizeT _index)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_arr(cur))
    {
        JAM_LOG_ERROR("YYJson EnterAt {}: current node is not an array.", _index);
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }

    yyjson_val* val = yyjson_arr_get(cur, static_cast<USizeT>(_index));
    if (!val)
    {
        JAM_LOG_ERROR("YYJson EnterAt: index {} out of bounds.", _index);
        MarkError(Error::OutOfRange);
        return Error::OutOfRange;
    }

    Push_(val);
    return Error::None;
}

void JsonReader::Exit()
{
    Pop_();
}

SizeT JsonReader::GetSize() const
{
    yyjson_val* cur = Top_();
    if (yyjson_is_arr(cur))
    {
        return static_cast<SizeT>(yyjson_arr_size(cur));
    }

    if (yyjson_is_obj(cur))
    {
        return static_cast<SizeT>(yyjson_obj_size(cur));
    }

    return 0;
}

bool JsonReader::IsNull() const
{
    return yyjson_is_null(Top_());
}

Error JsonReader::ForEachArray(const Delegate<Error(SizeT, ArchiveReader&)>& _func)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_arr(cur))
    {
        JAM_LOG_ERROR("YYJson ForEachArray: current node is not an array.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }

    SizeT       index = 0, max = 0;
    yyjson_val* val = nullptr;
    yyjson_arr_foreach(cur, index, max, val)
    {
        Push_(val);
        const Error err = _func(index, *this);
        Pop_();
        if (err)
        {
            return err;
        }
    }
    return Error::None;
}

Error JsonReader::ForEachObject(const Delegate<Error(StringView, ArchiveReader&)>& _func)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_obj(cur))
    {
        JAM_LOG_ERROR("YYJson ForEachObject: current node is not an object.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }

    SizeT       index = 0, max = 0;
    yyjson_val *key = nullptr, *val = nullptr;
    yyjson_obj_foreach(cur, index, max, key, val)
    {
        const StringView kv { yyjson_get_str(key), static_cast<SizeT>(yyjson_get_len(key)) };
        Push_(val);
        const Error err = _func(kv, *this);
        Pop_();
        if (err)
        {
            return err;
        }
    }
    return Error::None;
}

bool JsonReader::IsFine() const
{
    return m_lastError.IsOk();
}
void JsonReader::MarkError(const Error _err)
{
    m_lastError = _err;
}
Error JsonReader::GetLastError() const
{
    return m_lastError;
}
bool JsonReader::HasError() const
{
    return m_lastError.IsError();
}
void JsonReader::ClearLastError()
{
    m_lastError = Error::None;
}

Error JsonReader::ReadPrimitive_(std::nullptr_t&)
{
    if (!yyjson_is_null(Top_()))
    {
        JAM_LOG_ERROR("YYJson read error: expected null.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    return Error::None;
}

Error JsonReader::ReadPrimitive_(bool& _value)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_bool(cur))
    {
        JAM_LOG_ERROR("YYJson read error: expected bool.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = yyjson_get_bool(cur);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(Int32& _value)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_int(cur))
    {
        JAM_LOG_ERROR("YYJson read error: expected int.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = static_cast<Int32>(yyjson_get_sint(cur));
    return Error::None;
}

Error JsonReader::ReadPrimitive_(UInt32& _value)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_uint(cur))
    {
        JAM_LOG_ERROR("YYJson read error: expected uint.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = static_cast<UInt32>(yyjson_get_uint(cur));
    return Error::None;
}

Error JsonReader::ReadPrimitive_(Int64& _value)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_int(cur))
    {
        JAM_LOG_ERROR("YYJson read error: expected int64.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = yyjson_get_sint(cur);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(UInt64& _value)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_uint(cur))
    {
        JAM_LOG_ERROR("YYJson read error: expected uint64.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = yyjson_get_uint(cur);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(float& _value)
{
    yyjson_val* cur = Top_();
    if (yyjson_is_real(cur))
    {
        _value = static_cast<float>(yyjson_get_real(cur));
    }
    else if (yyjson_is_sint(cur))
    {
        _value = static_cast<float>(yyjson_get_sint(cur));
    }
    else if (yyjson_is_uint(cur))
    {
        _value = static_cast<float>(yyjson_get_uint(cur));
    }
    else
    {
        JAM_LOG_ERROR("YYJson read error: expected number (float).");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    return Error::None;
}

Error JsonReader::ReadPrimitive_(double& _value)
{
    yyjson_val* cur = Top_();
    if (yyjson_is_real(cur))
    {
        _value = yyjson_get_real(cur);
    }
    else if (yyjson_is_sint(cur))
    {
        _value = static_cast<double>(yyjson_get_sint(cur));
    }
    else if (yyjson_is_uint(cur))
    {
        _value = static_cast<double>(yyjson_get_uint(cur));
    }
    else
    {
        JAM_LOG_ERROR("YYJson read error: expected number (double).");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    return Error::None;
}

Error JsonReader::ReadPrimitive_(const char*& _value)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_str(cur))
    {
        JAM_LOG_ERROR("YYJson read error: expected string (const char*).");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = yyjson_get_str(cur);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(std::string& _value)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_str(cur))
    {
        JAM_LOG_ERROR("YYJson read error: expected string (std::string).");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value.assign(yyjson_get_str(cur), yyjson_get_len(cur));
    return Error::None;
}

Error JsonReader::ReadPrimitive_(std::string_view& _value)
{
    yyjson_val* cur = Top_();
    if (!yyjson_is_str(cur))
    {
        JAM_LOG_ERROR("YYJson read error: expected string (std::string_view).");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = { yyjson_get_str(cur), yyjson_get_len(cur) };
    return Error::None;
}

yyjson_val* JsonReader::Top_() const
{
    return m_stack[m_stackTop];
}

void JsonReader::Push_(yyjson_val* _val)
{
    m_stack[++m_stackTop] = _val;
}

void JsonReader::Pop_()
{
    --m_stackTop;
}

}   // namespace Jam