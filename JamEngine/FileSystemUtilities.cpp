
#include "pch.h"

#include "FileSystemUtilities.h"

namespace jam
{

bool IsBasePathOf(
    const fs::path& _base,
    const fs::path& _path)
{
    fs::path base = fs::absolute(_base);
    base          = base.lexically_normal();

    fs::path path = fs::absolute(_path);
    path          = path.lexically_normal();

    const fs::path   rel    = path.lexically_relative(base);
    const StringView strRel = rel.native();
    return !strRel.StartWith("..");
}

}   // namespace jam