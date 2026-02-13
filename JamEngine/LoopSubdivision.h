#pragma once

namespace Jam
{

struct LoopSubdivisionOutput
{
    Vector<Vec3> positions = {};
    Vector<Byte> indices   = {};
    bool         bUseU32   = false;
};

struct LoopSubdivisionOutputWithTexcoords
{
    Vector<Vec3> positions = {};
    Vector<Vec2> texcoords = {};
    Vector<Byte> indices   = {};
    bool         bUseU32   = false;
};

LoopSubdivisionOutput LoopSubdivide(
    Span<const Vec3> _positions,
    CMemoryBlock     _idxData,
    bool             _bUseU32,
    Int32            _level);

LoopSubdivisionOutputWithTexcoords LoopSubdivide(
    Span<const Vec3> _positions,
    Span<const Vec2> _texcoordsOrEmpty,
    CMemoryBlock     _idxData,
    bool             _bUseU32,
    Int32            _level);

}   // namespace Jam
