#include "pch.h"

#include "Vertex.h"

namespace jam
{

UInt32 GetVertexStride(const eVertexType _type)
{
    switch (_type)
    {
        case eVertexType::Vertex2:
            return sizeof(Vertex2);

        case eVertexType::Vertex3:
            return sizeof(Vertex3);

        default:
            JAM_ASSERT(false, "Unknown vertex type");
            return 0;
    }
}

void PackVertex(const VertexAttribute& _vertex, const eVertexType _type, void* _out_vertex)
{
    switch (_type)
    {
        case eVertexType::Vertex2:
        {
            Vertex2* vertex  = static_cast<Vertex2*>(_out_vertex);
            vertex->position = ToVec2(_vertex.position);
            vertex->uv0      = _vertex.uv0;
            vertex->color    = _vertex.color;
            break;
        }

        case eVertexType::Vertex3:
        {
            Vertex3* vertex  = static_cast<Vertex3*>(_out_vertex);
            vertex->position = _vertex.position;
            vertex->uv0      = _vertex.uv0;
            vertex->uv1      = _vertex.uv1;
            vertex->normal   = _vertex.normal;
            vertex->tangent  = _vertex.tangent;
            break;
        }

        default:
            JAM_ASSERT(false, "Unknown vertex type");
    }
}

void UnpackVertex(const eVertexType _type, const void* _in_vertex, VertexAttribute& _out_vertex)
{
    switch (_type)
    {
        case eVertexType::Vertex2:
        {
            const Vertex2* vertex = static_cast<const Vertex2*>(_in_vertex);
            _out_vertex.position  = ToVec3(vertex->position);
            _out_vertex.uv0       = vertex->uv0;
            _out_vertex.color     = vertex->color;
            break;
        }

        case eVertexType::Vertex3:
        {
            const Vertex3* vertex = static_cast<const Vertex3*>(_in_vertex);
            _out_vertex.position  = vertex->position;
            _out_vertex.uv0       = vertex->uv0;
            _out_vertex.uv1       = vertex->uv1;
            _out_vertex.normal    = vertex->normal;
            _out_vertex.tangent   = vertex->tangent;
            break;
        }

        default:
            JAM_ASSERT(false, "Unknown vertex type");
    }
}

}   // namespace jam