#pragma once
#include "Mesh.h"

namespace jam
{

MeshGeometry CreatePlaneMesh(float _width, float _height, const Mat4& _transform = Mat4::Identity);
MeshGeometry CreateCubeMesh(float _width, float _height, float _depth, const Mat4& _transform = Mat4::Identity);
MeshGeometry CreateSphereMesh(float _radius, UInt32 _segments, UInt32 _rings, const Mat4& _transform = Mat4::Identity);
MeshGeometry CreateCylinderMesh(float _radius, float _height, UInt32 _segments, const Mat4& _transform = Mat4::Identity);
MeshGeometry CreateConeMesh(float _radius, float _height, UInt32 _segments, const Mat4& _transform = Mat4::Identity);
MeshGeometry CreateCapsuleMesh(float _radius, float _height, UInt32 _segments, UInt32 _rings, const Mat4& _transform = Mat4::Identity);
MeshGeometry CreateGridMesh(float _width, float _height, UInt32 _rows, UInt32 _columns, const Mat4& _transform = Mat4::Identity);

}   // namespace jam