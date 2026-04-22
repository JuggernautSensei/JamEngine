#include "pch.h"
#include "Vector.h"

namespace Jam
{

const Vec2i Vec2i::kZero  = { 0, 0 };
const Vec2i Vec2i::kOne   = { 1, 1 };
const Vec2i Vec2i::kUnitX = { 1, 0 };
const Vec2i Vec2i::kUnitY = { 0, 1 };

const Vec2 Vec2::kZero  = { 0.f, 0.f };
const Vec2 Vec2::kOne   = { 1.f, 1.f };
const Vec2 Vec2::kUnitX = { 1.f, 0.f };
const Vec2 Vec2::kUnitY = { 0.f, 1.f };
const Vec2 Vec2::kRight = Vec2::kUnitX;
const Vec2 Vec2::kUp    = Vec2::kUnitY;

const Vec3 Vec3::kZero    = { 0.f, 0.f, 0.f };
const Vec3 Vec3::kOne     = { 1.f, 1.f, 1.f };
const Vec3 Vec3::kUnitX   = { 1.f, 0.f, 0.f };
const Vec3 Vec3::kUnitY   = { 0.f, 1.f, 0.f };
const Vec3 Vec3::kUnitZ   = { 0.f, 0.f, 1.f };
const Vec3 Vec3::kRight   = Vec3::kUnitX;
const Vec3 Vec3::kUp      = Vec3::kUnitY;
const Vec3 Vec3::kForward = Vec3::kUnitZ;

const Vec4 Vec4::kZero    = { 0.f, 0.f, 0.f, 0.f };
const Vec4 Vec4::kOne     = { 1.f, 1.f, 1.f, 1.f };
const Vec4 Vec4::kUnitX   = { 1.f, 0.f, 0.f, 0.f };
const Vec4 Vec4::kUnitY   = { 0.f, 1.f, 0.f, 0.f };
const Vec4 Vec4::kUnitZ   = { 0.f, 0.f, 1.f, 0.f };
const Vec4 Vec4::kUnitW   = { 0.f, 0.f, 0.f, 1.f };
const Vec4 Vec4::kRight   = Vec4::kUnitX;
const Vec4 Vec4::kUp      = Vec4::kUnitY;
const Vec4 Vec4::kForward = Vec4::kUnitZ;

}   // namespace Jam