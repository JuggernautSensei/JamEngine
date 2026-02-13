#include "pch.h"

void Jam::Details::AssertFail(
    const std::source_location& _loc,
    const StringView            _str)
{
    fmt::print(stderr, "Assertion failed at {} ({}:{}): {}\n", _loc.file_name(), _loc.function_name(), _loc.line(), _str.ToString());
    __debugbreak();
}