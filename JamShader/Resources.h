#pragma once
#include "../JamEngine/OrderedHashMap.h"

// gui payload magics
constexpr StringView k_pathDragDropPayload = "_PATH_";

class ShaderSource : public RefCounted
{
public:
    [[nodiscard]] static Expect<Ref<ShaderSource>> Create(StringView _path);

    Error Load();
    Error Rename(StringView _path);

    [[nodiscard]] StringView GetPath() const;
    [[nodiscard]] StringView GetSource() const;
    [[nodiscard]] String     MakeFullPath() const;

private:
    ShaderSource() = default;
    Error Load_(StringView _path);

    String m_path   = {};
    String m_source = {};
};

void  WriteToArchive(ArchiveWriter& _archive, const Ref<ShaderSource>& _pShaderSource);
Error ReadFromArchive(ArchiveReader& _archive, Ref<ShaderSource>& _pShaderSource);
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Ref<ShaderSource>);

class ShaderItem
{
public:
    struct Macro
    {
        void  WriteToArchive(ArchiveWriter& _archive) const;
        Error ReadFromArchive(ArchiveReader& _archive);

        String name     = {};
        String value    = {};
        bool   bEnabled = false;
    };
    JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Macro);

    [[nodiscard]] ShaderCompiler Compile() const;

    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

    Ref<ShaderSource>          pShaderSource = nullptr;
    Vector<Macro>              macros        = {};
    String                     entryPoint    = {};
    eShader                    stage         = eShader::VertexShader;
    Flags<eShaderCompileFlags> compileFlags  = eShaderCompileFlags::None;
};
JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(ShaderItem);

class Project
{
public:
    [[nodiscard]] static Expect<Project> LoadFromFile(StringView _path);
    [[nodiscard]] static Expect<Project> LoadNew(StringView _path);

    Error Save();
    Error SaveAs(StringView _path);
    Error Rename(StringView _newName);

    // shader item management
    void                                                  AddShaderItem(StringView _name, const ShaderItem& _item);
    void                                                  RemoveShaderItem(StringView _name);
    void                                                  RenameShaderItem(StringView _oldName, StringView _newName);
    [[nodiscard]] const ShaderItem&                       GetShaderItem(StringView _name) const;
    [[nodiscard]] bool                                    ContainsShaderItem(StringView _name) const;
    [[nodiscard]] const OrderedStringHashMap<ShaderItem>& GetShaderItems();

    // shader item operator
    void SetShaderItem(StringView _name, const ShaderItem& _item);
    void SetShaderItemSource(StringView _name, const Ref<ShaderSource>& _pShaderSource);
    void SetShaderItemEntryPoint(StringView _name, StringView _entryPoint);
    void SetShaderItemStage(StringView _name, eShader _stage);
    void SetShaderItemCompileFlags(StringView _name, Flags<eShaderCompileFlags> _compileFlags);
    void PushShaderItemMacro(StringView _name, const ShaderItem::Macro& _macro);
    void InsertShaderItemMacro(StringView _name, SizeT _idx, const ShaderItem::Macro& _macro);
    void RemoveShaderItemMacro(StringView _name, SizeT _idx);
    void SetShaderItemMacro(StringView _name, SizeT _idx, const ShaderItem::Macro& _macro);
    void MoveShaderItemMacro(StringView _name, SizeT _fromIdx, SizeT _toIdx);

    [[nodiscard]] StringView GetBaseName() const;
    [[nodiscard]] StringView GetWorkingDirectory() const;
    [[nodiscard]] String     MakeFullPath() const;

    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

    constexpr static StringView k_extension = ".jshproj";
    constexpr static StringView k_filter    = "*.jshproj";

private:
    Project() = default;
    Error LoadNew_(StringView _path);
    Error LoadFromFile_(StringView _path);

    String                           m_workingDir  = {};
    String                           m_basename    = {};
    OrderedStringHashMap<ShaderItem> m_shaderItems = {};
    bool                             m_bDirty      = false;
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(Project);
