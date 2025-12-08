#pragma once
#include "Material.h"
#include "Mesh.h"

namespace jam
{

class Transform;

struct Renderable
{
    Ref<Mesh>     pMesh     = nullptr;
    Ref<Material> pMaterial = nullptr;
};

// immediately renderable item (do not have ownership)
struct VisibleItem
{
    const Renderable* renderable  = nullptr;
    const Transform*  pTransform  = nullptr;
    UInt16            encodeDepth = 0;
};

}   // namespace jam
