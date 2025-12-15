#include "pch.h"

#include "FileUtilities.h"
#include "MemView.h"

#include <fstream>
#include <system_error>

namespace jam
{

std::optional<Vector<Byte>> LoadBinaryFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        ErrorBuffer::Set("failed to open file: {}", _path.native());
        return std::nullopt;
    }

    std::error_code ec;
    const size_t    fileSize = fs::file_size(_path, ec);
    if (ec)
    {
        ErrorBuffer::Set("failed to query file size: {}. {}", _path.native(), ec.message());
        return std::nullopt;
    }

    if (fileSize == 0)
    {
        return Vector<Byte>();
    }

    Vector<Byte> result;
    result.resize(fileSize);
    ifs.read(reinterpret_cast<char*>(result.data()), static_cast<std::streamsize>(fileSize));

    if (!ifs)
    {
        ErrorBuffer::Set("failed to read file: {}", _path.native());
        return std::nullopt;
    }

    return result;
}

bool SaveBinaryFile(const fs::path& _path, const ConstMemView _data)
{
    std::ofstream ofs { _path, std::ios::binary };
    if (!ofs)
    {
        ErrorBuffer::Set("failed to open file for writing: {}", _path.native());
        return false;
    }

    if (_data.GetByteWidth() == 0)
    {
        return true;
    }

    ofs.write(reinterpret_cast<const char*>(_data.GetPointer()), static_cast<std::streamsize>(_data.GetByteWidth()));

    if (!ofs)
    {
        ErrorBuffer::Set("failed to write data to file: {}", _path.native());
        return false;
    }

    return true;
}

std::optional<std::string> LoadTextFile(const fs::path& _path)
{
    std::ifstream ifs(_path);
    if (!ifs)
    {
        ErrorBuffer::Set("failed to open file: {}", _path.native());
        return std::nullopt;
    }

    std::error_code ec;
    const size_t    fileSize = fs::file_size(_path, ec);
    if (ec)
    {
        ErrorBuffer::Set("failed to query file size: {}. {}", _path.native(), ec.message());
        return std::nullopt;
    }

    if (fileSize == 0)
    {
        return std::string();
    }

    std::string buffer;
    buffer.resize(fileSize);
    ifs.read(buffer.data(), static_cast<std::streamsize>(fileSize));

    if (!ifs)
    {
        ErrorBuffer::Set("failed to read file: {}", _path.native());
        return std::nullopt;
    }

    return buffer;
}

bool SaveTextFile(const fs::path& _path, const std::string_view _text)
{
    return SaveBinaryFile(_path, { reinterpret_cast<const Byte*>(_text.data()), _text.size() });
}

}   // namespace jam