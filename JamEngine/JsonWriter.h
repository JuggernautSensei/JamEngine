#pragma once

namespace jam
{

// _json 쓰기 헬퍼
// 기존 nlohmann json은 try catch 떡칠이라 사용이 불편해서 래핑한 클래스
// write failed은 대부분 프로그래머 실책. assert로 처리함

class JsonWriter
{
public:
    explicit JsonWriter(Json& _json)
        : m_pJson(&_json)
    {
    }

    ////// VALUE //////

    template<typename TValue>
    JsonWriter Write(TValue&& _value)   // value
    {
        try
        {
            *m_pJson = std::forward<TValue>(_value);
            return JsonWriter { *m_pJson };
        }
        catch (const std::exception& e)
        {
            JAM_ASSERT(0, "JsonWriter::Write {} failed: {}", NameOf<TValue>(), e.what());
            return {};
        }
    }

    ////// OBJECT //////

    template<typename TValue>
    JsonWriter WriteField(const StringView _field, TValue&& _value)   // object
    {
        JAM_ASSERT(m_pJson->is_object() || m_pJson->is_null(), "JsonWriter::WriteField('{}', {}) failed: not an object", _field.ToString(), NameOf<TValue>());

        try
        {
            Json& json = (*m_pJson)[_field.GetPtr()];
            json       = std::forward<TValue>(_value);
            return JsonWriter { json };
        }
        catch (const std::exception& e)
        {
            JAM_ASSERT(0, "JsonWriter::WriteField('{}', {}) failed: {}", _field.ToString(), NameOf<TValue>(), e.what());
            return {};
        }
    }

    JsonWriter WriteField(const StringView _field)   // object
    {
        return WriteField(_field, nullptr);   // placeholder
    }

    [[nodiscard]] JsonWriter EnterField(const StringView _field)   // object
    {
        JAM_ASSERT(m_pJson->is_object() || m_pJson->is_null(), "JsonWriter::EnterField('{}') failed: not an object", _field.ToString());

        try
        {
            Json& json = (*m_pJson)[_field.GetPtr()];
            return JsonWriter { json };
        }
        catch (const std::exception& e)
        {
            JAM_ASSERT(0, "JsonWriter::EnterField('{}') failed: {}", _field.ToString(), e.what());
            return {};
        }
    }

    ////// ARRAY //////

    template<typename TValue>
    JsonWriter PushBack(TValue&& _value)
    {
        JAM_ASSERT(m_pJson->is_array() || m_pJson->is_null(), "JsonWriter::PushBack {} failed: not an array", NameOf<TValue>());

        try
        {
            Json& json = m_pJson->emplace_back(_value);
            return JsonWriter { json };
        }
        catch (const std::exception& e)
        {
            JAM_ASSERT(0, "JsonWriter::WriteAndPushBack {} failed: {}", NameOf<TValue>(), e.what());
            return {};
        }
    }

    JsonWriter PushBack()
    {
        return PushBack(nullptr);   // placeholder
    }

    template<typename TValue>
    JsonWriter WriteAt(const Int32 _idx, TValue&& _value)   // array
    {
        JAM_ASSERT(m_pJson->is_array(), "JsonWriter::WriteAt({}, {}) failed: not an array", _idx, NameOf<TValue>());

        try
        {
            Json& json = m_pJson->at(_idx);
            json       = std::forward<TValue>(_value);
            return JsonWriter { json };
        }
        catch (const std::exception& e)
        {
            JAM_ASSERT(0, "JsonWriter::WriteAt({}, {}) failed: {}", _idx, NameOf<TValue>(), e.what());
            return {};
        }
    }

    [[nodiscard]] JsonWriter EnterAt(const Int32 _idx) const   // array
    {
        JAM_ASSERT(m_pJson->is_array(), "JsonWriter::EnterAt({}) failed: not an array", _idx);

        try
        {
            Json& json = (*m_pJson)[_idx];
            return JsonWriter { json };
        }
        catch (const std::exception& e)
        {
            JAM_ASSERT(0, "JsonWriter::EnterAt({}) failed: {}", _idx, e.what());
            return {};
        }
    }

    JsonWriter EnterBack() const
    {
        JAM_ASSERT(m_pJson->is_array(), "JsonWriter::EnterBack() failed: not an array");
        JAM_ASSERT(!m_pJson->empty(), "JsonWriter::EnterBack() failed: array is empty");

        try
        {
            Json& json = m_pJson->back();
            return JsonWriter { json };
        }
        catch (const std::exception& e)
        {
            JAM_ASSERT(0, "JsonWriter::EnterBack() failed: {}", e.what());
            return {};
        }
    }

    ////// GLOBAL //////

    [[nodiscard]] Int32 GetSize() const
    {
        return NarrowCast<Int32>(m_pJson->size());
    }

private:
    JsonWriter() = default;

    Json* m_pJson = nullptr;
};

}   // namespace jam