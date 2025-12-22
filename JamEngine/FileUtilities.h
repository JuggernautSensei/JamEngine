#pragma once

namespace jam
{

[[nodiscard]] Optional<Vector<Byte>> LoadBinaryFile(const fs::path& _path);
bool                                 SaveBinaryFile(const fs::path& _path, CMemView _data);

[[nodiscard]] Optional<std::string> LoadTextFile(const fs::path& _path);
bool                                SaveTextFile(const fs::path& _path, StringView _text);

}   // namespace jam