#include "pch.h"

#include "MeshFactory.h"

#include <algorithm>

namespace jam
{

// left-hand coordinate system
MeshGeometry CreatePlaneMesh(float _width, float _height, const Mat4& _transform)
{
    MeshGeometry mesh;

    const float halfWidth  = _width * 0.5f;
    const float halfHeight = _height * 0.5f;

    VertexAttribute v1, v2, v3, v4;

    // 왼쪽
    v1.position = Vec3(-halfWidth, 0.0f, -halfHeight);
    v1.normal   = Vec3(0.0f, 1.0f, 0.0f);
    v1.color    = Vec3(1.0f, 1.0f, 1.0f);
    v1.uv0 = v1.uv1 = Vec2(0.0f, 1.0f);

    // 오른쪽 아래
    v2.position = Vec3(halfWidth, 0.0f, -halfHeight);
    v2.normal   = Vec3(0.0f, 1.0f, 0.0f);
    v2.color    = Vec3(1.0f, 1.0f, 1.0f);
    v2.uv0 = v2.uv1 = Vec2(1.0f, 1.0f);

    // 왼쪽 위
    v3.position = Vec3(-halfWidth, 0.0f, halfHeight);
    v3.normal   = Vec3(0.0f, 1.0f, 0.0f);
    v3.color    = Vec3(1.0f, 1.0f, 1.0f);
    v3.uv0 = v3.uv1 = Vec2(0.0f, 0.0f);

    // 오른쪽 위
    v4.position = Vec3(halfWidth, 0.0f, halfHeight);
    v4.normal   = Vec3(0.0f, 1.0f, 0.0f);
    v4.color    = Vec3(1.0f, 1.0f, 1.0f);
    v4.uv0 = v4.uv1 = Vec2(1.0f, 0.0f);

    // 탄젠트 공간
    v1.tangent = v2.tangent = v3.tangent = v4.tangent = Vec3(1.0f, 0.0f, 0.0f);
    v1.bitangent = v2.bitangent = v3.bitangent = v4.bitangent = Vec3(0.0f, 0.0f, -1.0f);

    // 변환 적용
    if (_transform != Mat4::Identity)
    {
        v1.position = Vec3::Transform(v1.position, _transform);
        v2.position = Vec3::Transform(v2.position, _transform);
        v3.position = Vec3::Transform(v3.position, _transform);
        v4.position = Vec3::Transform(v4.position, _transform);

        v1.normal = Vec3::TransformNormal(v1.normal, _transform);
        v2.normal = Vec3::TransformNormal(v2.normal, _transform);
        v3.normal = Vec3::TransformNormal(v3.normal, _transform);
        v4.normal = Vec3::TransformNormal(v4.normal, _transform);

        v1.tangent = Vec3::TransformNormal(v1.tangent, _transform);
        v2.tangent = Vec3::TransformNormal(v2.tangent, _transform);
        v3.tangent = Vec3::TransformNormal(v3.tangent, _transform);
        v4.tangent = Vec3::TransformNormal(v4.tangent, _transform);

        v1.bitangent = Vec3::TransformNormal(v1.bitangent, _transform);
        v2.bitangent = Vec3::TransformNormal(v2.bitangent, _transform);
        v3.bitangent = Vec3::TransformNormal(v3.bitangent, _transform);
        v4.bitangent = Vec3::TransformNormal(v4.bitangent, _transform);
    }

    // 정점 추가
    mesh.vertices = { v1, v2, v3, v4 };

    // clang-format off

    // 인덱스
    mesh.indices = {
        0, 2, 1, 
        1, 2, 3  
    };

    // clang-format on

    return mesh;
}
MeshGeometry CreateCubeMesh(const float _width, const float _height, const float _depth, const Mat4& _transform)
{
    MeshGeometry mesh;

    const float halfWidth  = _width * 0.5f;
    const float halfHeight = _height * 0.5f;
    const float halfDepth  = _depth * 0.5f;

    const Vec3 positions[8] = {
        // 앞쪽 면
        Vec3(-halfWidth, -halfHeight, halfDepth),
        Vec3(halfWidth, -halfHeight, halfDepth),
        Vec3(halfWidth, halfHeight, halfDepth),
        Vec3(-halfWidth, halfHeight, halfDepth),

        // 뒤쪽 면
        Vec3(-halfWidth, -halfHeight, -halfDepth),
        Vec3(halfWidth, -halfHeight, -halfDepth),
        Vec3(halfWidth, halfHeight, -halfDepth),
        Vec3(-halfWidth, halfHeight, -halfDepth)
    };

    // 법선 벡터
    constexpr Vec3 normals[6] = {
        Vec3(0.0f, 0.0f, 1.0f),    // 앞쪽
        Vec3(0.0f, 0.0f, -1.0f),   // 뒤쪽
        Vec3(1.0f, 0.0f, 0.0f),    // 오른쪽
        Vec3(-1.0f, 0.0f, 0.0f),   // 왼쪽
        Vec3(0.0f, 1.0f, 0.0f),    // 위쪽
        Vec3(0.0f, -1.0f, 0.0f)    // 아래쪽
    };

    // 탄젠트 벡터
    constexpr Vec3 tangents[6] = {
        Vec3(1.0f, 0.0f, 0.0f),    // 앞쪽
        Vec3(-1.0f, 0.0f, 0.0f),   // 뒤쪽
        Vec3(0.0f, 0.0f, -1.0f),   // 오른쪽
        Vec3(0.0f, 0.0f, 1.0f),    // 왼쪽
        Vec3(1.0f, 0.0f, 0.0f),    // 위쪽
        Vec3(1.0f, 0.0f, 0.0f)     // 아래쪽
    };

    // 바이 탄젠트
    constexpr Vec3 bitangents[6] = {
        Vec3(0.0f, -1.0f, 0.0f),   // 앞쪽
        Vec3(0.0f, -1.0f, 0.0f),   // 뒤쪽
        Vec3(0.0f, -1.0f, 0.0f),   // 오른쪽
        Vec3(0.0f, -1.0f, 0.0f),   // 왼쪽
        Vec3(0.0f, 0.0f, -1.0f),   // 위쪽
        Vec3(0.0f, 0.0f, 1.0f)     // 아래쪽
    };

    // UV 좌표
    constexpr Vec2 uvs[4] = {
        Vec2(0.0f, 1.0f),   // 왼쪽 아래
        Vec2(1.0f, 1.0f),   // 오른쪽 아래
        Vec2(1.0f, 0.0f),   // 오른쪽 위
        Vec2(0.0f, 0.0f)    // 왼쪽 위
    };

    // 각 면별 인덱스
    const UInt32 indices[6][4] = {
        { 0, 1, 2, 3 }, // 앞쪽
        { 5, 4, 7, 6 }, // 뒤쪽
        { 1, 5, 6, 2 }, // 오른쪽
        { 4, 0, 3, 7 }, // 왼쪽
        { 3, 2, 6, 7 }, // 위쪽
        { 4, 5, 1, 0 }  // 아래쪽
    };

    // 버텍스 생성
    for (int face = 0; face < 6; face++)
    {
        for (int v = 0; v < 4; v++)
        {
            VertexAttribute vertex;
            vertex.position = positions[indices[face][v]];
            vertex.normal   = normals[face];
            vertex.color    = Vec3(1.0f, 1.0f, 1.0f);
            vertex.uv0 = vertex.uv1 = uvs[v];
            vertex.tangent          = tangents[face];
            vertex.bitangent        = bitangents[face];

            // 변환 적용
            if (_transform != Mat4::Identity)
            {
                vertex.position  = Vec3::Transform(vertex.position, _transform);
                vertex.normal    = Vec3::TransformNormal(vertex.normal, _transform);
                vertex.tangent   = Vec3::TransformNormal(vertex.tangent, _transform);
                vertex.bitangent = Vec3::TransformNormal(vertex.bitangent, _transform);
            }

            mesh.vertices.push_back(vertex);
        }

        UInt32 baseIndex = face * 4;
        mesh.indices.push_back(baseIndex);
        mesh.indices.push_back(baseIndex + 2);
        mesh.indices.push_back(baseIndex + 1);

        mesh.indices.push_back(baseIndex);
        mesh.indices.push_back(baseIndex + 3);
        mesh.indices.push_back(baseIndex + 2);
    }

    return mesh;
}

MeshGeometry CreateSphereMesh(const float _radius, UInt32 _segments, UInt32 _rings, const Mat4& _transform)
{
    MeshGeometry mesh;

    // 최소 세그먼트 및 링 수 보장
    _segments = std::max(_segments, 3u);
    _rings    = std::max(_rings, 2u);

    // 정점 생성
    for (UInt32 ring = 0; ring <= _rings; ring++)
    {
        const float phi       = ring * k_pi / _rings;
        auto [sinPhi, cosPhi] = ComputeSinCos(phi);

        for (UInt32 segment = 0; segment <= _segments; segment++)
        {
            const float theta    = segment * k_2pi / _segments;
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            VertexAttribute vertex;

            vertex.position = Vec3(
                _radius * sinPhi * cosTheta,
                _radius * cosPhi,
                _radius * sinPhi * sinTheta);

            vertex.normal = vertex.position;
            vertex.normal.Normalize();

            // 탄젠트
            vertex.tangent = Vec3(
                -sinTheta,
                0.0f,
                cosTheta);
            vertex.tangent.Normalize();

            // 바이 탄젠트
            vertex.bitangent = vertex.normal.Cross(vertex.tangent);
            vertex.bitangent.Normalize();

            // 텍스처 좌표
            vertex.uv0 = vertex.uv1 = Vec2(
                static_cast<float>(segment) / _segments,
                static_cast<float>(ring) / _rings);

            vertex.color = Vec3(1.0f, 1.0f, 1.0f);

            // 변환 적용
            if (_transform != Mat4::Identity)
            {
                vertex.position  = Vec3::Transform(vertex.position, _transform);
                vertex.normal    = Vec3::TransformNormal(vertex.normal, _transform);
                vertex.tangent   = Vec3::TransformNormal(vertex.tangent, _transform);
                vertex.bitangent = Vec3::TransformNormal(vertex.bitangent, _transform);
            }

            mesh.vertices.push_back(vertex);
        }
    }

    // 인덱스 생성
    for (UInt32 ring = 0; ring < _rings; ring++)
    {
        const UInt32 ringStart     = ring * (_segments + 1);
        const UInt32 nextRingStart = (ring + 1) * (_segments + 1);

        for (UInt32 segment = 0; segment < _segments; segment++)
        {
            // 극점이 아닌 경우 사각형을 두 개의 삼각형으로 분할
            if (ring != 0)
            {
                // 위쪽 삼각형
                mesh.indices.push_back(ringStart + segment);
                mesh.indices.push_back(ringStart + segment + 1);
                mesh.indices.push_back(nextRingStart + segment);
            }

            if (ring != _rings - 1)
            {
                // 아래쪽 삼각형
                mesh.indices.push_back(nextRingStart + segment);
                mesh.indices.push_back(ringStart + segment + 1);
                mesh.indices.push_back(nextRingStart + segment + 1);
            }
        }
    }

    return mesh;
}

MeshGeometry CreateCylinderMesh(float _radius, float _height, UInt32 _segments, const Mat4& _transform)
{
    MeshGeometry mesh;

    // 최소 세그먼트 수 보장
    _segments              = std::max(_segments, 3u);
    const float halfHeight = _height * 0.5f;

    // 상단 중심점
    VertexAttribute topCenter;
    topCenter.position = Vec3(0.0f, halfHeight, 0.0f);
    topCenter.normal   = Vec3(0.0f, 1.0f, 0.0f);
    topCenter.uv0 = topCenter.uv1 = Vec2(0.5f, 0.5f);
    topCenter.tangent             = Vec3(1.0f, 0.0f, 0.0f);
    topCenter.bitangent           = Vec3(0.0f, 0.0f, -1.0f);
    topCenter.color               = Vec3(1.0f, 1.0f, 1.0f);

    // 하단 중심점
    VertexAttribute bottomCenter;
    bottomCenter.position = Vec3(0.0f, -halfHeight, 0.0f);
    bottomCenter.normal   = Vec3(0.0f, -1.0f, 0.0f);
    bottomCenter.uv0 = bottomCenter.uv1 = Vec2(0.5f, 0.5f);
    bottomCenter.tangent                = Vec3(1.0f, 0.0f, 0.0f);
    bottomCenter.bitangent              = Vec3(0.0f, 0.0f, 1.0f);
    bottomCenter.color                  = Vec3(1.0f, 1.0f, 1.0f);

    // 변환 적용
    if (_transform != Mat4::Identity)
    {
        topCenter.position  = Vec3::Transform(topCenter.position, _transform);
        topCenter.normal    = Vec3::TransformNormal(topCenter.normal, _transform);
        topCenter.tangent   = Vec3::TransformNormal(topCenter.tangent, _transform);
        topCenter.bitangent = Vec3::TransformNormal(topCenter.bitangent, _transform);

        bottomCenter.position  = Vec3::Transform(bottomCenter.position, _transform);
        bottomCenter.normal    = Vec3::TransformNormal(bottomCenter.normal, _transform);
        bottomCenter.tangent   = Vec3::TransformNormal(bottomCenter.tangent, _transform);
        bottomCenter.bitangent = Vec3::TransformNormal(bottomCenter.bitangent, _transform);
    }

    mesh.vertices.push_back(topCenter);      // 정점 0
    mesh.vertices.push_back(bottomCenter);   // 정점 1

    // 원형 테두리와 옆면 정점 생성
    for (UInt32 i = 0; i <= _segments; i++)
    {
        float angle = i * k_2pi / _segments;
        float c     = std::cos(angle);
        float s     = std::sin(angle);

        // 상단 원의 테두리 정점
        VertexAttribute topRim;
        topRim.position = Vec3(_radius * c, halfHeight, _radius * s);
        topRim.normal   = Vec3(0.0f, 1.0f, 0.0f);
        topRim.uv0 = topRim.uv1 = Vec2(c * 0.5f + 0.5f, s * 0.5f + 0.5f);
        topRim.tangent          = Vec3(1.0f, 0.0f, 0.0f);
        topRim.bitangent        = Vec3(0.0f, 0.0f, -1.0f);
        topRim.color            = Vec3(1.0f, 1.0f, 1.0f);

        // 하단 원의 테두리 정점
        VertexAttribute bottomRim;
        bottomRim.position = Vec3(_radius * c, -halfHeight, _radius * s);
        bottomRim.normal   = Vec3(0.0f, -1.0f, 0.0f);
        bottomRim.uv0 = bottomRim.uv1 = Vec2(c * 0.5f + 0.5f, s * 0.5f + 0.5f);
        bottomRim.tangent             = Vec3(1.0f, 0.0f, 0.0f);
        bottomRim.bitangent           = Vec3(0.0f, 0.0f, 1.0f);
        bottomRim.color               = Vec3(1.0f, 1.0f, 1.0f);

        // 옆면 정점
        VertexAttribute sideTop;
        sideTop.position = topRim.position;
        sideTop.normal   = Vec3(c, 0.0f, s);
        sideTop.normal.Normalize();
        sideTop.uv0 = sideTop.uv1 = Vec2(static_cast<float>(i) / _segments, 0.0f);
        sideTop.tangent           = Vec3(-s, 0.0f, c);
        sideTop.tangent.Normalize();
        sideTop.bitangent = sideTop.normal.Cross(sideTop.tangent);
        sideTop.color     = Vec3(1.0f, 1.0f, 1.0f);

        // 옆면 정점
        VertexAttribute sideBottom;
        sideBottom.position = bottomRim.position;
        sideBottom.normal   = sideTop.normal;
        sideBottom.uv0 = sideBottom.uv1 = Vec2(static_cast<float>(i) / _segments, 1.0f);
        sideBottom.tangent              = sideTop.tangent;
        sideBottom.bitangent            = sideTop.bitangent;
        sideBottom.color                = Vec3(1.0f, 1.0f, 1.0f);

        // 변환 적용
        if (_transform != Mat4::Identity)
        {
            topRim.position  = Vec3::Transform(topRim.position, _transform);
            topRim.normal    = Vec3::TransformNormal(topRim.normal, _transform);
            topRim.tangent   = Vec3::TransformNormal(topRim.tangent, _transform);
            topRim.bitangent = Vec3::TransformNormal(topRim.bitangent, _transform);

            bottomRim.position  = Vec3::Transform(bottomRim.position, _transform);
            bottomRim.normal    = Vec3::TransformNormal(bottomRim.normal, _transform);
            bottomRim.tangent   = Vec3::TransformNormal(bottomRim.tangent, _transform);
            bottomRim.bitangent = Vec3::TransformNormal(bottomRim.bitangent, _transform);

            sideTop.position  = Vec3::Transform(sideTop.position, _transform);
            sideTop.normal    = Vec3::TransformNormal(sideTop.normal, _transform);
            sideTop.tangent   = Vec3::TransformNormal(sideTop.tangent, _transform);
            sideTop.bitangent = Vec3::TransformNormal(sideTop.bitangent, _transform);

            sideBottom.position  = Vec3::Transform(sideBottom.position, _transform);
            sideBottom.normal    = Vec3::TransformNormal(sideBottom.normal, _transform);
            sideBottom.tangent   = Vec3::TransformNormal(sideBottom.tangent, _transform);
            sideBottom.bitangent = Vec3::TransformNormal(sideBottom.bitangent, _transform);
        }

        mesh.vertices.push_back(topRim);       // 정점 2
        mesh.vertices.push_back(bottomRim);    // 정점 3
        mesh.vertices.push_back(sideTop);      // 정점 4
        mesh.vertices.push_back(sideBottom);   // 정점 5
    }

    // 인덱스 생성
    for (UInt32 i = 0; i < _segments; i++)
    {
        // 상단 원 삼각형
        mesh.indices.push_back(0);
        mesh.indices.push_back(2 + 4 * (i + 1));
        mesh.indices.push_back(2 + 4 * i);

        // 하단 원 삼각형
        mesh.indices.push_back(1);
        mesh.indices.push_back(3 + 4 * i);
        mesh.indices.push_back(3 + 4 * (i + 1));

        // 옆면 사각형
        // 첫 번째 삼각형
        mesh.indices.push_back(4 + 4 * i);
        mesh.indices.push_back(4 + 4 * (i + 1));
        mesh.indices.push_back(5 + 4 * i);

        // 두 번째 삼각형
        mesh.indices.push_back(5 + 4 * i);
        mesh.indices.push_back(4 + 4 * (i + 1));
        mesh.indices.push_back(5 + 4 * (i + 1));
    }

    return mesh;
}

MeshGeometry CreateConeMesh(float _radius, float _height, UInt32 _segments, const Mat4& _transform)
{
    MeshGeometry mesh;

    // 최소 세그먼트 수 보장
    _segments = std::max(_segments, 3u);

    // 꼭지점
    VertexAttribute apex;
    apex.position = Vec3(0.0f, _height, 0.0f);
    apex.normal   = Vec3(0.0f, 1.0f, 0.0f);
    apex.uv0 = apex.uv1 = Vec2(0.5f, 0.5f);
    apex.tangent        = Vec3(1.0f, 0.0f, 0.0f);
    apex.bitangent      = Vec3(0.0f, 0.0f, -1.0f);
    apex.color          = Vec3(1.0f, 1.0f, 1.0f);

    // 바닥 중심점
    VertexAttribute bottomCenter;
    bottomCenter.position = Vec3(0.0f, 0.0f, 0.0f);
    bottomCenter.normal   = Vec3(0.0f, -1.0f, 0.0f);
    bottomCenter.uv0 = bottomCenter.uv1 = Vec2(0.5f, 0.5f);
    bottomCenter.tangent                = Vec3(1.0f, 0.0f, 0.0f);
    bottomCenter.bitangent              = Vec3(0.0f, 0.0f, 1.0f);
    bottomCenter.color                  = Vec3(1.0f, 1.0f, 1.0f);

    // 변환 적용
    if (_transform != Mat4::Identity)
    {
        apex.position  = Vec3::Transform(apex.position, _transform);
        apex.normal    = Vec3::TransformNormal(apex.normal, _transform);
        apex.tangent   = Vec3::TransformNormal(apex.tangent, _transform);
        apex.bitangent = Vec3::TransformNormal(apex.bitangent, _transform);

        bottomCenter.position  = Vec3::Transform(bottomCenter.position, _transform);
        bottomCenter.normal    = Vec3::TransformNormal(bottomCenter.normal, _transform);
        bottomCenter.tangent   = Vec3::TransformNormal(bottomCenter.tangent, _transform);
        bottomCenter.bitangent = Vec3::TransformNormal(bottomCenter.bitangent, _transform);
    }

    mesh.vertices.push_back(apex);           // 정점 0
    mesh.vertices.push_back(bottomCenter);   // 정점 1

    // 원의 바닥 테두리 정점 생성
    for (UInt32 i = 0; i <= _segments; i++)
    {
        float angle = i * k_2pi / _segments;
        float c     = std::cos(angle);
        float s     = std::sin(angle);

        // 바닥 원의 테두리 정점
        VertexAttribute bottomRim;
        bottomRim.position = Vec3(_radius * c, 0.0f, _radius * s);
        bottomRim.normal   = Vec3(0.0f, -1.0f, 0.0f);
        bottomRim.uv0 = bottomRim.uv1 = Vec2(c * 0.5f + 0.5f, s * 0.5f + 0.5f);
        bottomRim.tangent             = Vec3(1.0f, 0.0f, 0.0f);
        bottomRim.bitangent           = Vec3(0.0f, 0.0f, 1.0f);
        bottomRim.color               = Vec3(1.0f, 1.0f, 1.0f);

        // 옆면 정점
        VertexAttribute side;
        side.position = bottomRim.position;

        // 원뿔의 옆면 법선 벡터 계산
        Vec3 toApex     = Vec3(0.0f, _height, 0.0f) - bottomRim.position;
        Vec3 sideNormal = toApex.Cross(Vec3(-s, 0.0f, c));
        sideNormal.Normalize();
        side.normal = sideNormal;

        side.uv0 = side.uv1 = Vec2(static_cast<float>(i) / _segments, 1.0f);
        side.tangent        = Vec3(-s, 0.0f, c);
        side.tangent.Normalize();
        side.bitangent = side.normal.Cross(side.tangent);
        side.color     = Vec3(1.0f, 1.0f, 1.0f);

        // 변환 적용
        if (_transform != Mat4::Identity)
        {
            bottomRim.position  = Vec3::Transform(bottomRim.position, _transform);
            bottomRim.normal    = Vec3::TransformNormal(bottomRim.normal, _transform);
            bottomRim.tangent   = Vec3::TransformNormal(bottomRim.tangent, _transform);
            bottomRim.bitangent = Vec3::TransformNormal(bottomRim.bitangent, _transform);

            side.position  = Vec3::Transform(side.position, _transform);
            side.normal    = Vec3::TransformNormal(side.normal, _transform);
            side.tangent   = Vec3::TransformNormal(side.tangent, _transform);
            side.bitangent = Vec3::TransformNormal(side.bitangent, _transform);
        }

        mesh.vertices.push_back(bottomRim);   // 정점 2+2i
        mesh.vertices.push_back(side);        // 정점 3+2i
    }

    // 인덱스 생성
    for (UInt32 i = 0; i < _segments; i++)
    {
        // 바닥 원 삼각형
        mesh.indices.push_back(1);                 // 바닥 중심
        mesh.indices.push_back(2 + 2 * i);         // 현재 테두리 정점
        mesh.indices.push_back(2 + 2 * (i + 1));   // 다음 테두리 정점

        // 옆면 삼각형
        mesh.indices.push_back(0);                 // 꼭지점
        mesh.indices.push_back(3 + 2 * (i + 1));   // 다음 옆면 정점
        mesh.indices.push_back(3 + 2 * i);         // 현재 옆면 정점
    }

    return mesh;
}

MeshGeometry CreateCapsuleMesh(float _radius, float _height, UInt32 _segments, UInt32 _rings, const Mat4& _transform)
{
    MeshGeometry mesh;

    // 최소 세그먼트 및 링 수 보장
    _segments = std::max(_segments, 3u);
    _rings    = std::max(_rings, 2u);

    float halfHeight = _height * 0.5f;

    // 실린더 부분의 높이 계산
    float cylinderHeight = _height - 2.0f * _radius;
    cylinderHeight       = std::max(cylinderHeight, 0.0f);

    float topSphereYStart  = halfHeight - _radius;
    float bottomSphereYEnd = -halfHeight + _radius;

    // 1. 상단 반구 정점 생성
    for (UInt32 ring = 0; ring <= _rings / 2; ring++)
    {
        float phi    = ring * k_pi / _rings;
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        float y = _radius * cosPhi + topSphereYStart;

        for (UInt32 segment = 0; segment <= _segments; segment++)
        {
            float theta    = segment * k_2pi / _segments;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            VertexAttribute vertex;

            // 위치 계산
            vertex.position = Vec3(
                _radius * sinPhi * cosTheta,
                y,
                _radius * sinPhi * sinTheta);

            // 법선 벡터
            vertex.normal = Vec3(
                sinPhi * cosTheta,
                cosPhi,
                sinPhi * sinTheta);
            vertex.normal.Normalize();

            // 접선 벡터
            vertex.tangent = Vec3(
                -sinTheta,
                0.0f,
                cosTheta);
            vertex.tangent.Normalize();

            // 종법선 벡터
            vertex.bitangent = vertex.normal.Cross(vertex.tangent);
            vertex.bitangent.Normalize();

            // 텍스처 좌표
            vertex.uv0 = vertex.uv1 = Vec2(
                static_cast<float>(segment) / _segments,
                static_cast<float>(ring) / _rings);

            vertex.color = Vec3(1.0f, 1.0f, 1.0f);

            // 변환 적용
            if (_transform != Mat4::Identity)
            {
                vertex.position  = Vec3::Transform(vertex.position, _transform);
                vertex.normal    = Vec3::TransformNormal(vertex.normal, _transform);
                vertex.tangent   = Vec3::TransformNormal(vertex.tangent, _transform);
                vertex.bitangent = Vec3::TransformNormal(vertex.bitangent, _transform);
            }

            mesh.vertices.push_back(vertex);
        }
    }

    // 2. 중앙 실린더 정점 생성 (cylinderHeight > 0인 경우만)
    if (cylinderHeight > 0.0f)
    {
        for (UInt32 ring = 0; ring <= 1; ring++)
        {
            float y = ring == 0 ? topSphereYStart : bottomSphereYEnd;

            for (UInt32 segment = 0; segment <= _segments; segment++)
            {
                float theta    = segment * k_2pi / _segments;
                float sinTheta = std::sin(theta);
                float cosTheta = std::cos(theta);

                VertexAttribute vertex;

                // 위치 계산
                vertex.position = Vec3(
                    _radius * cosTheta,
                    y,
                    _radius * sinTheta);

                // 법선 벡터 (표면 바깥쪽 방향)
                vertex.normal = Vec3(
                    cosTheta,
                    0.0f,
                    sinTheta);
                vertex.normal.Normalize();

                // 접선 벡터
                vertex.tangent = Vec3(
                    -sinTheta,
                    0.0f,
                    cosTheta);
                vertex.tangent.Normalize();

                // 종법선 벡터
                vertex.bitangent = vertex.normal.Cross(vertex.tangent);
                vertex.bitangent.Normalize();

                // 텍스처 좌표
                vertex.uv0 = vertex.uv1 = Vec2(
                    static_cast<float>(segment) / static_cast<float>(_segments),
                    static_cast<float>(ring));

                vertex.color = Vec3(1.0f, 1.0f, 1.0f);

                // 변환 적용
                if (_transform != Mat4::Identity)
                {
                    vertex.position  = Vec3::Transform(vertex.position, _transform);
                    vertex.normal    = Vec3::TransformNormal(vertex.normal, _transform);
                    vertex.tangent   = Vec3::TransformNormal(vertex.tangent, _transform);
                    vertex.bitangent = Vec3::TransformNormal(vertex.bitangent, _transform);
                }

                mesh.vertices.push_back(vertex);
            }
        }
    }

    // 3. 하단 반구 정점 생성
    for (UInt32 ring = _rings / 2; ring <= _rings; ring++)
    {
        float phi    = ring * k_pi / _rings;
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        float y = _radius * cosPhi + bottomSphereYEnd;

        for (UInt32 segment = 0; segment <= _segments; segment++)
        {
            float theta    = segment * k_2pi / _segments;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            VertexAttribute vertex;

            // 위치 계산
            vertex.position = Vec3(
                _radius * sinPhi * cosTheta,
                y,
                _radius * sinPhi * sinTheta);

            // 법선 벡터
            vertex.normal = Vec3(
                sinPhi * cosTheta,
                cosPhi,
                sinPhi * sinTheta);
            vertex.normal.Normalize();

            // 접선 벡터
            vertex.tangent = Vec3(
                -sinTheta,
                0.0f,
                cosTheta);
            vertex.tangent.Normalize();

            // 종법선 벡터
            vertex.bitangent = vertex.normal.Cross(vertex.tangent);
            vertex.bitangent.Normalize();

            // 텍스처 좌표
            vertex.uv0 = vertex.uv1 = Vec2(
                static_cast<float>(segment) / _segments,
                static_cast<float>(ring) / _rings);

            vertex.color = Vec3(1.0f, 1.0f, 1.0f);

            // 변환 적용
            if (_transform != Mat4::Identity)
            {
                vertex.position  = Vec3::Transform(vertex.position, _transform);
                vertex.normal    = Vec3::TransformNormal(vertex.normal, _transform);
                vertex.tangent   = Vec3::TransformNormal(vertex.tangent, _transform);
                vertex.bitangent = Vec3::TransformNormal(vertex.bitangent, _transform);
            }

            mesh.vertices.push_back(vertex);
        }
    }

    // 인덱스 생성

    // 1. 상단 반구 인덱스
    UInt32 topSphereRings = _rings / 2 + 1;
    for (UInt32 ring = 0; ring < topSphereRings - 1; ring++)
    {
        UInt32 ringStart     = ring * (_segments + 1);
        UInt32 nextRingStart = (ring + 1) * (_segments + 1);

        for (UInt32 segment = 0; segment < _segments; segment++)
        {
            // 각 사각형을 두 개의 삼각형으로 분할 (왼손 좌표계 - 시계 방향)

            // 위쪽 삼각형
            mesh.indices.push_back(ringStart + segment);
            mesh.indices.push_back(ringStart + segment + 1);
            mesh.indices.push_back(nextRingStart + segment);

            // 아래쪽 삼각형
            mesh.indices.push_back(nextRingStart + segment);
            mesh.indices.push_back(ringStart + segment + 1);
            mesh.indices.push_back(nextRingStart + segment + 1);
        }
    }

    // 2. 중앙 실린더 인덱스 (cylinderHeight > 0인 경우만)
    if (cylinderHeight > 0.0f)
    {
        UInt32 cylinderStart = topSphereRings * (_segments + 1);

        for (UInt32 segment = 0; segment < _segments; segment++)
        {
            // 각 사각형을 두 개의 삼각형으로 분할 (왼손 좌표계 - 시계 방향)

            // 위쪽 삼각형
            mesh.indices.push_back(cylinderStart + segment);
            mesh.indices.push_back(cylinderStart + segment + 1);
            mesh.indices.push_back(cylinderStart + (_segments + 1) + segment);

            // 아래쪽 삼각형
            mesh.indices.push_back(cylinderStart + (_segments + 1) + segment);
            mesh.indices.push_back(cylinderStart + segment + 1);
            mesh.indices.push_back(cylinderStart + (_segments + 1) + segment + 1);
        }
    }

    // 3. 하단 반구 인덱스
    UInt32 bottomSphereStart = topSphereRings * (_segments + 1);
    if (cylinderHeight > 0.0f)
    {
        bottomSphereStart += 2 * (_segments + 1);
    }

    UInt32 bottomSphereRings = _rings - _rings / 2 + 1;
    for (UInt32 ring = 0; ring < bottomSphereRings - 1; ring++)
    {
        UInt32 ringStart     = bottomSphereStart + ring * (_segments + 1);
        UInt32 nextRingStart = bottomSphereStart + (ring + 1) * (_segments + 1);

        for (UInt32 segment = 0; segment < _segments; segment++)
        {
            // 각 사각형을 두 개의 삼각형으로 분할 (왼손 좌표계 - 시계 방향)

            // 위쪽 삼각형
            mesh.indices.push_back(ringStart + segment);
            mesh.indices.push_back(ringStart + segment + 1);
            mesh.indices.push_back(nextRingStart + segment);

            // 아래쪽 삼각형
            mesh.indices.push_back(nextRingStart + segment);
            mesh.indices.push_back(ringStart + segment + 1);
            mesh.indices.push_back(nextRingStart + segment + 1);
        }
    }

    return mesh;
}

MeshGeometry CreateGridMesh(const float _width, const float _height, UInt32 _rows, UInt32 _columns, const Mat4& _transform)
{
    MeshGeometry mesh;

    // 최소 행/열 수 보장
    _rows    = std::max(_rows, 1u);
    _columns = std::max(_columns, 1u);

    const float halfWidth  = _width * 0.5f;
    const float halfHeight = _height * 0.5f;

    const float cellWidth  = _width / _columns;
    const float cellHeight = _height / _rows;

    // 그리드의 정점 생성 (XZ 평면 상에 정점 생성)
    for (UInt32 row = 0; row <= _rows; row++)
    {
        for (UInt32 col = 0; col <= _columns; col++)
        {
            VertexAttribute vertex;

            // 위치 계산 (왼손 좌표계)
            vertex.position = Vec3(
                -halfWidth + col * cellWidth,    // x
                0.0f,                            // y
                -halfHeight + row * cellHeight   // z
            );

            // 법선 벡터 (위로 향함)
            vertex.normal = Vec3(0.0f, 1.0f, 0.0f);

            // 접선 벡터 (X 방향)
            vertex.tangent = Vec3(1.0f, 0.0f, 0.0f);

            // 종법선 벡터 (Z 방향 - 왼손 좌표계에서는 음의 Z)
            vertex.bitangent = Vec3(0.0f, 0.0f, -1.0f);

            // 텍스처 좌표
            vertex.uv0 = vertex.uv1 = Vec2(
                static_cast<float>(col) / _columns,
                static_cast<float>(row) / _rows);

            vertex.color = Vec3(1.0f, 1.0f, 1.0f);

            // 변환 적용
            if (_transform != Mat4::Identity)
            {
                vertex.position  = Vec3::Transform(vertex.position, _transform);
                vertex.normal    = Vec3::TransformNormal(vertex.normal, _transform);
                vertex.tangent   = Vec3::TransformNormal(vertex.tangent, _transform);
                vertex.bitangent = Vec3::TransformNormal(vertex.bitangent, _transform);
            }

            mesh.vertices.push_back(vertex);
        }
    }

    // 인덱스 생성
    for (UInt32 row = 0; row < _rows; row++)
    {
        for (UInt32 col = 0; col < _columns; col++)
        {
            UInt32 bottomLeft  = row * (_columns + 1) + col;
            UInt32 bottomRight = bottomLeft + 1;
            UInt32 topLeft     = (row + 1) * (_columns + 1) + col;
            UInt32 topRight    = topLeft + 1;

            // 각 사각형을 두 개의 삼각형으로 분할 (왼손 좌표계 - 시계 방향)

            // 아래쪽 삼각형
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(bottomRight);

            // 위쪽 삼각형
            mesh.indices.push_back(bottomRight);
            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(topRight);
        }
    }

    return mesh;
}

}   // namespace jam