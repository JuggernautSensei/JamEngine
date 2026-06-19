#pragma once

namespace Jam
{

struct LoopSubdivisionResult
{
    Vector<Vec3> positions = {};
    Vector<Vec2> texcoords = {};
    Vector<Byte> indices   = {};
    bool         bUseU32   = false;
};

LoopSubdivisionResult LoopSubdivide(
    Span<const Vec3> _positions,
    MemoryView       _indexData,
    bool             _bUseU32,
    Int32            _level);

LoopSubdivisionResult LoopSubdivide(
    Span<const Vec3> _positions,
    Span<const Vec2> _texcoords,
    MemoryView       _indexData,
    bool             _bUseU32,
    Int32            _level);

}   // namespace Jam
