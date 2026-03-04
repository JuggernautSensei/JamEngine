#pragma once

class ProjectChangedEvent;
class FocusingShaderItemChangedEvent;

class WorkspaceWindow
{
public:
    void OnImGuiRender();
    void OnEvent(Event& _event);

private:
    void OnProjectChangedEvent_(const ProjectChangedEvent& _event);

    void Widget_ProjectName_();

    void NewShaderItem_() const;
    void CloneFocisingShaderItem_() const;
    void RemoveFocisingShaderItem_() const;
    void ClearShaderItems_() const;
    void SetFocusingShaderItem_(const Ref<ShaderItem>& _pShaderItem);

    String m_renameProjectBuffer = {};
    bool   m_bRenamingProject    = false;

    Ref<Project>    m_pProject            = nullptr;
    Ref<ShaderItem> m_pFocusingShaderItem = nullptr;
};
