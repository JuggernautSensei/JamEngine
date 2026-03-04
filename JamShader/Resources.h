#pragma once
#include "../JamEngine/OrderedHashMap.h"

////////// Resource interface //////////

enum class eResourceChangeType
{
    Modified,
    Loaded,
    Saved,
    Renamed,
};

class Resource : public RefCounted
{
    // ReSharper disable once CppAccessSpecifierWithNoDeclarations
public:
};

using OnChangeSignal           = Signal<void(const Ref<Resource>&, eResourceChangeType)>;
using OnChangeDelegate         = OnChangeSignal::DelegateT;
using OnChangeScopedConnection = OnChangeSignal::ScopedConnection;

////////// Shader Source //////////

class ShaderSource : public Resource
{
public:
    [[nodiscard]] static Expect<Ref<ShaderSource>> Create(StringView _path);

    Error Reload();
    Error Save();
    Error SaveAs(StringView _path) const;
    Error Rename(StringView _path);
    void  SetSource(StringView _source);

    [[nodiscard]] StringView GetPath() const;
    [[nodiscard]] StringView GetBaseName() const;
    [[nodiscard]] StringView GetSource() const;
    [[nodiscard]] String     MakeFullPath() const;

    [[nodiscard]] bool IsDirty() const;

private:
    ShaderSource() = default;
    Error Load_(StringView _path);

    String m_path     = {};
    String m_baseName = {};
    String m_source   = {};
    bool   m_bDirty   = false;
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<ShaderSource>& _pShaderSource);
Error ReadFromArchive(ArchiveReader& _archive, Ref<ShaderSource>& _pShaderSource);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<ShaderSource>);

struct ShaderMacroItem
{
    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

    String name     = {};
    String value    = {};
    bool   bEnabled = false;
};
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(ShaderMacroItem);

class ShaderItem : public Resource
{
    enum class eDirtyFlags : UInt32
    {
        None         = 0,
        Self         = 1 << 0,
        ShaderSource = 1 << 1,
    };

public:
    [[nodiscard]] Expect<ShaderCompiler> Compile();

    void SetName(StringView _name);
    void SetShaderSource(const Ref<ShaderSource>& _pShaderSource);
    void SetMacros(const List<ShaderMacroItem>& _macros);
    void SetEntryPoint(StringView _entryPoint);
    void SetStage(eShader _stage);
    void SetCompileFlags(Flags<eShaderCompileFlags> _flags);

    StringView                   GetName() const;
    const Ref<ShaderSource>&     GetShaderSource() const;
    const List<ShaderMacroItem>& GetMacros() const;
    StringView                   GetEntryPoint() const;
    eShader                      GetStage() const;
    Flags<eShaderCompileFlags>   GetCompileFlags() const;
    bool                         IsDirty() const;

    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

private:
    void OnShaderSourceChanged_(const Ref<Resource>& _pResource, eResourceChangeType _type);

    String                     m_name          = {};
    Ref<ShaderSource>          m_pShaderSource = nullptr;
    List<ShaderMacroItem>      m_macros        = {};
    String                     m_entryPoint    = {};
    eShader                    m_stage         = eShader::VertexShader;
    Flags<eShaderCompileFlags> m_compileFlags  = eShaderCompileFlags::Default;
    Flags<eDirtyFlags>         m_dirtyFlags    = eDirtyFlags::None;

    OnChangeScopedConnection m_shaderSourceOnChangeConnection = {};
};
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(ShaderItem);

class Project : public RefCounted
{
public:
    [[nodiscard]] static Expect<Ref<Project>> LoadFromFile(StringView _path);
    [[nodiscard]] static Expect<Ref<Project>> CreateNew(StringView _path);

    Error Save();
    Error SaveAs(StringView _path);
    Error Rename(StringView _newName);

    [[nodiscard]] StringView GetWorkingDirectory();
    [[nodiscard]] StringView GetName();
    [[nodiscard]] String     MakeFullPath() const;
    [[nodiscard]] bool       IsDirty() const;

    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

    constexpr static StringView k_extension = ".jshproj";
    constexpr static StringView k_filter    = "*.jshproj";

private:
    Project() = default;

    Error LoadFromFile_(StringView _path);
    Error CreateNew_(StringView _path);

    String                                  m_workingDir  = {};
    String                                  m_name        = {};
    OrderedHashMap<String, Ref<ShaderItem>> m_shaderItems = {};

    bool m_bDirty = false;
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Project);
