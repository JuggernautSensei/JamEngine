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
    void Widget_EditShaderItemMacros();
    void Widget_ShaderSourceTextViewer(ImVec2 _size);

    void OpenShaderSourceAsExternalEditor_(StringView _path) const;

    void InitFocusingContext_(Project* _pFocusingProjectOrNull, StringView _focusingShaderItemOrEnpty);
    void ClearFocusingContext_();
    void InitEditContext_(StringView _name, const ShaderItem* _pShaderItem);
    void ClearEditContext_();

    // focusing context
    Project*          m_pProject               = nullptr;
    String            m_focusingShaderItemName = {};
    const ShaderItem* m_pFocusingShaderItem    = nullptr;

    // edit context
    String                    m_focusingShaderItemRenameEdit     = {};
    Vector<ShaderItem::Macro> m_focusingShaderItemMacrosEdit     = {};
    String                    m_focusingShaderItemEntryPointEdit = {};

    // external editor
    String m_shaderSourceEditorPathOrEmpty = {};   // if empty, use default editor.
};


JAM_STATIC_ASSERT_IS_ARCHIVE_SERIALIZABLE_TYPE(ShaderItemEditorWindow);
