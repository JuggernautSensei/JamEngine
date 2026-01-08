#include "pch.h"

#include "FileIOUtilities.h"
#include "MemView.h"

#include <fstream>
#include <system_error>

namespace jam
{

Optional<Vector<Byte>> LoadBinaryFile(
    const fs::path& _path)
{
    std::ifstream ifs { _path, std::ios::binary };
    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to open file: {}", _path.native());
        return k_nullopt;
    }

    std::error_code ec       = {};
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

    Vector<Byte> result(fileSize);
    ifs.read(reinterpret_cast<char*>(result.data()), static_cast<std::streamsize>(fileSize));

    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to read file: {}", _path.native());
        return k_nullopt;
    }

    return result;
}

bool SaveBinaryFile(
    const fs::path&      _path,
    const CMemView       _data,
    const eSaveFileFlags _flags)
{
    if (!CheckSaveFileFlags(_path, _flags))
    {
        return false;
    }

    // io
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

bool CheckSaveFileFlags(
    const fs::path&      _path,
    const eSaveFileFlags _flags)
{
    // check existence
    if (_flags & eSaveFileFlags_FailIfExists)
    {
        if (fs::exists(_path))
        {
            GlobalBuffer::SetLastError("file already exists: {}", _path.native());
            return false;
        }
    }

    // create directories
    if (_flags & eSaveFileFlags_CreateDirs)
    {
        const fs::path  parentPath = _path.parent_path();
        std::error_code ec         = {};
        fs::create_directories(parentPath, ec);
        if (ec)
        {
            GlobalBuffer::SetLastError("failed to create directories: {}. {}", parentPath.native(), ec.message());
            return false;
        }
    }

    return true;
}

Optional<std::string> LoadTextFile(
    const fs::path& _path)
{
    std::ifstream ifs(_path);
    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to open file: {}", _path.native());
        return k_nullopt;
    }

    std::error_code ec       = {};
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

    std::string buffer(fileSize, '\0');
    ifs.read(buffer.data(), static_cast<std::streamsize>(fileSize));

    if (!ifs)
    {
        GlobalBuffer::SetLastError("failed to read file: {}", _path.native());
        return k_nullopt;
    }

    return buffer;
}

bool SaveTextFile(
    const fs::path&      _path,
    const StringView     _text,
    const eSaveFileFlags _flags)
{
    return SaveBinaryFile(_path, { reinterpret_cast<const Byte*>(_text.GetPtr()), _text.GetLength() }, _flags);
}

}   // namespace jam