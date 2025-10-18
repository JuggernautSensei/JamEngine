#pragma once

// clang-format off
namespace jam { class MemoryView; }
// clang-format on

namespace jam
{

Result<std::string> ReadFromFile(const fs::path& _path);
eErrorCode          WriteToFile(const fs::path& _path, std::string_view _data);

Result<std::vector<Byte>> ReadBinaryFromFile(const fs::path& _path);
eErrorCode                WriteBinaryToFile(const fs::path& _path, const MemoryView& _data);

Result<Json> ReadJsonFromFile(const fs::path& _path);
eErrorCode   WriteJsonToFile(const fs::path& _path, const Json& _json, Int32 _dump = 4);

}   // namespace jam