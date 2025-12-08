#pragma once

#define GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
#include "vendor/ghc/filesystem.hpp"

namespace jam
{

namespace fs = ghc::filesystem;

[[nodiscard]] bool IsAncestorPathOf(const fs::path& _ancester, const fs::path& _descendant);

}

template<>
struct std::hash<jam::fs::path>
{
    std::size_t operator()(const jam::fs::path& p) const noexcept
    {
        return std::hash<std::string>()(p.native());
    }
};
