#pragma once
#include "GraphicsBase.h"

namespace jam
{

class ConstByteView;

// triangle mesh
struct LoopSubdivisionOutput
{
    std::vector<Vec3> positions = {};
    std::vector<Vec2> texcoords = {};
    std::vector<Byte> indices   = {};
    bool              bUseU32   = false;
};

/// @brief Loop Subdivision 알고리즘을 사용하여 삼각형 메쉬를 세분화.
/// @param _positions 위치 정보
/// @param _texcoordsOrEmpty 텍스처 좌표 정보 (없으면 빈 span 전달)
/// @param _indexData 인덱스 정보
/// @param _bUseU32 U32 인텍스 포맷을 사용할 것인지
/// @param _level 세분화 레벨 (0 이상. 세분화 레벨 1당 삼각형 개수가 4배 증가)
LoopSubdivisionOutput LoopSubdivide(
    std::span<const Vec3> _positions,
    std::span<const Vec2> _texcoordsOrEmpty,
    ConstMemView          _indexData,
    bool                  _bUseU32,
    Int32                 _level);

}   // namespace jam
