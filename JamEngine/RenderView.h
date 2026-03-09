#pragma once
#include "GraphicsBase.h"

namespace Jam
{

struct RenderView
{
    FrameBufferHandle fbh = k_nullHandle;   // output
    Viewport          vp  = {};                // output vp

    Vector3 eye  = Vec3::k_zero;       // camera position
    Matrix4x4 view = Mtx4::k_identity;   // view matrix
    Matrix4x4 proj = Mtx4::k_identity;   // projection matrix
};

}   // namespace Jam