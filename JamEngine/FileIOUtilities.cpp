#include "pch.h"

#include "FileIOUtilities.h"
#include "MemView.h"

#include <fstream>
#include <system_error>

namespace jam
{

Optional<Vector<Byte>> LoadBinaryFile(const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to open file: {}", _path.native());
        return k_nullopt;
    }

    std::error_code ec;
    const size_t    fileSize = fs::file_size(_path, ec);
    if (ec)
    {
        GlobalBuffer::SetLastError("failed to query file size: {}. {}", _path.native(), ec.message());
        return k_nullopt;
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
        GlobalBuffer::SetLastError("failed to read file: {}", _path.native());
        return k_nullopt;
    }

    return result;
}

bool SaveBinaryFile(const fs::path& _path, const CMemView _data)
{
    std::ofstream ofs { _path, std::ios::binary };
    if (!ofs)
    {
        GlobalBuffer::SetLastError("failed to open file for writing: {}", _path.native());
        return false;
    }

    if (_data.GetByteWidth() == 0)
    {
        return true;
    }

    ofs.write(reinterpret_cast<const char*>(_data.GetPtr()), static_cast<std::streamsize>(_data.GetByteWidth()));

    if (!ofs)
    {
        GlobalBuffer::SetLastError("failed to write data to file: {}", _path.native());
        return false;
    }

    return true;
}

Optional<std::string> LoadTextFile(const fs::path& _path)
{
    std::ifstream ifs(_path);
    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to open file: {}", _path.native());
        return k_nullopt;
    }

    std::error_code ec;
    const size_t    fileSize = fs::file_size(_path, ec);
    if (ec)
    {
        GlobalBuffer::SetLastError("failed to query file size: {}. {}", _path.native(), ec.message());
        return k_nullopt;
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
        GlobalBuffer::SetLastError("failed to read file: {}", _path.native());
        return k_nullopt;
    }

    return buffer;
}

bool SaveTextFile(const fs::path& _path, const StringView _text)
{
    return SaveBinaryFile(_path, { reinterpret_cast<const Byte*>(_text.GetPtr()), _text.GetLength() });
}

}   // namespace jam