#pragma once

#define MODEL_PATH(_path)   "Resources/Models/" _path
#define TEXTURE_PATH(_path) "Resources/Textures/" _path

namespace Jam
{

class RenderGraph;

}

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
    Flags<eShaderFlags>     _flags         = eShaderFlags::Vertex | eShaderFlags::Pixel,
    Span<const ShaderMacro> _macrosOrEmpty = {});

void LoadProgram(
    RenderGraph&            _graph,
    StringView              _name,
    StringView              _path,
    Flags<eShaderFlags>     _flags         = eShaderFlags::Vertex | eShaderFlags::Pixel,
    Span<const ShaderMacro> _macrosOrEmpty = {});

[[nodiscard]] Ref<Mesh> LoadMesh(
    StringView          _path,
    const VertexLayout& _attribLayoutOrEmpty);

[[nodiscard]] Ref<Texture> LoadTexture(
    StringView _path);