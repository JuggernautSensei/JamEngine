#include "pch.h"

#include "FileUtilities.h"
#include "MemView.h"

#include <fstream>
#include <system_error>

namespace jam
{

std::optional<std::string> LoadFromFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        SetErrorString("Failed to open file: {}", _path.native());
        return std::nullopt;
    }

    size_t fileSize;
    try
    {
        fileSize = fs::file_size(_path);
    }
    catch (const fs::filesystem_error& e)
    {
        SetErrorString("Failed to query file size: {}. {}", _path.native(), e.what());
        return std::nullopt;
    }

    std::string result;
    result.resize(fileSize);
    if (fileSize > 0)
    {
        ifs.read(result.data(), fileSize);
        if (!ifs)
        {
            SetErrorString("Failed to read file: {}", _path.native());
            return std::nullopt;
        }
    }

    return result;
}

bool SaveToFile(const fs::path& _path, const std::string_view _str)
{
    std::ofstream ofs { _path, std::ios::binary | std::ios::out | std::ios::trunc };
    if (!ofs)
    {
        SetErrorString("Failed to open file for writing: {}", _path.native());
        return false;
    }

    if (!_str.empty())
    {
        ofs.write(_str.data(), static_cast<std::streamsize>(_str.size()));
    }

    if (!ofs)
    {
        SetErrorString("Failed to write file: {}", _path.native());
        return false;
    }

    return true;
}

std::optional<std::vector<Byte>> LoadBinaryFromFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        SetErrorString("Failed to open file: {}", _path.native());
        return std::nullopt;
    }

    size_t fileSize;
    try
    {
        fileSize = fs::file_size(_path);
    }
    catch (const fs::filesystem_error& e)
    {
        SetErrorString("Failed to query file size: {}. {}", _path.native(), e.what());
        return std::nullopt;
    }

    std::vector<Byte> bytes;
    bytes.resize(fileSize);
    if (fileSize > 0)
    {
        ifs.read(reinterpret_cast<char*>(bytes.data()), fileSize);
        if (!ifs)
        {
            SetErrorString("Failed to read file: {}", _path.native());
            return std::nullopt;
        }
    }

    return bytes;
}

bool SaveBinaryToFile(const fs::path& _path, const ConstMemView _mem)
{
    std::ofstream ofs { _path, std::ios::binary | std::ios::out | std::ios::trunc };
    if (!ofs)
    {
        SetErrorString("Failed to open file for writing: {}", _path.native());
        return false;
    }

    const Byte*  pData = _mem.GetDataPtr();
    const size_t size  = _mem.GetByteWidth();

    if (size > 0)
    {
        ofs.write(reinterpret_cast<const char*>(pData), size);
    }

    if (!ofs)
    {
        SetErrorString("Failed to write file: {}", _path.native());
        return false;
    }

    return true;
}

std::optional<Json> LoadJsonFromFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        SetErrorString("Failed to open file: {}", _path.native());
        return std::nullopt;
    }

    Json json;
    try
    {
        json = Json::parse(ifs);
    }
    catch (const Json::parse_error& e)
    {
        SetErrorString("Failed to parse JSON file: {}. {}", _path.native(), e.what());
        return std::nullopt;
    }
    return json;
}

bool SaveJsonToFile(const fs::path& _path, const Json& _json, const Int32 _dump)
{
    std::ofstream ofs { _path, std::ios::binary | std::ios::out | std::ios::trunc };
    if (!ofs)
    {
        SetErrorString("Failed to open file for writing: {}", _path.native());
        return false;
    }

    try
    {
        if (_dump > 0)
        {
            ofs << _json.dump(_dump);
        }
        else
        {
            ofs << _json;
        }
    }
    catch (const std::exception& e)
    {
        SetErrorString("Failed to write JSON file: {}. {}", _path.native(), e.what());
        return false;
    }

    if (!ofs)
    {
        SetErrorString("Failed to write file: {}", _path.native());
        return false;
    }

    return true;
}

}   // namespace jam