#pragma once
#include "../JamEngine/Random.h"

extern RngMWC g_rng;

[[nodiscard]] float MakeRandomFloat(float _min, float _max);
[[nodiscard]] Vec2  MakeRandomVec2(float _minX, float _maxX, float _minY, float _maxY);
[[nodiscard]] Vec3  MakeRandomVec3(float _minX, float _maxX, float _minY, float _maxY, float _minZ, float _maxZ);
[[nodiscard]] RGBA  MakeRandomOpaqueRGBA(UInt8 _rMin, UInt8 _rMax, UInt8 _gMin, UInt8 _gMax, UInt8 _bMin, UInt8 _bMax);
[[nodiscard]] RGBA  MakeRandomOpaqueRGBAFromLinear(float _rMin, float _rMax, float _gMin, float _gMax, float _bMin, float _bMax);
[[nodiscard]] RGBA  MakeRandomOpaqueRGBAFromHSV(float _hMinRad, float _hMaxRad, float _sMin, float _sMax, float _vMin, float _vMax);