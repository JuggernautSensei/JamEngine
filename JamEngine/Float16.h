#pragma once

namespace Jam
{

enum class Float16 : UInt16
{
};

[[nodiscard]] Float16 PackFloat32to16(float _value);
[[nodiscard]] float   UnpackFloat16To32(Float16 _value);

}   // namespace Jam