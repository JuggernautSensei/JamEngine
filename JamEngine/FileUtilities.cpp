#include "pch.h"

#include "FileUtilities.h"
#include "Memory.h"

#include <fstream>
#include <system_error>

namespace jam
{

Result<std::string> ReadFromFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs.is_open())
    {
        SetErrorMessage("Failed to open file: {}", _path.native());
        return eErrorCode::FileOpenFailed;
    }

    std::error_code ec;
    size_t          filesize = fs::file_size(_path, ec);
    if (ec)
    {
        SetErrorMessage("Failed to query file size: {}", _path.native());
        return eErrorCode::FileReadFailed;
    }

    std::string result;
    result.resize(filesize);
    if (filesize > 0)
    {
        ifs.read(result.data(), filesize);
        if (!ifs)
        {
            SetErrorMessage("Failed to read file: {}", _path.native());
            return eErrorCode::FileReadFailed;
        }
    }

    return result;
}

eErrorCode WriteToFile(const fs::path& _path, const std::string_view _data)
{
    std::ofstream ofs { _path, std::ios::binary | std::ios::out | std::ios::trunc };
    if (!ofs.is_open())
    {
        SetErrorMessage("Failed to open file for writing: {}", _path.native());
        return eErrorCode::FileOpenFailed;
    }

    if (!_data.empty())
    {
        ofs.write(_data.data(), static_cast<std::streamsize>(_data.size()));
    }

    if (!ofs)
    {
        SetErrorMessage("Failed to write file: {}", _path.native());
        return eErrorCode::FileWriteFailed;
    }

    return eErrorCode::Ok;
}

Result<std::vector<Byte>> ReadBinaryFromFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs.is_open())
    {
        SetErrorMessage("Failed to open file: {}", _path.native());
        return eErrorCode::FileOpenFailed;
    }

    std::error_code ec;
    const size_t    filesize = fs::file_size(_path, ec);
    if (ec)
    {
        SetErrorMessage("Failed to query file size: {}", _path.native());
        return eErrorCode::FileReadFailed;
    }

    std::vector<Byte> bytes;
    bytes.resize(filesize);
    if (filesize > 0)
    {
        ifs.read(reinterpret_cast<char*>(bytes.data()), filesize);
        if (!ifs)
        {
            SetErrorMessage("Failed to read file: {}", _path.native());
            return eErrorCode::FileReadFailed;
        }
    }

    return bytes;
}

eErrorCode WriteBinaryToFile(const fs::path& _path, const MemoryView& _data)
{
    std::ofstream ofs { _path, std::ios::binary | std::ios::out | std::ios::trunc };
    if (!ofs.is_open())
    {
        SetErrorMessage("Failed to open file for writing: {}", _path.native());
        return eErrorCode::FileOpenFailed;
    }

    const Byte*  pData = _data.GetDataPtr();
    const size_t size  = _data.GetByteWidth();

    if (size > 0)
    {
        ofs.write(reinterpret_cast<const char*>(pData), size);
    }

    if (!ofs)
    {
        SetErrorMessage("Failed to write file: {}", _path.native());
        return eErrorCode::FileWriteFailed;
    }

    return eErrorCode::Ok;
}

Result<Json> ReadJsonFromFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs.is_open())
    {
        SetErrorMessage("Failed to open file: {}", _path.native());
        return eErrorCode::FileOpenFailed;
    }

    Json json;
    try
    {
        json = Json::parse(ifs);
    }
    catch (const Json::parse_error& e)
    {
        SetErrorMessage("Failed to parse JSON file: {}. {}", _path.native(), e.what());
        return eErrorCode::FileParseFailed;
    }
    return json;
}

eErrorCode WriteJsonToFile(const fs::path& _path, const Json& _json, const Int32 _dump)
{
    std::ofstream ofs { _path, std::ios::binary | std::ios::out | std::ios::trunc };
    if (!ofs.is_open())
    {
        SetErrorMessage("Failed to open file for writing: {}", _path.native());
        return eErrorCode::FileOpenFailed;
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
        SetErrorMessage("Failed to write JSON file: {}. {}", _path.native(), e.what());
        return eErrorCode::FileWriteFailed;
    }

    if (!ofs)
    {
        SetErrorMessage("Failed to write file: {}", _path.native());
        return eErrorCode::FileWriteFailed;
    }

    return eErrorCode::Ok;
}

}   // namespace jam
