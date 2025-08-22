#pragma once

namespace jam
{

NODISCARD Result<Json> LoadJsonFromFile(const std::filesystem::path& filePath);
bool                          SaveJsonToFile(const Json& json, const std::filesystem::path& filePath);

template<typename ValueT, typename KeyT>
ValueT GetJsonValueOrDefault(const Json& json, const KeyT& key, const ValueT& defaultValue)
{
    auto it = json.find(key);
    return (it != json.end()) ? it->template get<ValueT>() : defaultValue;
}

}   // namespace jam