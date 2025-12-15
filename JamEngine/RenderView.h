#pragma once
#include "GraphicsBase.h"

namespace jam
{

struct RenderView
{
    FrameBufferHandle fbh = k_invalidHandle;   // output
    Viewport          vp  = {};                // output vp

    Vec3 eye  = Vec3::Zero;       // camera position
    Mat4 view = Mat4::Identity;   // view matrix
    Mat4 proj = Mat4::Identity;   // projection matrix
};

}   // namespace jam