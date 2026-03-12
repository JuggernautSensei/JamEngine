#pragma once

class ProjectChangedEvent;
class FocusingShaderItemChagnedEvent;

class ShaderItemEditorWindow
{
public:
    void OnImGuiRender();
    void OnEvent(Event& event);

    void  WriteToArchive(ArchiveWriter& _archive) const;
    Error ReadFromArchive(ArchiveReader& _archive);

private:
    void OnProjectChangedEvent_(const ProjectChangedEvent& _event);
    void OnFocusingShaderItemChangedEvent_(const FocusingShaderItemChagnedEvent& _event);

    void OnFileModifiedEvent_(const FileModifiedEvent& _event) const;
    void OnFileDeletedEvent_(const FileDeletedEvent& _event) const;

    void Widget_EditShaderItemName();
    void Widget_EditShaderItemShaderSource() const;
    void Widget_EditShaderItemEntryPoint();
    void Widget_EditShaderItemStage() const;
    void Widget_EditShaderItemCompileFlags() const;

    void OpenShaderSourceAsExternalEditor_(StringView _path) const;

    // focusing context
    Project*          m_pProjectCache            = nullptr;
    const ShaderItem* m_pFocusingShaderItemCache = nullptr;

    // edit context
    String                    m_focusingShaderItemNameEdit       = {};
    Vector<ShaderItem::Macro> m_focusingShaderItemMacrosEdit     = {};
    String                    m_focusingShaderItemEntryPointEdit = {};

    // external editor
    String m_sourceEditorPathOrEmpty = {};   // if empty, use default editor.
};

JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(ShaderItemEditorWindow);