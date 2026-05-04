#pragma once
#include "GraphicsBase.h"

namespace Jam
{

struct RenderView
{
    FrameBufferHandle fbh = kInvalidHandle;   // output
    Viewport          vp  = {};               // output vp

    Vec3 eye  = Vec3::kZero;       // eye position
    Mtx4 view = Mtx4::kIdentity;   // view matrix
    Mtx4 proj = Mtx4::kIdentity;   // projection matrix
};

}   // namespace Jam