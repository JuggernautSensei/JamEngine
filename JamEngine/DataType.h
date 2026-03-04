#pragma once

namespace Jam
{

using Int8     = std::int8_t;
using Int16    = std::int16_t;
using Int32    = std::int32_t;
using Int64    = std::int64_t;
using UInt8    = std::uint8_t;
using UInt16   = std::uint16_t;
using UInt32   = std::uint32_t;
using UInt64   = std::uint64_t;
using Byte     = UInt8;
using SizeT    = std::ptrdiff_t;   // now my application use Int32 as size type. future: change to Int32
using USizeT   = size_t;

using Vec2    = DirectX::SimpleMath::Vector2;
using Vec3    = DirectX::SimpleMath::Vector3;   // euler: pitch, yaw, roll
using Vec4    = DirectX::SimpleMath::Vector4;
using Mtx4    = DirectX::SimpleMath::Matrix;
using Quat    = DirectX::SimpleMath::Quaternion;
using Float16 = DirectX::PackedVector::HALF;

using Registry       = entt::registry;
using RegistryHandle = entt::entity;

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

using namespace std::chrono_literals;

}   // namespace Jam