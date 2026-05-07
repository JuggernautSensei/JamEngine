#include "pch.h"
#include "Utils.h"

#include "../JamEngine/AssimpImporter.h"

ProgramHandle LoadProgram(
    const StringView              _path,
    const Span<const ShaderMacro> _macros,
    const Flags<eShaderFlags>     _flags)
{
    JAM_ASSERT(_flags != eShaderFlags::None, "No shader flags specified.");
    JAM_ASSERT(_flags.IsDisabled(eShaderFlags::Compute), "Use LoadComputeProgram for compute shaders.");

    Graphics& gfx = GetGraphics();

    ShaderHandle vsh = kInvalidHandle;
    ShaderHandle psh = kInvalidHandle;
    ShaderHandle gsh = kInvalidHandle;
    ShaderHandle hsh = kInvalidHandle;
    ShaderHandle dsh = kInvalidHandle;

    ShaderLoadInfo sci = {};
    sci.macros         = _macros;
    sci.flags          = eShaderCompileFlags::Default;

    if (_flags & eShaderFlags::Vertex)
    {
        sci.stage          = eShader::VertexShader;
        sci.entryPoint     = "VSMain";
        ShaderCompiler vsc = ShaderCompiler::CompileHLSLFile(_path, sci);
        JAM_ASSERT(vsc.IsSucceeded(), "Vertex shader compilation failed: {}", vsc.GetErrorMessage());
        vsh = gfx.CreateShader(vsc.GetBytecode());
    }

    if (_flags & eShaderFlags::Pixel)
    {
        sci.stage          = eShader::PixelShader;
        sci.entryPoint     = "PSMain";
        ShaderCompiler psc = ShaderCompiler::CompileHLSLFile(_path, sci);
        JAM_ASSERT(psc.IsSucceeded(), "Pixel shader compilation failed: {}", psc.GetErrorMessage());
        psh = gfx.CreateShader(psc.GetBytecode());
    }

    if (_flags & eShaderFlags::Geometry)
    {
        sci.stage          = eShader::GeometryShader;
        sci.entryPoint     = "GSMain";
        ShaderCompiler gsc = ShaderCompiler::CompileHLSLFile(_path, sci);
        JAM_ASSERT(gsc.IsSucceeded(), "Geometry shader compilation failed: {}", gsc.GetErrorMessage());
        gsh = gfx.CreateShader(gsc.GetBytecode());
    }

    if (_flags & eShaderFlags::Hull)
    {
        sci.stage          = eShader::HullShader;
        sci.entryPoint     = "HSMain";
        ShaderCompiler hsc = ShaderCompiler::CompileHLSLFile(_path, sci);
        JAM_ASSERT(hsc.IsSucceeded(), "Hull shader compilation failed: {}", hsc.GetErrorMessage());
        hsh = gfx.CreateShader(hsc.GetBytecode());
    }

    if (_flags & eShaderFlags::Domain)
    {
        sci.stage          = eShader::DomainShader;
        sci.entryPoint     = "DSMain";
        ShaderCompiler dsc = ShaderCompiler::CompileHLSLFile(_path, sci);
        JAM_ASSERT(dsc.IsSucceeded(), "Domain shader compilation failed: {}", dsc.GetErrorMessage());
        dsh = gfx.CreateShader(dsc.GetBytecode());
    }

    return gfx.CreateProgram(vsh, psh, gsh, hsh, dsh, true);
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
    JAM_ASSERT(importerOrNot, "Failed to import mesh from file '{}': {}", _path, importerOrNot.GetError().ToErrorMessage());
    Expect<Ref<Model>> pModelOrNot = importerOrNot->ConvertToModel(_attribLayoutOrEmpty, eAssimpConvertToModelFlags::Mesh);
    JAM_ASSERT(pModelOrNot, "Failed to convert imported data to model for file '{}': {}", _path, pModelOrNot.GetError().ToErrorMessage());
    Ref<Model> pModel = pModelOrNot.GetValue();
    return pModel->GetMeshes()[0];
}