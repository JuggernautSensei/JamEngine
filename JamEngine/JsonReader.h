#pragma once

namespace jam
{

enum eJsonReaderFlags_
{
    eJsonReaderFlags_None,
    eJsonReaderFlags_LogOnError  = 1 << 0,
    eJsonReaderFlags_StopOnError = 1 << 1,
};
using eJsonReaderFlags = EnumIntType<eJsonReaderFlags_>;

class JsonReader
{
public:
    explicit JsonReader(const Json& _json, const eJsonReaderFlags _flags = eJsonReaderFlags_LogOnError)
        : m_pJson(&_json)
        , m_flags(_flags)
    {
        m_pRootSucceeded = &m_bSucceeded;
    }

    ////// VALUE //////

    template<typename TValue>
    bool Read(TValue& _outValue)
    {
        JAM_ASSERT(m_pJson != nullptr, "JsonReader::Read() called at invalid state");
        return Read_(*m_pJson, _outValue);
    }

    ////// OBJECT //////

    template<typename TValue>
    bool ReadField(const StringView _field, TValue& _outValue)
    {
        JAM_ASSERT(m_pJson != nullptr, "JsonReader::ReadField() called at invalid state");
        if (NeedSkip_())
        {
            return false;
        }

        const auto it = m_pJson->find(_field.GetPtr());
        if (it != m_pJson->end())
        {
            return Read_(it.value(), _outValue);
        }
        else
        {
            MarkFailed();
            GlobalBuffer::SetLastError("JsonReader::ReadField('{}') failed: field not found", _field);
            LogLastErrorIfNeed_();
            return false;
        }
    }

    [[nodiscard]] Optional<JsonReader> EnterField(const StringView _field)
    {
        JAM_ASSERT(m_pJson != nullptr, "JsonReader::EnterField() called at invalid state");
        if (NeedSkip_())
        {
            return k_nullopt;
        }

        const auto it = m_pJson->find(_field.GetPtr());
        if (it != m_pJson->end())
        {
            JsonReader reader { it.value(), m_flags };
            reader.m_pRootSucceeded = m_pRootSucceeded;
            return reader;
        }
        else
        {
            MarkFailed();
            GlobalBuffer::SetLastError("JsonReader::EnterField('{}') failed: field not found", _field);
            LogLastErrorIfNeed_();
            return k_nullopt;
        }
    }

    ////// ARRAY //////

    template<typename TValue>
    bool ReadAt(const Int32 _idx, TValue& _outValue)
    {
        JAM_ASSERT(m_pJson != nullptr, "JsonReader::ReadAt() called at invalid state");
        if (NeedSkip_())
        {
            return false;
        }

        try
        {
            const Json& json = m_pJson->at(_idx);
            return Read_(json, _outValue);
        }
        catch (const std::exception& e)
        {
            MarkFailed();
            GlobalBuffer::SetLastError("JsonReader::ReadAt({}) failed: {}", _idx, e.what());
            LogLastErrorIfNeed_();
            return false;
        }
    }

    [[nodiscard]] Optional<JsonReader> EnterAt(const Int32 _idx)
    {
        JAM_ASSERT(m_pJson != nullptr, "JsonReader::EnterAt() called at invalid state");
        if (NeedSkip_())
        {
            return k_nullopt;
        }

        try
        {
            const Json& json = m_pJson->at(_idx);
            JsonReader  reader { json, m_flags };
            reader.m_pRootSucceeded = m_pRootSucceeded;
            return reader;
        }
        catch (const std::exception& e)
        {
            MarkFailed();
            GlobalBuffer::SetLastError("JsonReader::EnterAt({}) failed: {}", _idx, e.what());
            LogLastErrorIfNeed_();
            return k_nullopt;
        }
    }

    ////// GLOBAL //////

    void MarkFailed()
    {
        m_bSucceeded      = false;
        *m_pRootSucceeded = false;
    }

    [[nodiscard]] bool IsNull() const
    {
        return m_pJson->is_null();
    }

    [[nodiscard]] Int32 GetSize() const
    {
        return NarrowCast<Int32>(m_pJson->size());
    }

    [[nodiscard]] bool IsSucceeded() const
    {
        return m_bSucceeded;
    }

    [[nodiscard]] const Json& GetJson() const
    {
        return *m_pJson;
    }

private:
    template<typename TValue>
    bool Read_(const Json& _json, TValue& _outValue)
    {
        if (NeedSkip_())
        {
            return false;
        }

        try
        {
            _outValue = _json.get<TValue>();
            return true;
        }
        catch (const std::exception& e)
        {
            MarkFailed();
            GlobalBuffer::SetLastError("JsonReader::As {} failed: {}", NameOf<TValue>(), e.what());
            LogLastErrorIfNeed_();
            return false;
        }
    }

    void LogLastErrorIfNeed_() const
    {
        if ((m_flags & eJsonReaderFlags_LogOnError) != 0)
        {
            Log::Error("{}", GlobalBuffer::GetLastError());
        }
    }

    [[nodiscard]] bool NeedSkip_() const
    {
        return (m_flags & eJsonReaderFlags_StopOnError) != 0 && !IsSucceeded();
    }

    const Json*      m_pJson = nullptr;
    eJsonReaderFlags m_flags = eJsonReaderFlags_None;

    bool  m_bSucceeded     = true;
    bool* m_pRootSucceeded = nullptr;
};

}   // namespace jam