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
        VertexLayout layout;
        layout
            .Add(eVertexAttribute::Position, eAttributeFormat::Float2)
            .Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
        return layout;
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
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3)
            .Add(eVertexAttribute::Normal, eAttributeFormat::Float3)
            .Add(eVertexAttribute::Tangent, eAttributeFormat::Float3)
            .Add(eVertexAttribute::TexCoord0, eAttributeFormat::Float2)
            .Add(eVertexAttribute::TexCoord1, eAttributeFormat::Float2);
        return layout;
    }();
};

struct VertexPos3D
{
    Vec3 position;

    inline static VertexLayout s_layout = []
    {
        VertexLayout layout;
        layout.Add(eVertexAttribute::Position, eAttributeFormat::Float3);
        return layout;
    }();
};

class BasicRenderer
{
public:
    static void Init();
    static void Shutdown();
    static void Render(const RenderView& _view, const Scene& _scene);

    [[nodiscard]] static TextureHandle GetDepthTexture();
};

}   // namespace jam
