#pragma once

namespace jam
{

// basic formats
using Int8  = std::int8_t;
using Int16 = std::int16_t;
using Int32 = std::int32_t;
using Int64 = std::int64_t;

using UInt8  = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;

using Byte  = UInt8;
using Index = std::ptrdiff_t;

// json
using Json = nlohmann::json;

// math formats
using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Mat4 = DirectX::SimpleMath::Matrix;
using Quat = DirectX::SimpleMath::Quaternion;

using AABB    = DirectX::BoundingBox;
using Sphere  = DirectX::BoundingSphere;
using Frustum = DirectX::BoundingFrustum;
using OBB     = DirectX::BoundingOrientedBox;
using Plane   = DirectX::SimpleMath::Plane;
using Ray     = DirectX::SimpleMath::Ray;

// container
template<typename KeyT,
         typename ValueT,
         typename Hash            = ankerl::unordered_dense::hash<KeyT>,
         typename KeyEqual        = std::equal_to<KeyT>,
         typename Alloc           = std::allocator<std::pair<KeyT, ValueT>>,
         typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
         typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
using HashMap = ankerl::unordered_dense::map<KeyT, ValueT, Hash, KeyEqual, Alloc, Bucket, BucketContainer>;

template<typename KeyT,
         typename Hash            = ankerl::unordered_dense::hash<KeyT>,
         typename KeyEqual        = std::equal_to<KeyT>,
         typename Alloc           = std::allocator<KeyT>,
         typename Bucket          = ankerl::unordered_dense::bucket_type::standard,
         typename BucketContainer = ankerl::unordered_dense::detail::default_container_t>
using HashSet = ankerl::unordered_dense::set<KeyT, Hash, KeyEqual, Alloc, Bucket, BucketContainer>;

// ComPtr
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

// namespace
namespace fs = ghc::filesystem;
using namespace std::chrono_literals;

}   // namespace jam