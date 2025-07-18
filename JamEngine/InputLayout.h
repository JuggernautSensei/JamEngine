#pragma once

namespace jam
{

enum class eDataType
{
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UInt,
    UInt2,
    UInt3,
    UInt4,
    Bool,
};

struct InputElement
{
    std::string_view name;
    UInt32           index;
    eDataType        format;
};

}   // namespace jam