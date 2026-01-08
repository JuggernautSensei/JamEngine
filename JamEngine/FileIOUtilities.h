#pragma once

namespace jam
{

enum eSaveFileFlags_
{
    eSaveFileFlags_None                = 0,
    eSaveFileFlags_FailIfExists        = 1 << 0,
    eSaveFileFlags_CreateDirs          = 1 << 2,
};
using eSaveFileFlags = UnderlyingType<eSaveFileFlags_>;

[[nodiscard]] Optional<Vector<Byte>> LoadBinaryFile(const fs::path& _path);
bool                                 SaveBinaryFile(const fs::path& _path, CMemView _data, eSaveFileFlags _flags = eSaveFileFlags_None);
[[nodiscard]] bool                   CheckSaveFileFlags(const fs::path& _path, eSaveFileFlags _flags);

[[nodiscard]] Optional<std::string> LoadTextFile(const fs::path& _path);
bool                                SaveTextFile(const fs::path& _path, StringView _text, eSaveFileFlags _flags = eSaveFileFlags_None);

}   // namespace jam