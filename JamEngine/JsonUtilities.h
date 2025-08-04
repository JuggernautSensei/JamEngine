#pragma once

namespace jam
{

NODISCARD std::optional<Json> LoadJsonFromFile(const std::filesystem::path& filePath);
bool                          SaveJsonToFile(const Json& json, const std::filesystem::path& filePath);

}   // namespace jam