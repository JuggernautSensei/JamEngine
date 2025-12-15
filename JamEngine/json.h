#pragma once

#define JSON_THROW_USER throw std::exception
#include "Mesh.h"
#include "Vendor/nlohmann/Json.hpp"
#include "Vendor/nlohmann/json_fwd.hpp"

namespace jam
{

namespace json = nlohmann;
using Json     = json::json;

[[nodiscard]] std::optional<Json> LoadJsonFile(const fs::path& _path);
bool                              SaveJsonFile(const fs::path& _path, const Json& _json, Int32 _dump = 4);

[[nodiscard]] std::optional<Json> LoadBsonFile(const fs::path& _path);
bool                              SaveBsonFile(const fs::path& _path, const Json& _j);

template<typename T>
bool TryParseJson(const Json& _json, T& _outValue, const bool _logOnError = true)
{
    try
    {
        _outValue = _json.get<T>();
        return true;
    }
    catch (const std::exception& e)
    {
        ErrorBuffer::Set("failed to parse json to {}. {}", NameOf<T>(), e.what());

        if (_logOnError)
        {
            Log::Warn(ErrorBuffer::Get());
        }

        return false;
    }
}

template<typename TKey, typename TValue>
bool TryParseJsonField(const Json& _json, TKey&& _field, TValue& _outValue, const bool _logOnError = true)
{
    if (const auto it = _json.find(_field); it != _json.end())
    {
        return TryParseJson(*it, _outValue);
    }
    else
    {
        ErrorBuffer::Set("json field '{}' not found", _field);

        if (_logOnError)
        {
            Log::Warn(ErrorBuffer::Get());
        }

        return false;
    }
}

// do not use on bussiness logic
template<typename TKey, typename TValue>
bool TryParseJsonField_ThrowOnParseError(const Json& _json, TKey&& _field, TValue& _outValue, const bool _logOnError = true)
{
    if (const auto it = _json.find(_field); it != _json.end())
    {
        _outValue = *it.template get<TValue>();   // or throw
        return true;
    }
    else
    {
        ErrorBuffer::Set("json field '{}' not found", _field);

        if (_logOnError)
        {
            Log::Warn(ErrorBuffer::Get());
        }

        return false;
    }
}

template<typename TKey>
[[nodiscard]] std::optional<Json> FindJsonField(const Json& _json, TKey&& _field, const bool _bLogError = true)
{
    if (const auto it = _json.find(_field); it != _json.end())
    {
        return *it;
    }
    else
    {
        ErrorBuffer::Set("json field '{}' not found", _field);

        if (_bLogError)
        {
            Log::Warn(ErrorBuffer::Get());
        }

        return std::nullopt;
    }
}

}   // namespace jam
