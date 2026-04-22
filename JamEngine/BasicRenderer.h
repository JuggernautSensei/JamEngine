#pragma once
#include "Scene.h"
//
//namespace Jam
//{
//
//struct RenderView;
//class Transform;
//class Material;
//class Mesh;
//class Timer;
//struct Frustum;
//class RenderGraph;
//
//struct VertexPosTex2D
//{
//    Vec2 position = Vec2::kZero;
//    Vec2 texCoord = Vec2::kZero;
//
//    inline static VertexLayout s_layout = []
//    {
//        VertexLayout vl = {};
//        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float2);
//        vl.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
//        return vl;
//    }();
//};
//
//struct VertexStaticMesh
//{
//    Vec3 position  = Vec3::kZero;
//    Vec3 normal    = Vec3::kZero;
//    Vec3 tangent   = Vec3::kZero;
//    Vec2 texCoord0 = Vec2::kZero;
//    Vec2 texCoord1 = Vec2::kZero;
//
//    inline static VertexLayout s_layout = []
//    {
//        VertexLayout vl = {};
//        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
//        vl.Push(eVertexAttribute::Normal, eAttributeFormat::Float3);
//        vl.Push(eVertexAttribute::Tangent, eAttributeFormat::Float3);
//        vl.Push(eVertexAttribute::TexCoord0, eAttributeFormat::Float2);
//        vl.Push(eVertexAttribute::TexCoord1, eAttributeFormat::Float2);
//        return vl;
//    }();
//};
//
//struct VertexPos3D
//{
//    Vec3 position = Vec3::kZero;
//
//    inline static VertexLayout s_layout = []
//    {
//        VertexLayout vl;
//        vl.Push(eVertexAttribute::Position, eAttributeFormat::Float3);
//        return vl;
//    }();
//};
//
//class BasicRenderer
//{
//public:
//    static void InitFileWatcher();
//    static void Shutdown();
//    static void Render(const RenderView& _renderView, const Ref<Scene>& _scene);
//
//    [[nodiscard]] static TextureHandle GetDepthTexture();
//};
//
//}   // namespace Jam
