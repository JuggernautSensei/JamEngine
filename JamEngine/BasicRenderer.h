#pragma once
#include "Scene.h"

namespace jam
{

struct RenderView;
class Transform;
class Material;
class Mesh;
class Timer;
struct Frustum;
class RenderGraph;

struct VertexPosTex2D
{
    Vec2 position;
    Vec2 texCoord;

    inline static VertexLayout s_layout = []
    {
        VertexLayout vl;
        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float2)
            .Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
        return vl;
    }();
};

struct VertexStaticMesh
{
    Vec3 position;
    Vec3 normal;
    Vec3 tangent;
    Vec2 texCoord0;
    Vec2 texCoord1;

    inline static VertexLayout s_layout = []
    {
        VertexLayout vl;
        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3)
            .Push(eVertexAttribute::Normal, eAttributeFormat::Float3)
            .Push(eVertexAttribute::Tangent, eAttributeFormat::Float3)
            .Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2)
            .Push(eVertexAttribute::TexCoord1, eAttributeFormat::Float2);
        return vl;
    }();
};

struct VertexPos3D
{
    Vec3 position;

    inline static VertexLayout s_layout = []
    {
        VertexLayout vl;
        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
        return vl;
    }();
};

class BasicRenderer
{
public:
    static void Init();
    static void Shutdown();
    static void Render(const RenderView& _renderView, const Scene& _scene);

    [[nodiscard]] static TextureHandle GetDepthTexture();
};

}   // namespace jam
