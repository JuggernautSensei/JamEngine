#pragma once

enum class eShaderFlags : UInt32
{
    None     = 0,
    Vertex   = 1 << 0,
    Pixel    = 1 << 1,
    Geometry = 1 << 2,
    Hull     = 1 << 3,
    Domain   = 1 << 4,
    Compute  = 1 << 5,
};

[[nodiscard]] ProgramHandle LoadProgram(
    StringView              _path,
    Span<const ShaderMacro> _macros = {},
    Flags<eShaderFlags>     _flags  = eShaderFlags::Vertex | eShaderFlags::Pixel);

[[nodiscard]] Ref<Mesh> LoadMesh(
    StringView          _path,
    const VertexLayout& _attribLayoutOrEmpty);