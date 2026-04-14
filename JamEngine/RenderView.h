#pragma once
#include "GraphicsBase.h"

namespace Jam
{

struct RenderView
{
    FrameBufferHandle fbh      = k_invalidHandle;   // output
    Viewport          viewport = {};             // output vp

    Vec3 eyePos = Numbers<Vec3>::k_zero;       // eye position
    Mtx4 view   = Numbers<Mtx4>::k_identity;   // view matrix
    Mtx4 proj   = Numbers<Mtx4>::k_identity;   // projection matrix
};

}   // namespace Jam