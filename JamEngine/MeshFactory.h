#pragma once
#include "Mesh.h"

namespace jam
{

MeshData CreatePlaneMesh(float _width, float _height, const Mat4& _transform = Mat4::Identity);
MeshData CreateCubeMesh(float _width, float _height, float _depth, const Mat4& _transform = Mat4::Identity);
MeshData CreateSphereMesh(float _radius, UInt32 _segments, UInt32 _rings, const Mat4& _transform = Mat4::Identity);
MeshData CreateCylinderMesh(float _radius, float _height, UInt32 _segments, const Mat4& _transform = Mat4::Identity);
MeshData CreateConeMesh(float _radius, float _height, UInt32 _segments, const Mat4& _transform = Mat4::Identity);
MeshData CreateCapsuleMesh(float _radius, float _height, UInt32 _segments, UInt32 _rings, const Mat4& _transform = Mat4::Identity);
MeshData CreateGridMesh(float _width, float _height, UInt32 _rows, UInt32 _columns, const Mat4& _transform = Mat4::Identity);

}   // namespace jam