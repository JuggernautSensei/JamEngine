#pragma once

#define GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
#include <ghc/filesystem.hpp>

namespace Jam
{

namespace fs                           = ghc::filesystem;
using Path                             = fs::path;
constexpr char k_preferedPathSeparator = Path::preferred_separator;

[[nodiscard]] bool       IsBasePathOf(const Path& _base, const Path& _path);            // if _base == _path -> true
[[nodiscard]] StringView ParentPathViewOf(const Path& _path);                           // '/abv/gd/555/333/pod.mac' -> '/abv/gd/555/333'
[[nodiscard]] StringView BaseNameViewOf(const Path& _path);                             // '/abv/gd/555/333/pod.mac' -> 'pod'
[[nodiscard]] StringView FileNameViewOf(const Path& _path);                             // '/abv/gd/555/333/pod.mac' -> pod.mac
[[nodiscard]] StringView ExtensionViewOf(const Path& _path);                            // '/abv/gd/555/333/pod.mac' -> .mac
[[nodiscard]] String     ParentPathOf(const Path& _path);                               // '/abv/gd/555/333/pod.mac' -> '/abv/gd/555/333'
[[nodiscard]] String     BaseNameOf(const Path& _path);                                 // '/abv/gd/555/333/pod.mac' -> 'pod'
[[nodiscard]] String     FileNameOf(const Path& _path);                                 // '/abv/gd/555/333/pod.mac' -> 'pod.mac'
[[nodiscard]] String     ExtensionOf(const Path& _path);                                // '/abv/gd/555/333/pod.mac' -> '.mac'
[[nodiscard]] Path       PathFrom(StringView _str);                                     // create Path from StringView
[[nodiscard]] bool       MatchesExtension(const Path& _path, StringView _extension);    // ignore case, _extension can be with or without '.'
void                     ReplaceExtension(Path& _path, StringView _extension);          // replace extension in-place
[[nodiscard]] Path       ReplacedExtension(const Path& _path, StringView _extension);   // return new path with replaced extension
void                     JoinPath(Path& _base, StringView _path);                       // join _base and _path in-place -> _base/_path
[[nodiscard]] Path       JoinedPath(const Path& _base, StringView _path);               // join _base and _path -> _base/_path

// serialization
class ArchiveWriter;
class ArchiveReader;

void  WriteToArchive(ArchiveWriter& _writer, const Path& _path);
Error ReadFromArchive(ArchiveReader& _reader, Path& _path);

}   // namespace Jam

template<>
struct std::hash<Jam::Path>
{
    std::size_t operator()(const Jam::Path& p) const noexcept
    {
        return ankerl::unordered_dense::hash<std::string> {}(p.native());
    }
};

template<>
struct fmt::formatter<Jam::Path> : fmt::formatter<std::string>
{
    template<typename FormatContext>
    auto format(const Jam::Path& p, FormatContext& ctx) const
    {
        return fmt::formatter<std::string>::format(p.native(), ctx);
    }
};