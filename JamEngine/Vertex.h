#pragma once

namespace jam
{

struct VertexAttribute
{
    Vec3 position  = Vec3::Zero;
    Vec3 color     = Vec3::One;
    Vec3 normal    = Vec3::Zero;
    Vec2 uv0       = Vec2::Zero;
    Vec2 uv1       = Vec2::Zero;
    Vec3 tangent   = Vec3::Zero;
    Vec3 bitangent = Vec3::Zero;
};

enum class eVertexType
{
    Vertex2,
    Vertex3,
    Vertex3PosOnly
};

UInt32 GetVertexStride(eVertexType _type);
void   PackVertex(const VertexAttribute& _vertex, eVertexType _type, void* _out_vertex);
void   UnpackVertex(eVertexType _type, const void* _in_vertex, VertexAttribute& _out_vertex);

struct Vertex2
{
    Vec2 position;
    Vec2 uv0;

    inline static eVertexType s_staticType = eVertexType::Vertex2;
};

struct Vertex3
{
    Vec3 position;
    Vec2 uv0;
    Vec2 uv1;
    Vec3 normal;
    Vec3 tangent;

    inline static eVertexType s_staticType = eVertexType::Vertex3;
};

struct Vertex3PosOnly
{
    Vec3 position;

    inline static eVertexType s_staticType = eVertexType::Vertex3PosOnly;
};

}   // namespace jam