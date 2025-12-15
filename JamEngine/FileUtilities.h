#pragma once

namespace jam
{

[[nodiscard]] std::optional<Vector<Byte>> LoadBinaryFile(const fs::path& _path);
bool                                           SaveBinaryFile(const fs::path& _path, ConstMemView _data);

[[nodiscard]] std::optional<std::string> LoadTextFile(const fs::path& _path);
bool                                     SaveTextFile(const fs::path& _path, std::string_view _text);

}   // namespace jam