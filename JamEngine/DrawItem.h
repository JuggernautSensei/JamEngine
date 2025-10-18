#pragma once

namespace jam
{

class Mesh;
class Material;

struct DrawItem
{
    Mesh*     pMesh     = nullptr;
    Material* pMaterial = nullptr;
    Mat4      world     = Mat4::Identity;
    UInt64    sortKey   = 0;
};

}   // namespace jam