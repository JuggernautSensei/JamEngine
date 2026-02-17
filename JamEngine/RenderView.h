#pragma once
#include "GraphicsBase.h"

namespace Jam
{

struct RenderView
{
    FrameBufferHandle fbh = k_nullHandle;   // output
    Viewport          vp  = {};                // output vp

    Vec3 eye  = vec3::k_zero;       // camera position
    Mtx4 view = mtx4::k_identity;   // view matrix
    Mtx4 proj = mtx4::k_identity;   // projection matrix
};

}   // namespace Jam