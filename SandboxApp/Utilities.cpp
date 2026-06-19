#include "pch.h"
#include "Utilities.h"

#include "../JamEngine/AssimpImporter.h"
#include "../JamEngine/RenderGraph.h"

ProgramHandle LoadProgram(
    const StringView              _path,
    const Flags<eShaderFlags>     _flags,
    const Span<const ShaderMacro> _macrosOrEmpty)
{
    JAM_ASSERT(_flags != eShaderFlags::None, "No shader flags specified.");
    JAM_ASSERT(_flags == eShaderFlags::Compute || _flags.IsDisabled(eShaderFlags::Compute), "compute shader must be the only shader stage when specified.");

    Graphics& gfx = GetGraphics();

    ShaderHandle vsh = kInvalidHandle;
    ShaderHandle psh = kInvalidHandle;
    ShaderHandle gsh = kInvalidHandle;
    ShaderHandle hsh = kInvalidHandle;
    ShaderHandle dsh = kInvalidHandle;
    ShaderHandle csh = kInvalidHandle;

    ShaderCompileInfo sci = {};
    sci.macros             = _macrosOrEmpty;
    sci.flags              = eShaderCompileFlags::Default;

    if (_flags & eShaderFlags::Vertex)
    {
        sci.stage                  = eShader::VertexShader;
        sci.entryPoint             = "VSMain";
        Expect<ShaderBinary> vsc = ShaderBinary::CompileHLSLFile(_path, sci);
        JAM_ASSERT(vsc, "Vertex shader compilation failed: {}", vsc.GetError());
        vsh = gfx.CreateShader(vsc->GetBytecode());
    }

    if (_flags & eShaderFlags::Pixel)
    {
        sci.stage          = eShader::PixelShader;
        sci.entryPoint     = "PSMain";
        Expect<ShaderBinary> psc = ShaderBinary::CompileHLSLFile(_path, sci);
        JAM_ASSERT(psc, "Pixel shader compilation failed: {}", psc.GetError());
        psh = gfx.CreateShader(psc->GetBytecode());
    }

    if (_flags & eShaderFlags::Geometry)
    {
        sci.stage          = eShader::GeometryShader;
        sci.entryPoint     = "GSMain";
        Expect<ShaderBinary> gsc = ShaderBinary::CompileHLSLFile(_path, sci);
        JAM_ASSERT(gsc, "Geometry shader compilation failed: {}", gsc.GetError());
        gsh = gfx.CreateShader(gsc->GetBytecode());
    }

    if (_flags & eShaderFlags::Hull)
    {
        sci.stage          = eShader::HullShader;
        sci.entryPoint     = "HSMain";
        Expect<ShaderBinary> hsc = ShaderBinary::CompileHLSLFile(_path, sci);
        JAM_ASSERT(hsc, "Hull shader compilation failed: {}", hsc.GetError());
        hsh = gfx.CreateShader(hsc->GetBytecode());
    }

    if (_flags & eShaderFlags::Domain)
    {
        sci.stage          = eShader::DomainShader;
        sci.entryPoint     = "DSMain";
        Expect<ShaderBinary> dsc = ShaderBinary::CompileHLSLFile(_path, sci);
        JAM_ASSERT(dsc, "Domain shader compilation failed: {}", dsc.GetError());
        dsh = gfx.CreateShader(dsc->GetBytecode());
    }

    if (_flags & eShaderFlags::Compute)
    {
        sci.stage          = eShader::ComputeShader;
        sci.entryPoint     = "CSMain";
        Expect<ShaderBinary> csc = ShaderBinary::CompileHLSLFile(_path, sci);
        JAM_ASSERT(csc, "Compute shader compilation failed: {}", csc.GetError());
        csh = gfx.CreateShader(csc->GetBytecode());
    }

    if (_flags == eShaderFlags::Compute)
    {
        return gfx.CreateProgram(csh, true);
    }
    else
    {
        return gfx.CreateProgram(vsh, psh, gsh, hsh, dsh, true);
    }
}

void LoadProgram(
    RenderGraph&                  _graph,
    const StringView              _name,
    const StringView              _path,
    const Flags<eShaderFlags>     _flags,
    const Span<const ShaderMacro> _macrosOrEmpty)
{
    const ProgramHandle ph = LoadProgram(_path, _flags, _macrosOrEmpty);
    _graph.InsertProgram(_name, ph, true);
}

Ref<Mesh> LoadMesh(
    const StringView    _path,
    const VertexLayout& _attribLayoutOrEmpty)
{
    Flags<eAssimpImporterFlags> flags = eAssimpImporterFlags::LoadMeshes;
    if (_attribLayoutOrEmpty.Has(eVertexAttribute::Normal))
    {
        flags |= eAssimpImporterFlags::GenerateNormals;
    }
    if (_attribLayoutOrEmpty.Has(eVertexAttribute::Tangent) || _attribLayoutOrEmpty.Has(eVertexAttribute::Bitangent))
    {
        flags |= eAssimpImporterFlags::GenerateTangentFrame;
    }

    Expect<AssimpImporter> importerOrNot = AssimpImporter::ImportFromFile(_path, flags);
    JAM_ASSERT(importerOrNot, "Failed to import mesh from file '{}'. {}", _path, importerOrNot.GetError());
    Expect<Ref<Model>> pModelOrNot = importerOrNot->ConvertToModel(_attribLayoutOrEmpty, eAssimpConvertToModelFlags::Mesh);
    JAM_ASSERT(pModelOrNot, "Failed to convert imported data to model for file '{}'. {}", _path, pModelOrNot.GetError());
    Ref<Model> pModel = pModelOrNot.GetValue();
    return pModel->GetMeshes()[0];
}

Ref<Texture> LoadTexture(
    const StringView _path)
{
    Expect<Ref<Texture>> pTexture = Texture::LoadFromFile(_path);
    JAM_ASSERT(pTexture, "Failed to load texture from file '{}': {}", _path, pTexture.GetError().ToErrorMessage());
    return pTexture.GetValue();
}