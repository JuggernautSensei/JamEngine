#include "pch.h"

#include "FileSystemUtilities.h"

namespace jam
{

bool IsAncestorPathOf(const fs::path& _ancester, const fs::path& _descendant)
{
    JAM_ASSERT(_ancester.empty() == false, "Ancestor path is empty!");
    JAM_ASSERT(_descendant.empty() == false, "Path is empty!");

    const fs::path         ancestor = fs::absolute(_ancester).lexically_normal();
    const fs::path         target   = fs::absolute(_descendant).lexically_normal();
    const fs::path         rel      = target.lexically_relative(ancestor);
    const std::string_view strRel   = rel.native();
    return !strRel.empty() && strRel != "." && !strRel.starts_with("..");
}

}   // namespace jam