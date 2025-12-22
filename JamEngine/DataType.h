#pragma once

namespace jam
{

using Int8   = std::int8_t;
using Int16  = std::int16_t;
using Int32  = std::int32_t;
using Int64  = std::int64_t;
using UInt8  = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;
using Byte   = std::byte;

using Vec2    = DirectX::SimpleMath::Vector2;
using Vec3    = DirectX::SimpleMath::Vector3;
using Vec4    = DirectX::SimpleMath::Vector4;
using Mat4    = DirectX::SimpleMath::Matrix;
using Quat    = DirectX::SimpleMath::Quaternion;
using Float16 = DirectX::PackedVector::HALF;

using namespace std::chrono_literals;

}   // namespace jam