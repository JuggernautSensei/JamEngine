#include "pch.h"

#include "Json.h"

#include "AssetManager.h"
#include "Camera.h"
#include "FileUtilities.h"

namespace jam
{

std::optional<Json> LoadJsonFile(const fs::path& _path)
{
    if (std::optional<Vector<Byte>> opt = LoadBinaryFile(_path))
    {
        try
        {
            return Json::parse(*opt);
        }
        catch (const Json::parse_error& e)
        {
            ErrorBuffer::Set("Failed to parse JSON from file: {}. {}", _path.native(), e.what());
            return std::nullopt;
        }
    }
    else
    {
        return std::nullopt;
    }
}

bool SaveJsonFile(const fs::path& _path, const Json& _json, const Int32 _dump)
{
    if (_dump <= 0)
    {
        return SaveBinaryFile(_path, _json.dump());
    }
    else
    {
        return SaveBinaryFile(_path, _json.dump(_dump));
    }
}

std::optional<Json> LoadBsonFile(const fs::path& _path)
{
    if (std::optional<Vector<Byte>> opt = LoadBinaryFile(_path))
    {
        try
        {
            return Json::from_bson(*opt);
        }
        catch (const Json::parse_error& e)
        {
            ErrorBuffer::Set("Failed to parse BSON from file: {}. {}", _path.native(), e.what());
            return std::nullopt;
        }
    }
    else
    {
        return std::nullopt;
    }
}

bool SaveBsonFile(const fs::path& _path, const Json& _j)
{
    Vector<Byte> bson = Json::to_bson(_j);
    return SaveBinaryFile(_path, bson);
}

}   // namespace jam
