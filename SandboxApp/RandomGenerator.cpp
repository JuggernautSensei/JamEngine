#include "pch.h"
#include "RandomGenerator.h"

RngMWC g_rng;

float MakeRandomFloat(const float _min, const float _max)
{
    return MakeRandom<float>(g_rng, _min, _max);
}

Vec2 MakeRandomVec2(const float _minX, const float _maxX, const float _minY, const float _maxY)
{
    return { MakeRandomFloat(_minX, _maxX), MakeRandomFloat(_minY, _maxY) };
}

Vec3 MakeRandomVec3(const float _minX, const float _maxX, const float _minY, const float _maxY, const float _minZ, const float _maxZ)
{
    return { MakeRandomFloat(_minX, _maxX), MakeRandomFloat(_minY, _maxY), MakeRandomFloat(_minZ, _maxZ) };
}

RGBA MakeRandomOpaqueRGBA(const UInt8 _rMin, const UInt8 _rMax, const UInt8 _gMin, const UInt8 _gMax, const UInt8 _bMin, const UInt8 _bMax)
{
    return { MakeRandom(g_rng, _rMin, _rMax), MakeRandom(g_rng, _gMin, _gMax), MakeRandom(g_rng, _bMin, _bMax) };
}

RGBA MakeRandomOpaqueRGBAFromLinear(const float _rMin, const float _rMax, const float _gMin, const float _gMax, const float _bMin, const float _bMax)
{
    return RGBA::FromLinear(MakeRandomFloat(_rMin, _rMax), MakeRandomFloat(_gMin, _gMax), MakeRandomFloat(_bMin, _bMax));
}

RGBA MakeRandomOpaqueRGBAFromHSV(const float _hMinRad, const float _hMaxRad, const float _sMin, const float _sMax, const float _vMin, const float _vMax)
{
    return RGBA::FromHSV(MakeRandomFloat(_hMinRad, _hMaxRad), MakeRandomFloat(_sMin, _sMax), MakeRandomFloat(_vMin, _vMax));
}