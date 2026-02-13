#include "pch.h"
#include "JsonArchive.h"

#include "File.h"

namespace Jam
{

JsonWriter::JsonWriter()
    : m_ownsDoc(true)
{
    m_pDoc               = yyjson_mut_doc_new(nullptr);
    yyjson_mut_val* root = yyjson_mut_obj(m_pDoc);
    yyjson_mut_doc_set_root(m_pDoc, root);
    m_valueStack.PushBack(root);
}

JsonWriter::JsonWriter(
    yyjson_mut_doc* _pDoc)
    : m_pDoc(_pDoc)
      , m_ownsDoc(false)
{
    yyjson_mut_val* root = yyjson_mut_doc_get_root(m_pDoc);
    m_valueStack.PushBack(root);
}

JsonWriter::~JsonWriter()
{
    if (m_ownsDoc && m_pDoc)
    {
        yyjson_mut_doc_free(m_pDoc);
    }
}

JsonWriter::JsonWriter(
    JsonWriter&& _other) noexcept
    : m_pDoc(_other.m_pDoc)
      , m_valueStack(std::move(_other.m_valueStack))
      , m_ownsDoc(_other.m_ownsDoc)
{
    _other.m_pDoc    = nullptr;
    _other.m_ownsDoc = false;
    _other.m_valueStack.Clear();
}

JsonWriter& JsonWriter::operator=(
    JsonWriter&& _other) noexcept
{
    if (this != &_other)
    {
        if (m_ownsDoc && m_pDoc)
        {
            yyjson_mut_doc_free(m_pDoc);
        }

        m_pDoc       = _other.m_pDoc;
        m_valueStack = std::move(_other.m_valueStack);
        m_ownsDoc    = _other.m_ownsDoc;

        _other.m_pDoc    = nullptr;
        _other.m_ownsDoc = false;
        _other.m_valueStack.Clear();
    }
    return *this;
}

Expect<String> JsonWriter::Stringify(
    const Flags<eJsonWriteFlags> _flags) const
{
    size_t len  = {};
    char*  json = yyjson_mut_write(m_pDoc, _flags.GetValue(), &len);
    if (!json)
    {
        JAM_LOG.Error("YYJson serialization error: failed to write document.");
        return Error::RuntimeError;
    }

    String result { json, NumericCast<SizeT>(len) };
    free(json);
    return result;
}

Error JsonWriter::SaveToFile(
    const Path&                  _path,
    const Flags<eJsonWriteFlags> _writeFlags,
    const Flags<eSaveFileFlags>  _saveFlags) const
{
    Expect<FileWriter> writer = FileWriter::Open(_path, _saveFlags);
    if (!writer)
    {
        return writer.GetError();
    }

    size_t len  = {};
    char*  json = yyjson_mut_write(m_pDoc, _writeFlags, &len);
    if (!json)
    {
        JAM_LOG.Error("YYJson serialization error: failed to write document.");
        return Error::RuntimeError;
    }

    const Error err = writer->Write(CMemoryBlock { json, NumericCast<SizeT>(len) });
    free(json);
    return err;
}

void JsonWriter::OpenField(
    const StringView _key)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    if (yyjson_mut_is_null(pCur))
    {
        yyjson_mut_obj_clear(pCur);
    }

    yyjson_mut_val* newVal = yyjson_mut_null(m_pDoc);
    yyjson_mut_val* key    = yyjson_mut_strncpy(m_pDoc, _key.GetPtr(), _key.GetLength());
    yyjson_mut_obj_add(pCur, key, newVal);

    yyjson_mut_val* lastVal = yyjson_mut_obj_getn(pCur, _key.GetPtr(), _key.GetLength());
    m_valueStack.PushBack(lastVal);
}

void JsonWriter::OpenAtBack()
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    if (yyjson_mut_is_null(pCur))
    {
        yyjson_mut_arr_clear(pCur);
    }

    yyjson_mut_val* newVal = yyjson_mut_null(m_pDoc);
    yyjson_mut_arr_append(pCur, newVal);

    size_t          size    = yyjson_mut_arr_size(pCur);
    yyjson_mut_val* lastVal = yyjson_mut_arr_get(pCur, size - 1);
    m_valueStack.PushBack(lastVal);
}

void JsonWriter::Close()
{
    m_valueStack.PopBack();
}

void JsonWriter::WritePrimitive_(std::nullptr_t)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_null(pCur);
}

void JsonWriter::WritePrimitive_(
    const bool _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_bool(pCur, _value);
}

void JsonWriter::WritePrimitive_(
    const Int32 _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_sint(pCur, static_cast<int64_t>(_value));
}

void JsonWriter::WritePrimitive_(
    const UInt32 _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_uint(pCur, static_cast<uint64_t>(_value));
}

void JsonWriter::WritePrimitive_(
    const Int64 _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_sint(pCur, _value);
}

void JsonWriter::WritePrimitive_(
    const UInt64 _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_uint(pCur, _value);
}

void JsonWriter::WritePrimitive_(
    const float _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_real(pCur, static_cast<double>(_value));
}

void JsonWriter::WritePrimitive_(
    const double _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_real(pCur, _value);
}

void JsonWriter::WritePrimitive_(
    const char* _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_str(pCur, _value);
}

void JsonWriter::WritePrimitive_(
    const std::string& _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_strn(pCur, _value.c_str(), _value.size());
}

void JsonWriter::WritePrimitive_(
    const std::string_view _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_strn(pCur, _value.data(), _value.size());
}

void JsonWriter::WritePrimitive_(const String& _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_strn(pCur, _value.GetPtr(), _value.GetLength());
}

void JsonWriter::WritePrimitive_(
    const StringView _value)
{
    yyjson_mut_val* pCur = m_valueStack.Back();
    yyjson_mut_set_strn(pCur, _value.GetPtr(), _value.GetLength());
}

JsonReader::~JsonReader()
{
    if (m_doc)
    {
        yyjson_doc_free(m_doc);
    }
}

JsonReader::JsonReader(
    JsonReader&& _other) noexcept
    : m_doc(_other.m_doc)
      , m_valueStack(std::move(_other.m_valueStack))
{
    _other.m_doc = nullptr;
    _other.m_valueStack.Clear();
}

JsonReader& JsonReader::operator=(
    JsonReader&& _other) noexcept
{
    if (this != &_other)
    {
        if (m_doc)
        {
            yyjson_doc_free(m_doc);
        }

        m_doc        = _other.m_doc;
        m_valueStack = std::move(_other.m_valueStack);

        _other.m_doc = nullptr;
        _other.m_valueStack.Clear();
    }
    return *this;
}

JsonReader::JsonReader(yyjson_doc* _pDoc)
    : m_doc(_pDoc)
{
    yyjson_val* root = yyjson_doc_get_root(m_doc);
    m_valueStack.PushBack(root);
}

Expect<JsonReader> JsonReader::Parse(
    const StringView            _json,
    const Flags<eJsonReadFlags> _flags)
{
    yyjson_read_err err  = {};
    yyjson_doc*     pDoc = yyjson_read_opts(const_cast<char*>(_json.GetPtr()), _json.GetLength(), _flags, nullptr, &err);
    if (!pDoc)
    {
        JAM_LOG.Error("YYJson parse error: {} at position {}", err.msg, err.pos);
        return Error::ParseError;
    }

    return JsonReader { pDoc };
}

Expect<JsonReader> JsonReader::ParseFromFile(
    const Path&                 _path,
    const Flags<eJsonReadFlags> _flags)
{
    Expect<String> data = FileReader::ReadFileAsString(_path);
    if (!data)
    {
        return data.GetError();
    }

    return Parse(data.GetValue(), _flags);
}

Error JsonReader::EnterField(StringView _key)
{
    yyjson_val* pValue = m_valueStack.Back();
    if (!yyjson_is_obj(pValue))
    {
        JAM_LOG.Error("YYJson enter field error: data type not matched (expected object type).");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }

    yyjson_val* _pVal = yyjson_obj_getn(pValue, _key.GetPtr(), _key.GetLength());
    if (!_pVal)
    {
        JAM_LOG.Error("YYJson enter field error: missing field \"{}\".", _key);
        MarkError(Error::MissingField);
        return Error::MissingField;
    }

    m_valueStack.PushBack(_pVal);
    return Error::None;
}

Error JsonReader::EnterAt(const SizeT _idx)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_arr(pCur))
    {
        JAM_LOG.Error("YYJson enter at error: data type not matched (expected array type).");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }

    yyjson_val* _pVal = yyjson_arr_get(pCur, NumericCast<USizeT>(_idx));
    if (!_pVal)
    {
        JAM_LOG.Error("YYJson enter at error: index out of bounds.");
        MarkError(Error::OutOfRange);
        return Error::OutOfRange;
    }

    m_valueStack.PushBack(_pVal);
    return Error::None;
}

void JsonReader::Exit()
{
    m_valueStack.PopBack();
}

SizeT JsonReader::GetSize() const
{
    yyjson_val* pCur = m_valueStack.Back();
    if (yyjson_is_arr(pCur))
    {
        return static_cast<SizeT>(yyjson_arr_size(pCur));
    }
    else if (yyjson_is_obj(pCur))
    {
        return static_cast<SizeT>(yyjson_obj_size(pCur));
    }
    return 0;
}

bool JsonReader::IsNull() const
{
    yyjson_val* pCur = m_valueStack.Back();
    return yyjson_is_null(pCur);
}

bool JsonReader::IsFine() const
{
    return m_lastError.IsOk();
}

void JsonReader::MarkError(
    const Error _err)
{
    m_lastError = _err;
}

Error JsonReader::GetLastError() const
{
    return m_lastError;
}

void JsonReader::ClearLastError()
{
    m_lastError = Error::None;
}

bool JsonReader::HasError() const
{
    return m_lastError.IsError();
}

Error JsonReader::ReadPrimitive_(std::nullptr_t&)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_null(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected null type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    return Error::None;
}

Error JsonReader::ReadPrimitive_(bool& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_bool(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected bool type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = yyjson_get_bool(pCur);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(
    Int32& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_int(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected int type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }

    _value = NumericCast<Int32>(yyjson_get_sint(pCur));
    return Error::None;
}

Error JsonReader::ReadPrimitive_(UInt32& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_uint(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected uint type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = NumericCast<UInt32>(yyjson_get_uint(pCur));
    return Error::None;
}

Error JsonReader::ReadPrimitive_(Int64& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_int(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected int64 type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = yyjson_get_sint(pCur);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(UInt64& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_uint(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected uint64 type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = yyjson_get_uint(pCur);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(float& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_num(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected number type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = NumericCast<float>(yyjson_get_real(pCur));
    return Error::None;
}

Error JsonReader::ReadPrimitive_(double& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_num(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected number type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = yyjson_get_real(pCur);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(const char*& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_str(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected string type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    _value = yyjson_get_str(pCur);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(
    std::string& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_str(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected string type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    size_t      len = yyjson_get_len(pCur);
    const char* str = yyjson_get_str(pCur);
    _value.assign(str, len);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(
    std::string_view& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_str(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected string type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    size_t      len = yyjson_get_len(pCur);
    const char* str = yyjson_get_str(pCur);
    _value          = std::string_view(str, len);
    return Error::None;
}

Error JsonReader::ReadPrimitive_(String& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_str(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected string type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    const char* str = yyjson_get_str(pCur);
    size_t      len = yyjson_get_len(pCur);
    _value.Append(str, NumericCast<SizeT>(len));
    return Error::None;
}

Error JsonReader::ReadPrimitive_(
    StringView& _value)
{
    yyjson_val* pCur = m_valueStack.Back();
    if (!yyjson_is_str(pCur))
    {
        JAM_LOG.Error("YYJson read error: expected string type.");
        MarkError(Error::TypeMismatch);
        return Error::TypeMismatch;
    }
    size_t      len = yyjson_get_len(pCur);
    const char* str = yyjson_get_str(pCur);
    _value          = StringView(str, static_cast<SizeT>(len));
    return Error::None;
}

}   // namespace Jam