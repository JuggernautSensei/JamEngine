#include "pch.h"
#include "Resources.h"

#include <ppltasks.h>

#include "Events.h"
#include "ResourceManager.h"
#include "../JamEngine/JsonArchive.h"

Expect<Ref<ShaderSource>> ShaderSource::Create(
    const StringView _path)
{
    Ref<ShaderSource> shaderSource = new ShaderSource();
    if (const Error err = shaderSource->Load_(_path))
    {
        return err;
    }
    return shaderSource;
}

Error ShaderSource::Reload()
{
    if (const Error err = Load_(m_path))
    {
        return err;
    }
    return Error::None;
}

Error ShaderSource::Save()
{
    if (!m_bDirty && FileSystem::Exists(m_path))
    {
        return Error::None;
    }

    if (const Error err = SaveAs(MakeFullPath()))
    {
        return err;
    }

    m_bDirty = false;
    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Saved);
    return Error::None;
}

Error ShaderSource::SaveAs(
    const StringView _path) const
{
    Expect<FileWriter> writer = FileWriter::Open(_path);
    if (!writer)
    {
        return writer.GetError();
    }

    writer->Write(m_source);
    return Error::None;
}

Error ShaderSource::Rename(
    const StringView _path)
{
    if (!m_bDirty && FileSystem::Exists(m_path))
    {
        if (const Error err = FileSystem::Rename(m_path, _path))
        {
            return err;
        }
    }
    else
    {
        if (const Error err = SaveAs(_path))
        {
            return err;
        }
    }

    m_path     = _path;
    m_baseName = _path.GetBaseName();
    m_bDirty   = false;
    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Renamed);
    return Error::None;
}

void ShaderSource::SetSource(
    const StringView _source)
{
    if (m_source == _source)
    {
        return;
    }

    m_source = _source;
    m_bDirty = true;
    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Modified);
}

StringView ShaderSource::GetPath() const
{
    return m_path;
}

StringView ShaderSource::GetBaseName() const
{
    return m_baseName;
}

StringView ShaderSource::GetSource() const
{
    return m_source;
}

String ShaderSource::MakeFullPath() const
{
    return m_path;
}

bool ShaderSource::IsDirty() const
{
    return m_bDirty;
}

Error ShaderSource::Load_(const StringView _path)
{
    Expect<String> source = FileReader::ReadFileAsString(_path);
    if (!source)
    {
        return source.GetError();
    }

    m_path     = _path;
    m_baseName = _path.GetBaseName();
    m_source   = source.Take();

    m_bDirty = false;
    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Loaded);
    return Error::None;
}

void WriteToArchive(
    ArchiveWriter&           _archive,
    const Ref<ShaderSource>& _pShaderSource)
{
    _archive.Write(_pShaderSource->GetPath());
}

Error ReadFromArchive(
    ArchiveReader&     _archive,
    Ref<ShaderSource>& _pShaderSource)
{
    String path = {};
    if (const Error err = _archive.Read(path))
    {
        return err;
    }

    Expect<Ref<ShaderSource>> pShaderSource = GetResourceManager().LoadShaderSource(path);
    if (!pShaderSource)
    {
        return pShaderSource.GetError();
    }

    _pShaderSource = pShaderSource.Take();
    return Error::None;
}

void ShaderMacroItem::WriteToArchive(ArchiveWriter& _archive) const
{
    _archive.WriteField("name", name);
    _archive.WriteField("value", value);
    _archive.WriteField("bEnabled", bEnabled);
}

Error ShaderMacroItem::ReadFromArchive(ArchiveReader& _archive)
{
    if (const Error err = _archive.ReadField("name", name))
    {
        return err;
    }

    if (const Error err = _archive.ReadField("value", value))
    {
        return err;
    }

    if (const Error err = _archive.ReadField("bEnabled", bEnabled))
    {
        return err;
    }

    return Error::None;
}

Expect<ShaderCompiler> ShaderItem::Compile()
{
    // clean-up dirty

    Vector<ShaderMacro> macros = {};
    macros.reserve(m_macros.size());
    for (const ShaderMacroItem& macroItem: m_macros)
    {
        if (!macroItem.bEnabled)
        {
            continue;
        }

        macros.emplace_back(macroItem.name, macroItem.value);
    }

    ShaderCompilerInfo info = {};
    info.entryPoint         = m_entryPoint;
    info.stage              = m_stage;
    info.flags              = m_compileFlags;
    info.macros             = macros;
    return ShaderCompiler::CompileFromHLSLSource(m_pShaderSource->GetSource(), info);
}

void ShaderItem::SetName(
    const StringView _name)
{
    if (m_name == _name)
    {
        return;
    }

    m_name = _name;
    m_dirtyFlags |= eDirtyFlags::Self;
    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Renamed);
}

void ShaderItem::SetShaderSource(
    const Ref<ShaderSource>& _pShaderSource)
{
    if (m_pShaderSource == _pShaderSource)
    {
        return;
    }

    m_pShaderSource = _pShaderSource;
    m_dirtyFlags |= eDirtyFlags::Self;

    if (m_pShaderSource)
    {
        m_shaderSourceOnChangeConnection = GetResourceManager().ConnectResourceOnChangeScoped(m_pShaderSource, Bind<&ShaderItem::OnShaderSourceChanged_>(this));
        if (m_pShaderSource->IsDirty())
        {
            m_dirtyFlags |= eDirtyFlags::ShaderSource;
        }
    }

    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Modified);
}

void ShaderItem::SetMacros(
    const List<ShaderMacroItem>& _macros)
{
    if (m_macros == _macros)
    {
        return;
    }

    m_macros = _macros;
    m_dirtyFlags |= eDirtyFlags::Self;
    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Modified);
}

void ShaderItem::SetEntryPoint(
    const StringView _entryPoint)
{
    if (m_entryPoint == _entryPoint)
    {
        return;
    }

    m_entryPoint = _entryPoint;
    m_dirtyFlags |= eDirtyFlags::Self;
    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Modified);
}

void ShaderItem::SetStage(
    const eShader _stage)
{
    if (m_stage == _stage)
    {
        return;
    }

    m_stage = _stage;
    m_dirtyFlags |= eDirtyFlags::Self;
    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Modified);
}

void ShaderItem::SetCompileFlags(
    const Flags<eShaderCompileFlags> _flags)
{
    if (m_compileFlags == _flags)
    {
        return;
    }

    m_compileFlags = _flags;
    m_dirtyFlags   = eDirtyFlags::Self;
    GetResourceManager().NotifyResourceChange(this, eResourceChangeType::Modified);
}

StringView ShaderItem::GetName() const
{
    return m_name;
}

const Ref<ShaderSource>& ShaderItem::GetShaderSource() const
{
    return m_pShaderSource;
}

const List<ShaderMacroItem>& ShaderItem::GetMacros() const
{
    return m_macros;
}

StringView ShaderItem::GetEntryPoint() const
{
    return m_entryPoint;
}

eShader ShaderItem::GetStage() const
{
    return m_stage;
}

Flags<eShaderCompileFlags> ShaderItem::GetCompileFlags() const
{
    return m_compileFlags;
}

bool ShaderItem::IsDirty() const
{
    return m_dirtyFlags != eDirtyFlags::None;
}

void ShaderItem::WriteToArchive(
    ArchiveWriter& _archive) const
{
    _archive.WriteField("shaderSource", m_pShaderSource);
    _archive.WriteField("m_macros", m_macros);
    _archive.WriteField("m_entryPoint", m_entryPoint);
    _archive.WriteField("m_stage", m_stage);
    _archive.WriteField("m_compileFlags", m_compileFlags);
}

Error ShaderItem::ReadFromArchive(ArchiveReader& _archive)
{
    if (const Error err = _archive.Read(m_pShaderSource))
    {
        return err;
    }

    if (const Error err = _archive.Read(m_macros))
    {
        return err;
    }

    if (const Error err = _archive.Read(m_entryPoint))
    {
        return err;
    }

    if (const Error err = _archive.Read(m_stage))
    {
        return err;
    }

    if (const Error err = _archive.Read(m_compileFlags))
    {
        return err;
    }

    return Error::None;
}

void ShaderItem::OnShaderSourceChanged_(
    const Ref<Resource>& _pResource,
    const eResourceChangeType)
{
    JAM_ASSERT(_pResource == m_pShaderSource, "Unexpected resource change event received for a shader item");
    JAM_ASSERT(_pResource, "Shader item has no shader source assigned");

    if (m_pShaderSource->IsDirty())
    {
        m_dirtyFlags |= eDirtyFlags::ShaderSource;
    }
    else
    {
        m_dirtyFlags.Disable(eDirtyFlags::ShaderSource);
    }
}

Expect<Ref<Project>> Project::LoadFromFile(
    const StringView _path)
{
    Ref<Project> pProject = new Project();
    if (const Error err = pProject->LoadFromFile_(_path))
    {
        return err;
    }

    return pProject;
}

Expect<Ref<Project>> Project::CreateNew(
    const StringView _path)
{
    Ref<Project> pProject = new Project();
    if (const Error err = pProject->CreateNew_(_path))
    {
        return err;
    }

    return pProject;
}

Error Project::Save()
{
    if (!m_bDirty && FileSystem::Exists(MakeFullPath()))
    {
        return Error::None;
    }

    m_bDirty = false;
    return SaveAs(MakeFullPath());
}

Error Project::SaveAs(
    const StringView _path)
{
    JsonWriter writer = {};
    writer.Write(*this);
    if (const Error err = writer.SaveToFile(_path, eJsonWriteFlags::Pretty))
    {
        return err;
    }

    return Error::None;
}

Error Project::Rename(
    const StringView _newName)
{
    const String oldPath = MakeFullPath();
    String       newPath = oldPath;
    newPath.ReplaceBaseName(_newName);

    if (m_bDirty)   // overwrite
    {
        if (const Error err = SaveAs(newPath))
        {
            return err;
        }
    }
    else
    {
        if (const Error err = FileSystem::Rename(oldPath, newPath))
        {
            return err;
        }
    }

    // done
    m_name = newPath.GetBaseName();

    // dispatch event
    ProjectRenamedEvent event { oldPath, newPath };
    GetApp().DispatchEvent(event);
    return Error::None;
}

void Project::SetShaders(const OrderedHashMap<Ref<ShaderItem>>& _shaders)
{
    if (m_shaders == _shaders)
    {
        return;
    }

    m_shaders = _shaders;
    m_bDirty  = true;
}

Ref<ShaderItem> Project::AddNewShader()
{
    constexpr StringView k_basename = "new shader";

    for (Int32 i = 0;; ++i)
    {
        String name = k_basename;
        if (i > 0)
        {
            name.AppendFormat(" {}", i);
        }

        if (!m_shaders.Contains(MakeRef<ShaderItem>(name)))
        {
            Ref<ShaderItem> pShader = MakeRef<ShaderItem>();
            pShader->SetName(name);
            m_shaders.PushBack(pShader);

            m_bDirty = true;

            return pShader;
        }
    }
}

const OrderedHashMap<Ref<ShaderItem>>& Project::GetShaders() const
{
    return m_shaders;
}

StringView Project::GetWorkingDirectory()
{
    return m_workingDir;
}

StringView Project::GetName()
{
    return m_name;
}

String Project::MakeFullPath() const
{
    String path = String::Joined(m_workingDir, m_name, eJoinType::File);
    path.ReplaceExtension(k_extension);
    return path;
}

bool Project::IsDirty() const
{
    return m_bDirty;
}

void Project::WriteToArchive(
    ArchiveWriter& _archive) const
{
    _archive.OpenField("shaders");
    for (const Ref<ShaderItem>& pShader: m_shaders)
    {
        _archive.PushBack(*pShader);
    }
    _archive.Close();
}

Error Project::ReadFromArchive(ArchiveReader& _archive)
{
    m_shaders.Clear();

    if (const Error err = _archive.EnterField("shaders"))
    {
        return err;
    }

    _archive.ForEach(
        [this](const SizeT, ArchiveReader& _reader) -> Error
        {
            Ref<ShaderItem> pShader = MakeRef<ShaderItem>();
            if (const Error err = _reader.Read(*pShader))
            {
                return err;
            }

            if (!m_shaders.PushBack(pShader).bSucceeded)
            {
                JAM_LOG_ERROR("duplicated shader item found in project file");
                return Error::InvalidData;
            }

            return Error::None;
        });

    _archive.Exit();

    return Error::None;
}

Error Project::LoadFromFile_(
    const StringView _path)
{
    Expect<JsonReader> reader = JsonReader::ParseFromFile(_path);
    if (!reader)
    {
        return reader.GetError();
    }

    if (const Error err = reader->Read(*this))
    {
        return err;
    }

    const String path = FileSystem::Absolute(_path);
    m_workingDir      = path.GetDirectory();
    m_name            = path.GetBaseName();
    return Error::None;
}

Error Project::CreateNew_(
    const StringView _path)
{
    const String path = FileSystem::Absolute(_path);
    m_workingDir      = path.GetDirectory();
    m_name            = path.GetBaseName();
    return SaveAs(_path);
}
