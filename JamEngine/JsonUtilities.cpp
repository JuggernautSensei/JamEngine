#include "pch.h"

#include "JsonUtilities.h"

#include <fstream>

namespace jam
{

std::optional<Json> LoadJsonFromFile(const std::filesystem::path& filePath)
{
    std::ifstream file { filePath };
    if (!file.is_open())
    {
        JAM_ERROR("Failed to open JSON file: {}", filePath.string());
        return std::nullopt;
    }

    try
    {
        Json json = Json::parse(file, nullptr, false);
        return json;
    }
    catch (std::exception& e)
    {
        JAM_ERROR("Failed to parse JSON file: {}. Error: {}", filePath.string(), e.what());
        return std::nullopt;
    }
}

bool SaveJsonToFile(const Json& json, const std::filesystem::path& filePath)
{
    std::ofstream file { filePath };
    if (!file.is_open())
    {
        JAM_ERROR("Failed to open JSON file for writing: {}", filePath.string());
        return false;
    }

    try
    {
        file << json;
        return true;
    }
    catch (std::exception& e)
    {
        JAM_ERROR("Failed to write JSON to file: {}. Error: {}", filePath.string(), e.what());
        return false;
    }
}

}   // namespace jam