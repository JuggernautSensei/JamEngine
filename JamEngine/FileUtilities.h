#pragma once

namespace jam
{

class ConstByteView;

std::optional<std::string> LoadFromFile(const fs::path& _path);
bool                       SaveToFile(const fs::path& _path, std::string_view _str);

std::optional<std::vector<Byte>> LoadBinaryFromFile(const fs::path& _path);
bool                             SaveBinaryToFile(const fs::path& _path, ConstMemView _mem);

std::optional<Json> LoadJsonFromFile(const fs::path& _path);
bool                SaveJsonToFile(const fs::path& _path, const Json& _json, Int32 _dump = 4);

}   // namespace jam