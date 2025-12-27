#include "pch.h"

#include "Json.h"

#include "AssetManager.h"
#include "FileIOUtilities.h"

namespace jam
{

Optional<Json> LoadJsonFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to open file: {}", _path.native());
        return k_nullopt;
    }

    try
    {
        return Json::parse(ifs);
    }
    catch (const std::exception& e)
    {
        GlobalBuffer::SetLastError("failed to parse json file: {}. {}", _path.native(), e.what());
        return k_nullopt;
    }
}

bool SaveJsonFile(const fs::path& _path, const Json& _json, const Int32 _dump)
{
    std::ofstream ofs { _path, std::ios::binary };
    if (!ofs)
    {
        GlobalBuffer::SetLastError("failed to open file for writing: {}", _path.native());
        return false;
    }

    try
    {
        if (_dump < 0)
        {
            ofs << _json;
        }
        else
        {
            ofs << _json.dump(_dump);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        GlobalBuffer::SetLastError("failed to write json file: {}. {}", _path.native(), e.what());
        return false;
    }
}

Optional<Json> LoadBsonFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to open file: {}", _path.native());
        return k_nullopt;
    }

    try
    {
        return Json::from_bson(ifs);
    }
    catch (const std::exception& e)
    {
        GlobalBuffer::SetLastError("failed to parse bson file: {}. {}", _path.native(), e.what());
        return k_nullopt;
    }
}

bool SaveBsonFile(const fs::path& _path, const Json& _j)
{
    std::vector<std::uint8_t> bytes = Json::to_bson(_j);
    return SaveBinaryFile(_path, bytes);
}

}   // namespace jam
