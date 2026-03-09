#pragma once

namespace Jam
{

struct LoopSubdivisionOutput
{
    Vector<Vector3> positions = {};
    Vector<Byte> indices   = {};
    bool         bUseU32   = false;
};

struct LoopSubdivisionOutputWithTexcoords
{
    Vector<Vector3> positions = {};
    Vector<Vector2> texcoords = {};
    Vector<Byte> indices   = {};
    bool         bUseU32   = false;
};

LoopSubdivisionOutput LoopSubdivide(
    Span<const Vector3> _positions,
    CMemoryBlock     _idxData,
    bool             _bUseU32,
    Int32            _level);

LoopSubdivisionOutputWithTexcoords LoopSubdivide(
    Span<const Vector3> _positions,
    Span<const Vector2> _texcoordsOrEmpty,
    CMemoryBlock     _idxData,
    bool             _bUseU32,
    Int32            _level);

}   // namespace Jam
