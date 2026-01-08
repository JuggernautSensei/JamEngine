#pragma once

#define GHC_WIN_DISABLE_WSTRING_STORAGE_TYPE
#include "vendor/ghc/filesystem.hpp"

namespace jam
{

namespace fs = ghc::filesystem;

// if _base == _path -> true
[[nodiscard]] bool IsBasePathOf(const fs::path& _base, const fs::path& _path);

}   // namespace jam

template<>
struct std::hash<jam::fs::path>
{
    std::size_t operator()(const jam::fs::path& p) const noexcept
    {
        return std::hash<std::string>()(p.native());
    }
};