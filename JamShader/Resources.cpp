#include "pch.h"
#include "Resources.h"

#include <ppltasks.h>

#include "Events.h"
#include "ResourceManager.h"
#include "../JamEngine/JsonArchive.h"
#include "../JamEngine/STLAlgorithm.h"

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

Error ShaderSource::Load()
{
    if (const Error err = Load_(m_path))
    {
        return err;
    }
    return Error::None;
}

StringView ShaderSource::GetPath() const
{
    return m_path;
}

StringView ShaderSource::GetSource() const
{
    return m_source;
}

String ShaderSource::MakeFullPath() const
{
    return m_path;
}

Error ShaderSource::Load_(
    const StringView _path)
{
    Expect<String> source = FileReader::ReadFileAsString(_path);
    if (!source)
    {
        return source.GetError();
    }

    m_path   = _path;
    m_source = source.Take();
    return Error::None;
}

void WriteToArchive(
    ArchiveWriter&           _archive,
    const Ref<ShaderSource>& _pShaderSource)
{
    if (!_pShaderSource)
    {
        _archive.Write(nullptr);
        return;
    }

    _archive.Write(_pShaderSource->GetPath());
}

Error ReadFromArchive(
    ArchiveReader&     _archive,
    Ref<ShaderSource>& _pShaderSource)
{
    if (_archive.IsNull())
    {
        _pShaderSource = nullptr;
        return Error::None;
    }

    StringView path = {};
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

void ShaderItem::Macro::WriteToArchive(ArchiveWriter& _archive) const
{
    _archive.WriteField("typeName", name);
    _archive.WriteField("value", value);
    _archive.WriteField("bEnabled", bEnabled);
}

Error ShaderItem::Macro::ReadFromArchive(ArchiveReader& _archive)
{
    if (const Error err = _archive.ReadField("typeName", name))
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

ShaderCompiler ShaderItem::Compile() const
{
    JAM_ASSERT(pShaderSource, "Shader source is null.");

    Vector<ShaderMacro> shaderMacros = {};
    shaderMacros.reserve(macros.size());
    for (const Macro& macro: macros)
    {
        if (!macro.bEnabled)
        {
            continue;
        }

        shaderMacros.emplace_back(macro.name, macro.value);
    }

    ShaderLoadInfo info = {};
    info.entryPoint         = entryPoint;
    info.stage              = stage;
    info.flags              = compileFlags;
    info.macros             = shaderMacros;
    return ShaderCompiler::CompileHLSLSource(pShaderSource->GetSource(), info);
}

void ShaderItem::WriteToArchive(
    ArchiveWriter& _archive) const
{
    _archive.BeginObject();
    _archive.WriteField("typeName", name);
    _archive.WriteField("shaderSource", pShaderSource);
    _archive.WriteField("macros", macros);
    _archive.WriteField("entryPoint", entryPoint);
    _archive.WriteField("stage", stage);
    _archive.WriteField("compileFlags", compileFlags);
    _archive.EndObject();
}

Error ShaderItem::ReadFromArchive(ArchiveReader& _archive)
{
    if (const Error err = _archive.ReadField("typeName", name))
    {
        return err;
    }

    if (const Error err = _archive.ReadField("shaderSource", pShaderSource))
    {
        return err;
    }

    if (const Error err = _archive.ReadField("macros", macros))
    {
        return err;
    }

    if (const Error err = _archive.ReadField("entryPoint", entryPoint))
    {
        return err;
    }

    if (const Error err = _archive.ReadField("stage", stage))
    {
        return err;
    }

    if (const Error err = _archive.ReadField("compileFlags", compileFlags))
    {
        return err;
    }

    return Error::None;
}

Expect<Project> Project::Load(
    const StringView _path)
{
    Project proj = {};
    if (const Error err = proj.LoadFromFile_(_path))
    {
        return err;
    }

    return proj;
}

Expect<Project> Project::LoadNew(
    const StringView _path)
{
    Project proj = {};
    if (const Error err = proj.LoadNew_(_path))
    {
        return err;
    }

    return proj;
}

Error Project::Save()
{
    MarkDirty_(false);
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

    if (FileSystem::Exists(oldPath))
    {
        if (const Error err = FileSystem::Rename(oldPath, newPath))
        {
            return err;
        }

        // dispatch event
        ProjectRenamedEvent event {};
        GetApp().DispatchEvent(event);
    }
    else
    {
        if (const Error err = SaveAs(newPath))
        {
            return err;
        }
    }

    // done
    m_name = newPath.GetBaseName();
    MarkDirty_(false);
    return Error::None;
}

void Project::AddShaderItem(
    const ShaderItem& _item)
{
    JAM_ASSERT(!m_shaderItems.Contains(_item.name), "Shader item with the same typeName already exists.");
    m_shaderItems.PushBack(_item.name, _item);
    MarkDirty_(true);
}

void Project::RemoveShaderItem(
    const StringView _name)
{
    const auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    m_shaderItems.Remove(it);
    MarkDirty_(true);
}

void Project::ClearShaderItems()
{
    if (m_shaderItems.empty())
    {
        return;
    }

    m_shaderItems.Clear();
    MarkDirty_(true);
}

void Project::RenameShaderItem(
    const StringView _name,
    const StringView _newName)
{
    if (_name == _newName)
    {
        return;
    }

    JAM_ASSERT(m_shaderItems.Contains(_name), "Shader item not found.");
    JAM_ASSERT(!m_shaderItems.Contains(_newName), "Shader item with the same typeName already exists.");

    ShaderItem oldValue    = m_shaderItems.Get(_name);
    oldValue.name          = _newName;
    const auto [it, bRet0] = m_shaderItems.Remove(_name);
    JAM_ASSERT(bRet0, "Shader item not found after removal.");
    const auto [_, bRet1] = m_shaderItems.Insert(it, _newName, oldValue);
    JAM_ASSERT(bRet1, "Failed to insert shader item with new typeName.");
    MarkDirty_(true);
}

void Project::MoveShaderItemBefore(
    const StringView _name,
    const StringView _to)
{
    const auto fromIt = m_shaderItems.Find(_name);
    const auto toIt   = m_shaderItems.Find(_to);
    JAM_ASSERT(fromIt != m_shaderItems.end(), "Shader item not found.");
    JAM_ASSERT(toIt != m_shaderItems.end(), "Shader item not found.");

    if (fromIt == toIt)
    {
        return;
    }

    m_shaderItems.Move(_name, _to);
}

const ShaderItem& Project::GetShaderItem(
    const StringView _name) const
{
    const auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    return it->value;
}

bool Project::ContainsShaderItem(
    const StringView _name) const
{
    return m_shaderItems.Contains(_name);
}

const OrderedStringHashMap<ShaderItem>& Project::GetShaderItems() const
{
    return m_shaderItems;
}

void Project::SetShaderItem(
    const StringView  _name,
    const ShaderItem& _item)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    it->value = _item;
    MarkDirty_(true);
}

void Project::SetShaderItemSource(
    const StringView         _name,
    const Ref<ShaderSource>& _pShaderSource)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    if (it->value.pShaderSource == _pShaderSource)
    {
        return;
    }

    it->value.pShaderSource = _pShaderSource;
    MarkDirty_(true);
}

void Project::SetShaderItemEntryPoint(
    const StringView _name,
    const StringView _entryPoint)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    if (it->value.entryPoint == _entryPoint)
    {
        return;
    }

    it->value.entryPoint = _entryPoint;
    MarkDirty_(true);
}

void Project::SetShaderItemStage(
    const StringView _name,
    const eShader    _stage)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    if (it->value.stage == _stage)
    {
        return;
    }

    it->value.stage = _stage;
    MarkDirty_(true);
}

void Project::SetShaderItemCompileFlags(
    const StringView                 _name,
    const Flags<eShaderCompileFlags> _compileFlags)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    if (it->value.compileFlags == _compileFlags)
    {
        return;
    }

    it->value.compileFlags = _compileFlags;
    MarkDirty_(true);
}

void Project::PushShaderItemMacro(
    const StringView         _name,
    const ShaderItem::Macro& _macro)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    it->value.macros.push_back(_macro);
    MarkDirty_(true);
}

void Project::RemoveShaderItemMacro(
    const StringView _name,
    const SizeT      _idx)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    JAM_ASSERT(_idx < it->value.macros.size(), "Index out of bounds.");
    it->value.macros.erase(it->value.macros.begin() + _idx);
    MarkDirty_(true);
}

void Project::SetShaderItemEnabled(
    const StringView _name,
    const bool       _bEnabled)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    if (it->value.bEnabled == _bEnabled)
    {
        return;
    }
    it->value.bEnabled = _bEnabled;
    MarkDirty_(true);
}

void Project::SetShaderItemMacro(
    const StringView         _name,
    const SizeT              _idx,
    const ShaderItem::Macro& _macro)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    JAM_ASSERT(_idx < it->value.macros.size(), "Index out of bounds.");
    it->value.macros[_idx] = _macro;
    MarkDirty_(true);
}

void Project::MoveShaderItemMacroBefore(
    const StringView _name,
    const SizeT      _fromIdx,
    const SizeT      _toIdx)
{
    auto it = m_shaderItems.Find(_name);
    JAM_ASSERT(it != m_shaderItems.end(), "Shader item not found.");
    JAM_ASSERT(_fromIdx < it->value.macros.size(), "From index out of bounds.");
    JAM_ASSERT(_toIdx < it->value.macros.size(), "To index out of bounds.");
    VectorSplice(it->value.macros, _toIdx, _fromIdx);
    MarkDirty_(true);
}

StringView Project::GetName() const
{
    return m_name;
}

StringView Project::GetWorkingDirectory() const
{
    return m_workingDir;
}

String Project::MakeFullPath() const
{
    String path = String::Joined(m_workingDir, m_name, eJoin::File);
    path.ReplaceExtension(kExtension);
    return path;
}

bool Project::IsDirty() const
{
    return m_bDirty;
}

void Project::WriteToArchive(
    ArchiveWriter& _archive) const
{
    _archive.BeginObject();
    _archive.AddField("shaders");
    _archive.BeginArray();
    for (const auto& [name, shaderItem]: m_shaderItems)
    {
        _archive.BeginObject();
        _archive.WriteField("typeName", name);
        _archive.WriteField("item", shaderItem);
        _archive.EndObject();
    }
    _archive.EndArray();
    _archive.EndObject();
}

Error Project::ReadFromArchive(
    ArchiveReader& _archive)
{
    m_shaderItems.Clear();

    // shaders
    if (const Error err = _archive.EnterField("shaders"))
    {
        return err;
    }

    _archive.ForEachArray(Bind(
        [this](const SizeT, ArchiveReader& _reader) -> Error {
            StringView name = {};
            if (const Error err = _reader.ReadField("typeName", name))
            {
                return err;
            }

            if (m_shaderItems.Contains(name))
            {
                JAM_LOG_ERROR("Duplicate shader item typeName: {}", name);
                return Error::InvalidData;
            }

            ShaderItem shaderItem = {};
            if (const Error err = _reader.ReadField("item", shaderItem))
            {
                return err;
            }

            m_shaderItems.PushBack(name, shaderItem);
            return Error::None;
        }));

    _archive.Exit();

    MarkDirty_(false);
    return Error::None;
}

Error Project::LoadNew_(
    const StringView _path)
{
    JsonWriter writer = {};
    writer.Write(*this);
    if (const Error err = writer.SaveToFile(_path, eJsonWriteFlags::Pretty))
    {
        return err;
    }

    const String path = FileSystem::Absolute(_path);
    m_workingDir      = path.GetDirectory();
    m_name            = path.GetBaseName();
    MarkDirty_(false);
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
    MarkDirty_(false);
    return Error::None;
}

void Project::MarkDirty_(
    const bool _bDirty)
{
    if (m_bDirty == _bDirty)
    {
        return;
    }

    m_bDirty = _bDirty;
    ProjectDirtyChangedEvent event {};
    GetApp().DispatchEvent(event);
}
